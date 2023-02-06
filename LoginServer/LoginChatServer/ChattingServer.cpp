#pragma comment(lib, "NetworkLib.lib")
#pragma comment (lib, "cpp_redis.lib")
#pragma comment (lib, "tacopie.lib")

#include "NetworkLib/NetworkLib.h"

#include <cpp_redis/cpp_redis>
#include <codecvt>
#include <string>

#include <DbgHelp.h>
#include <crtdbg.h>
#include <Psapi.h>
#include <string>

#include "Logger.h"

#include "CCrashDump.h"

#include "Pair.h"
#include "FixedHashMap.h"

#include "EPacketType.h"
#include "ChattingServer.h"

#define LOG_FILE_NAME (L"ChatServerLog.txt")

static bool gExit;

static size_t HashSessionId(sessionID_t id)
{
	id ^= id >> 32;
	id *= 0xd6e8feb86659fd93U;
	id ^= id >> 32;
	id *= 0xd6e8feb86659fd93U;
	id ^= id >> 32;

	return id;
}

static size_t Hash64(INT64 id)
{
	id ^= id >> 32;
	id *= 0xd6e8feb86659fd93U;
	id ^= id >> 32;
	id *= 0xd6e8feb86659fd93U;
	id ^= id >> 32;

	return id;
}

ChattingServer::ChattingServer(NetworkLib::WANServer* server)
	: mNetServer(server),
	mhWorkerThread(INVALID_HANDLE_VALUE),
	mhTimeoutThread(INVALID_HANDLE_VALUE),
	mhNetworkEvent(INVALID_HANDLE_VALUE),
	mPlayerPool(),
	mContentMessagePool(),
	mMessageQueue(),
	mPlayers(nullptr),
	mNumLoginSuccess(0),
	mNumSessionKeyNotMatched(0),
	mNumChatPacket(0),
	mNumLoginPacket(0),
	mNumSectorMovePacket(0),
	mNumUpdate(0),
	mNumTimeoutPlayer(0)
{
	mhNetworkEvent = CreateEvent(nullptr, false, false, nullptr);
}

ChattingServer::~ChattingServer()
{
	// 플레이어 삭제, 메시지 큐 비우기
	mNetServer->Terminate();

	gExit = true;

	WaitForSingleObject(mhWorkerThread, INFINITE);
	WaitForSingleObject(mhTimeoutThread, INFINITE);
	CloseHandle(mhWorkerThread);
	CloseHandle(mhNetworkEvent);

	delete mPlayers;
	//delete mSectors;
}

bool ChattingServer::TryRun(const unsigned long IP, const unsigned short port, const unsigned int numWorkerThread, const unsigned int numRunningThread, const unsigned int maxSessionCount, const bool bSupportsNagle, unsigned char fixed)
{
	mRedis.connect();

	mhRedisCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, numRunningThread);
	if (mhRedisCompletionPort == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	mhRedisThreads.push_back((HANDLE)_beginthreadex(nullptr, 0, &redisThread, this, 0, nullptr));

	mPlayers = new FixedHashMap<sessionID_t, Player*>((size_t)(maxSessionCount * 1.3), HASH_MAP_BUCKET_SIZE, HashSessionId);

	mhWorkerThread = (HANDLE)_beginthreadex(nullptr, 0, &workerThread, this, 0, nullptr);
	mhTimeoutThread = (HANDLE)_beginthreadex(nullptr, 0, &timeoutEventGenerator, this, 0, nullptr);

	if (mhWorkerThread == INVALID_HANDLE_VALUE || mhTimeoutThread == INVALID_HANDLE_VALUE)
	{
		delete mPlayers;

		return false;
	}
	if (!mNetServer->TryRun(IP, port, numWorkerThread, numRunningThread, maxSessionCount, bSupportsNagle, fixed, this))
	{
		gExit = true;
		SetEvent(mhNetworkEvent);

		WaitForSingleObject(mhWorkerThread, INFINITE);
		WaitForSingleObject(mhTimeoutThread, INFINITE);

		delete mPlayers;

		CloseHandle(mhWorkerThread);
		CloseHandle(mhTimeoutThread);

		return false;
	}
	return true;
}

unsigned int ChattingServer::GetTotalLoginSuccessCount() const
{
	return mNumLoginSuccess;
}

unsigned int ChattingServer::GetTotalInvalidSessionKeyCount() const
{
	return mNumSessionKeyNotMatched;
}

unsigned int ChattingServer::GetTotalLoginPacketCount() const
{
	return mNumLoginPacket;
}

unsigned int ChattingServer::GetTotalChattingPacketCount() const
{
	return mNumChatPacket;
}

unsigned int ChattingServer::GetTotalSectorMovePacketCount() const
{
	return mNumSectorMovePacket;
}

unsigned int ChattingServer::GetTotalUpdateCount() const
{
	return mNumUpdate;
}

unsigned int ChattingServer::GetPlayerCount() const
{
	return (unsigned int)mPlayers->GetSize();
}

unsigned int ChattingServer::GetTimeoutPlayer() const
{
	return mNumTimeoutPlayer;
}

unsigned int ChattingServer::GetMessagePoolAllocCount() const
{
	return mContentMessagePool.GetActiveCount();
}

unsigned int ChattingServer::GetPlayerPoolAllocCount() const
{
	return mPlayerPool.GetAllCount();
}

unsigned int ChattingServer::GetMessageQueueSize() const
{
	return (unsigned int)mMessageQueue.GetSize();
}

void ChattingServer::OnError(const int errorCode, const wchar_t* message)
{
	Logger::AppendLine(L"NetworkLib Error::%s, ERROR_CODE:%d", message, errorCode);
	Logger::Log(LOG_FILE_NAME);
}

void ChattingServer::OnRecv(const sessionID_t ID, NetworkLib::Message* message)
{
	message->AddReferenceCount();

	ContentMessage* contentMessage = mContentMessagePool.GetObject();
	{
		contentMessage->SessionID = ID;
		contentMessage->Event = EContentEvent::PacketReceived;
		contentMessage->Payload = message;
	}
	mMessageQueue.Enqueue(contentMessage);

	SetEvent(mhNetworkEvent);
}

bool ChattingServer::OnConnectionRequest(const unsigned long IP, const unsigned short port)
{
	return true;
}

void ChattingServer::OnClientJoin(const sessionID_t ID, const unsigned long IP, const unsigned short port)
{
	ContentMessage* contentMessage = mContentMessagePool.GetObject();
	{
		contentMessage->SessionID = ID;
		contentMessage->Event = EContentEvent::Join;
		contentMessage->Payload = mNetServer->CreateMessage();
		*contentMessage->Payload << IP << port;
	}
	mMessageQueue.Enqueue(contentMessage);

	SetEvent(mhNetworkEvent);
}

void ChattingServer::OnClientLeave(const sessionID_t ID)
{
	ContentMessage* contentMessage = mContentMessagePool.GetObject();
	{
		contentMessage->SessionID = ID;
		contentMessage->Event = EContentEvent::Leave;
		contentMessage->Payload = nullptr;
	}
	mMessageQueue.Enqueue(contentMessage);

	SetEvent(mhNetworkEvent);
}

unsigned int __stdcall ChattingServer::workerThread(void* param)
{
	ChattingServer* server = static_cast<ChattingServer*>(param);

	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);

	const long long loginTimeout = freq.QuadPart * LOGIN_PLAYER_TIMEOUT_SEC;
	const long long unloginTimeout = freq.QuadPart * UNLOGIN_PLAYER_TIMEOUT_SEC;

	while (!gExit)
	{
		ContentMessage* contentMessage;

		while (!server->mMessageQueue.Dequeue(&contentMessage))
		{
			WaitForSingleObject(server->mhNetworkEvent, INFINITE);
		}
		NetworkLib::Message* payload = contentMessage->Payload;
		sessionID_t sessionID = contentMessage->SessionID;

		switch (contentMessage->Event)
		{
		case EContentEvent::Join:
		{
			Player* playerBeforeLogin = server->mPlayerPool.GetObject();
			{
				*payload >> playerBeforeLogin->IP;
				*payload >> playerBeforeLogin->Port;
				playerBeforeLogin->bIsLogin = false;
				playerBeforeLogin->bIsValid = true;
				playerBeforeLogin->SectorX = 0;
				playerBeforeLogin->SectorY = 0;
				playerBeforeLogin->SessionID = sessionID;
				QueryPerformanceCounter(&playerBeforeLogin->LastReceivedTime);
			}
			server->mPlayers->Insert(sessionID, playerBeforeLogin);
		}
		break;
		case EContentEvent::Leave:
		{
			Player* player = server->mPlayers->Find(sessionID);
			player->bIsValid = false;

			std::vector<sessionID_t>& sector = server->mSectors[player->SectorY][player->SectorX];
			size_t numPlayer = sector.size();

			for (size_t i = 0; i < numPlayer; ++i)
			{
				if (sector[i] == sessionID)
				{
					sector[i] = sector[numPlayer - 1];
					sector.pop_back();

					break;
				}
			}
			server->mPlayers->Remove(player->SessionID);

			server->mPlayerPool.ReleaseObject(player);
		}
		break;
		case EContentEvent::PacketReceived:
		{
			WORD packetType;

			*payload >> packetType;

			switch (packetType)
			{
			case EPacketType::PACKET_TYPE_CS_CHAT_REQ_LOGIN:
			{
				server->processLoginPacket(sessionID, *payload);
			}
			break;
			case EPacketType::PACKET_TYPE_CS_CHAT_REQ_SECTOR_MOVE:
			{
				server->processMoveSectorPacket(sessionID, *payload);
			}
			break;
			case EPacketType::PACKET_TYPE_CS_CHAT_REQ_MESSAGE:
			{
				server->processChatPacket(sessionID, *payload);
			}
			break;
			case EPacketType::PACKET_TYPE_CS_CHAT_REQ_HEARTBEAT:
			{
				server->processHeartBeatPacket(sessionID, *payload);
			}
			break;
			default:
				server->mNetServer->TryDisconnect(sessionID);
				break;
			}
		}
		break;
		case EContentEvent::LoginCompletion:
		{
			if (!server->mPlayers->Contains(sessionID))
			{
				break;
			}
			Player* player = server->mPlayers->Find(sessionID);
			int result = 0;
			BYTE res = 0;

			*payload >> result;

			if (result == 0)
			{
				player->bIsLogin = true;
				res = 1;
				++server->mNumLoginSuccess;
			}
			else
			{
				res = 0;
				++server->mNumSessionKeyNotMatched;
			}

			NetworkLib::Message* sendMessage = server->mNetServer->CreateMessage();
			{
				*sendMessage << EPacketType::PACKET_TYPE_SC_CHAT_RES_LOGIN;
				*sendMessage << res;
				*sendMessage << player->AccountNo;

				server->mNetServer->TrySendMessage(sessionID, sendMessage);
			}
			server->mNetServer->ReleaseMessage(sendMessage);
		}
		break;
		case EContentEvent::Timeout:
		{
			LARGE_INTEGER curr;
			QueryPerformanceCounter(&curr);

			auto pairs = server->mPlayers->GetPairs();
			size_t numPlayer = server->mPlayers->GetSize();

			for (size_t i = 0; i < numPlayer; ++i)
			{
				Player* player = pairs[i]->GetValue();

				long long	elapsed = curr.QuadPart - player->LastReceivedTime.QuadPart;
				long long	timeout = player->bIsLogin ? loginTimeout : unloginTimeout;

				if (timeout < elapsed)
				{
					player->bIsValid = false;
					server->mNetServer->TryDisconnect(player->SessionID);

					++server->mNumTimeoutPlayer;
				}
			}
		}
		break;
		default:
			Logger::AppendLine(L"ChattingServer::Unhandled Content Event Error");
			Logger::Log(LOG_FILE_NAME);
			break;
		}
		if (payload != nullptr)
		{
			server->mNetServer->ReleaseMessage(payload);
		}
		server->mContentMessagePool.ReleaseObject(contentMessage);
		++server->mNumUpdate;
	}

	return 0;
}

unsigned int __stdcall ChattingServer::redisThread(void* param)
{
	ChattingServer* server = (ChattingServer*)param;

	while (!gExit)
	{
		//char sessionKeyBuff[SESSION_KEY_LENGTH + 1];
		char accountNoBuff[64];

		ContentMessage* contentMessage;
		DWORD cbTransferred = 0;
		OVERLAPPED overlapped;

		BOOL bSucceded = GetQueuedCompletionStatus(server->mhRedisCompletionPort, &cbTransferred
			, (PULONG_PTR)&contentMessage, (LPOVERLAPPED*)&overlapped, INFINITE);

		if (!bSucceded)
		{
			Logger::AppendLine(L"Redis Thread GQCS Error : %d", GetLastError());
			Logger::Log(LOG_FILE_NAME);
		}
		INT64 accountNo;

		*contentMessage->Payload >> accountNo;
		//contentMessage->Payload->Read(sessionKeyBuff, SESSION_KEY_LENGTH);

		//sessionKeyBuff[SESSION_KEY_LENGTH] = '\0';

		sprintf_s(accountNoBuff, "%lld", accountNo);

		server->mRedis.get(accountNoBuff, [server, contentMessage](cpp_redis::reply& reply) {
			char sessionKeyBuff[SESSION_KEY_LENGTH + 1];
			int result = 0;

			sessionKeyBuff[SESSION_KEY_LENGTH] = '\0';
			contentMessage->Payload->Read(sessionKeyBuff, SESSION_KEY_LENGTH);

			contentMessage->Event = EContentEvent::LoginCompletion;

			if (!reply.is_null())
			{
				result = reply.as_string().compare(sessionKeyBuff);
			}
			*contentMessage->Payload << result;

			server->mMessageQueue.Enqueue(contentMessage);
			});
		server->mRedis.sync_commit();
		/*std::string result;
		
		if (reply.valid() && !reply.get().is_null())
		{
			result = reply.get().as_string();
		}
		contentMessage->Event = EContentEvent::LoginCompletion;
		*contentMessage->Payload << result.compare(sessionKeyBuff);
		
		server->mMessageQueue.Enqueue(contentMessage);*/
	}
	return 0;
}

unsigned int __stdcall ChattingServer::timeoutEventGenerator(void* param)
{
	ChattingServer* server = (ChattingServer*)param;

	while (!gExit)
	{
		Sleep(TIMEOUT_EVENT_PERIOD_MS);

		ContentMessage* contentMessage = server->mContentMessagePool.GetObject();
		{
			contentMessage->Event = EContentEvent::Timeout;
			contentMessage->Payload = nullptr;
		}
		server->mMessageQueue.Enqueue(contentMessage);

		SetEvent(server->mhNetworkEvent);
	}
	return 0;
}

void ChattingServer::sendToSector(std::vector<sessionID_t>& sector, NetworkLib::Message& message)
{
	for (sessionID_t id : sector)
	{
		mNetServer->TrySendMessage(id, &message);
	}
}

void ChattingServer::sendToSectorRange(WORD x, WORD y, NetworkLib::Message& message)
{
	sendToSector(mSectors[y][x], message);
	sendToSector(mSectors[y][x - 1], message);
	sendToSector(mSectors[y - 1][x - 1], message);
	sendToSector(mSectors[y - 1][x], message);
	sendToSector(mSectors[y - 1][x + 1], message);
	sendToSector(mSectors[y][x + 1], message);
	sendToSector(mSectors[y + 1][x + 1], message);
	sendToSector(mSectors[y + 1][x], message);
	sendToSector(mSectors[y + 1][x - 1], message);
}

void ChattingServer::processLoginPacket(sessionID_t id, NetworkLib::Message& message)
{
	Player* player = mPlayers->Find(id);
	
	if (!player->bIsValid)
	{
		return;
	}
	if (player->bIsLogin)
	{
		player->bIsValid = false;
		mNetServer->TryDisconnect(id);
		return;
	}
	char sessionKey[SESSION_KEY_LENGTH + 1];

	sessionKey[SESSION_KEY_LENGTH] = '\0';

	message >> player->AccountNo;
	message.Read((char*)player->ID, sizeof(player->ID));
	message.Read((char*)player->Nickname, sizeof(player->Nickname));
	message.Read(sessionKey, sizeof(char) * SESSION_KEY_LENGTH);

	ContentMessage* contentMessage = mContentMessagePool.GetObject();
	{
		contentMessage->SessionID = id;

		NetworkLib::Message* redisMessage = mNetServer->CreateMessage();
		{
			*redisMessage << player->AccountNo;
			redisMessage->Write(sessionKey, sizeof(char) * SESSION_KEY_LENGTH);
		}
		contentMessage->Payload = redisMessage;
	}
	PostQueuedCompletionStatus(mhRedisCompletionPort, 0, (ULONG_PTR)contentMessage, (LPOVERLAPPED)nullptr);

	QueryPerformanceCounter(&player->LastReceivedTime);
	++mNumLoginPacket;
}

void ChattingServer::processMoveSectorPacket(sessionID_t id, NetworkLib::Message& message)
{
	Player* player = mPlayers->Find(id);

	if (!player->bIsValid)
	{
		return;
	}
	if (!player->bIsLogin)
	{
		player->bIsValid = false;
		mNetServer->TryDisconnect(id);
		return;
	}
	INT64 accountNo;
	WORD toX;
	WORD toY;

	message >> accountNo;
	message >> toX;
	message >> toY;

	if (SECTOR_COLUMN <= toX || SECTOR_ROW <= toY || accountNo != player->AccountNo)
	{
		player->bIsValid = false;
		mNetServer->TryDisconnect(id);
		return;
	}
	toX += 1;
	toY += 1;

	std::vector<sessionID_t>& sector = mSectors[player->SectorY][player->SectorX];
	size_t numPlayer = sector.size();

	for (size_t i = 0; i < numPlayer; ++i)
	{
		if (sector[i] == id)
		{
			sector[i] = sector[numPlayer - 1];
			sector.pop_back();

			break;
		}
	}
	player->SectorX = toX;
	player->SectorY = toY;

	mSectors[player->SectorY][player->SectorX].push_back(id);

	NetworkLib::Message* sendMessage = mNetServer->CreateMessage();
	{
		*sendMessage << EPacketType::PACKET_TYPE_SC_CHAT_RES_SECTOR_MOVE;
		*sendMessage << player->AccountNo;
		*sendMessage << player->SectorX - 1;
		*sendMessage << player->SectorY - 1;

		mNetServer->TrySendMessage(id, sendMessage);
	}
	mNetServer->ReleaseMessage(sendMessage);

	QueryPerformanceCounter(&player->LastReceivedTime);
	++mNumSectorMovePacket;
}

void ChattingServer::processChatPacket(sessionID_t id, NetworkLib::Message& message)
{
	Player* player = mPlayers->Find(id);

	if (!player->bIsValid)
	{
		return;
	}
	if (!player->bIsLogin)
	{
		player->bIsValid = false;
		mNetServer->TryDisconnect(id);
		return;
	}
	INT64 accountNo;
	WORD messageLength;

	message >> accountNo;
	message >> messageLength;

	if (MAX_CHAT_LENGTH < messageLength || accountNo != player->AccountNo)
	{
		player->bIsValid = false;
		mNetServer->TryDisconnect(id);
		return;
	}
	WORD sectorX = player->SectorX;
	WORD sectorY = player->SectorY;

	NetworkLib::Message* sendMessage = mNetServer->CreateMessage();
	{
		*sendMessage << EPacketType::PACKET_TYPE_SC_CHAT_RES_MESSAGE;
		*sendMessage << accountNo;
		sendMessage->Write((char*)player->ID, sizeof(player->ID));
		sendMessage->Write((char*)player->Nickname, sizeof(player->Nickname));
		*sendMessage << messageLength;
		sendMessage->Write(message.GetFront(), messageLength);

		sendToSectorRange(sectorX, sectorY, *sendMessage);
	}
	mNetServer->ReleaseMessage(sendMessage);

	QueryPerformanceCounter(&player->LastReceivedTime);
	++mNumChatPacket;
}

void ChattingServer::processHeartBeatPacket(sessionID_t id, NetworkLib::Message& message)
{
	Player* player = mPlayers->Find(id);

	if (!player->bIsValid)
	{
		return;
	}
	if (!player->bIsLogin)
	{
		player->bIsValid = false;
		mNetServer->TryDisconnect(id);
		return;
	}
	QueryPerformanceCounter(&player->LastReceivedTime);
}
