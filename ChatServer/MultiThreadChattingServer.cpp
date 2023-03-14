#include "stdafx.h"
#include "EPacketType.h"

#include "MultiThreadChattingServer.h"

thread_local std::vector<sessionID_t> gIdVector;
bool gExit;

MultiThreadChattingServer::MultiThreadChattingServer(WanServer* server)
	: mNetServer(server),
	mNumChatPacket(0),
	mNumLoginPacket(0),
	mNumSectorMovePacket(0),
	mNumUpdate(0)
{
	InitializeSRWLock(&mPlayersLock);

	for (int i = 0; i < SECTOR_ROW + 2; ++i)
	{
		for (int j = 0; j < SECTOR_COLUMN + 2; ++j)
		{
			InitializeSRWLock(&mSectors[i][j].Lock);
		}
	}
}

MultiThreadChattingServer::~MultiThreadChattingServer()
{
}

bool MultiThreadChattingServer::TryRun(const unsigned long IP, const unsigned short port, const unsigned int numWorkerThread, const unsigned int numRunningThread, const unsigned int maxSessionCount, const bool bSupportsNagle)
{
	mhTimeoutThread = (HANDLE)_beginthreadex(nullptr, 0, &timeoutThread, this, 0, nullptr);

	if (!mNetServer->TryRun(IP, port, numWorkerThread, numRunningThread, maxSessionCount, bSupportsNagle, this))
	{
		gExit = true;

		WaitForSingleObject(mhTimeoutThread, INFINITE);
		CloseHandle(mhTimeoutThread);
		return false;
	}
	return true;
}

unsigned int MultiThreadChattingServer::GetTotalLoginPacketCount() const
{
	return mNumLoginPacket;
}

unsigned int MultiThreadChattingServer::GetTotalChattingPacketCount() const
{
	return mNumChatPacket;
}

unsigned int MultiThreadChattingServer::GetTotalSectorMovePacketCount() const
{
	return mNumSectorMovePacket;
}

unsigned int MultiThreadChattingServer::GetTotalUpdateCount() const
{
	return mNumUpdate;
}

unsigned int MultiThreadChattingServer::GetPlayerCount() const
{
	return mPlayers.size();
}

unsigned int MultiThreadChattingServer::GetPlayerPoolAllocCount() const
{
	return mPlayerPool.GetActiveCount();;
}

void MultiThreadChattingServer::OnError(const int errorCode, const wchar_t* message)
{
}

void MultiThreadChattingServer::OnRecv(const sessionID_t ID, Message* message)
{
	AcquireSRWLockShared(&mPlayersLock);
	Player* player = mPlayers.find(ID)->second;
	ReleaseSRWLockShared(&mPlayersLock);

	if (!player->bIsValid)
	{
		return;
	}
	WORD packetType;

	*message >> packetType;

	switch (packetType)
	{
	case EPacketType::PACKET_TYPE_CS_CHAT_REQ_LOGIN:
	{
		processLoginPacket(player, *message);
	}
	break;
	case EPacketType::PACKET_TYPE_CS_CHAT_REQ_SECTOR_MOVE:
	{
		processMoveSectorPacket(player, *message);
	}
	break;
	case EPacketType::PACKET_TYPE_CS_CHAT_REQ_MESSAGE:
	{
		processChatPacket(player, *message);
	}
	break;
	case EPacketType::PACKET_TYPE_CS_CHAT_REQ_HEARTBEAT:
	{
		processHeartBeatPacket(player, *message);
	}
	break;
	default:
		mNetServer->TryDisconnect(ID);
		break;
	}
}

bool MultiThreadChattingServer::OnConnectionRequest(const unsigned long IP, const unsigned short port)
{
	return true;
}

void MultiThreadChattingServer::OnClientJoin(const sessionID_t ID, const unsigned long IP, const unsigned short port)
{
	Player* playerBeforeLogin = mPlayerPool.GetObject();
	{
		playerBeforeLogin->IP = IP;
		playerBeforeLogin->Port = port;
		playerBeforeLogin->SectorX = 0;
		playerBeforeLogin->SectorY = 0;
		playerBeforeLogin->SessionID = ID;
		playerBeforeLogin->bIsValid = true;
		playerBeforeLogin->bIsLogin = false;
		playerBeforeLogin->SectorX = 0;
		playerBeforeLogin->SectorY = 0;
		QueryPerformanceCounter(&playerBeforeLogin->LastReceivedTime);
	}
	AcquireSRWLockExclusive(&mPlayersLock);
	mPlayers.insert({ ID, playerBeforeLogin });
	ReleaseSRWLockExclusive(&mPlayersLock);
}

void MultiThreadChattingServer::OnClientLeave(const sessionID_t ID)
{
	Sector* sector;
	Player* leavedPlayer;

	AcquireSRWLockExclusive(&mPlayersLock);
	{
		auto iter = mPlayers.find(ID);

		leavedPlayer = iter->second;
		sector = &mSectors[leavedPlayer->SectorY][leavedPlayer->SectorX];

		mPlayers.erase(ID);
	}
	ReleaseSRWLockExclusive(&mPlayersLock);

	AcquireSRWLockExclusive(&sector->Lock);
	sector->Players.erase(ID);
	ReleaseSRWLockExclusive(&sector->Lock);

	mPlayerPool.ReleaseObject(leavedPlayer);
}

unsigned int __stdcall MultiThreadChattingServer::timeoutThread(void* param)
{
	MultiThreadChattingServer* server = (MultiThreadChattingServer*)param;

	LARGE_INTEGER curr;
	LARGE_INTEGER freq;

	QueryPerformanceFrequency(&freq);

	while (!gExit)
	{
		AcquireSRWLockShared(&server->mPlayersLock);
		{
			QueryPerformanceCounter(&curr);

			for (auto iter : server->mPlayers)
			{
				Player* player = iter.second;

				if (player->bIsLogin)
				{
					if (LOGIN_PLAYER_TIMEOUT_SEC * freq.QuadPart < curr.QuadPart - player->LastReceivedTime.QuadPart)
					{
						server->mNetServer->TryDisconnect(player->SessionID);
					}
				}
				else
				{
					if (UNLOGIN_PLAYER_TIMEOUT_SEC * freq.QuadPart < curr.QuadPart - player->LastReceivedTime.QuadPart)
					{
						server->mNetServer->TryDisconnect(player->SessionID);
					}
				}
			}
		}
		ReleaseSRWLockShared(&server->mPlayersLock);

		Sleep(TIMEOUT_EVENT_PERIOD_MS);
	}
	return 0;
}

void MultiThreadChattingServer::sendToSector(Sector& sector, Message& message)
{
	AcquireSRWLockShared(&sector.Lock);
	{
		for (auto iter : sector.Players)
		{
			gIdVector.push_back(iter.first);
		}
	}
	ReleaseSRWLockShared(&sector.Lock);

	for (sessionID_t id : gIdVector)
	{
		mNetServer->TrySendMessage(id, &message);
	}
	gIdVector.clear();
}

void MultiThreadChattingServer::sendToSectorRange(WORD x, WORD y, Message& message)
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


void MultiThreadChattingServer::processLoginPacket(Player* player, Message& message)
{
	if (player->bIsLogin)
	{
		player->bIsValid = false;
		mNetServer->TryDisconnect(player->SessionID);
		return;
	}
	message >> player->AccountNo;
	message.Read((char*)player->ID, sizeof(player->ID));
	message.Read((char*)player->Nickname, sizeof(player->Nickname));

	player->bIsLogin = true;

	Message* sendMessage = mNetServer->CreateMessage();
	{
		*sendMessage << EPacketType::PACKET_TYPE_SC_CHAT_RES_LOGIN;
		*sendMessage << (BYTE)1;
		*sendMessage << player->AccountNo;

		mNetServer->TrySendMessage(player->SessionID, sendMessage);
	}
	mNetServer->ReleaseMessage(sendMessage);

	QueryPerformanceCounter(&player->LastReceivedTime);
	InterlockedIncrement(&mNumLoginPacket);
}

void MultiThreadChattingServer::processMoveSectorPacket(Player* player, Message& message)
{
	if (!player->bIsLogin)
	{
		player->bIsValid = false;
		mNetServer->TryDisconnect(player->SessionID);
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
		mNetServer->TryDisconnect(player->SessionID);
		return;
	}
	toX += 1;
	toY += 1;

	Sector* toSector = &mSectors[toY][toX];
	Sector* fromSector = &mSectors[player->SectorY][player->SectorX];

	if (toSector != fromSector)
	{
		while (true)
		{
			if (TryAcquireSRWLockExclusive(&fromSector->Lock))
			{
				if (TryAcquireSRWLockExclusive(&toSector->Lock))
				{
					break;
				}
				ReleaseSRWLockExclusive(&fromSector->Lock);
			}
		}
		fromSector->Players.erase(player->SessionID);
		toSector->Players.insert({ player->SessionID, player });

		ReleaseSRWLockExclusive(&toSector->Lock);
		ReleaseSRWLockExclusive(&fromSector->Lock);
	}
	player->SectorX = toX;
	player->SectorY = toY;

	Message* sendMessage = mNetServer->CreateMessage();
	{
		*sendMessage << EPacketType::PACKET_TYPE_SC_CHAT_RES_SECTOR_MOVE;
		*sendMessage << player->AccountNo;
		*sendMessage << player->SectorX - 1;
		*sendMessage << player->SectorY - 1;

		mNetServer->TrySendMessage(player->SessionID, sendMessage);
	}
	mNetServer->ReleaseMessage(sendMessage);

	QueryPerformanceCounter(&player->LastReceivedTime);
	InterlockedIncrement(&mNumSectorMovePacket);
}

void MultiThreadChattingServer::processChatPacket(Player* player, Message& message)
{
	if (!player->bIsLogin)
	{
		player->bIsValid = false;
		mNetServer->TryDisconnect(player->SessionID);
		return;
	}
	INT64 accountNo;
	WORD messageLength;

	message >> accountNo;
	message >> messageLength;

	if (MAX_CHAT_LENGTH < messageLength || accountNo != player->AccountNo)
	{
		player->bIsValid = false;
		mNetServer->TryDisconnect(player->SessionID);
		return;
	}
	WORD sectorX = player->SectorX;
	WORD sectorY = player->SectorY;

	Message* sendMessage = mNetServer->CreateMessage();
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
	InterlockedIncrement(&mNumChatPacket);
}

void MultiThreadChattingServer::processHeartBeatPacket(Player* player, Message& message)
{
	if (!player->bIsLogin)
	{
		player->bIsValid = false;
		mNetServer->TryDisconnect(mNetServer->TryDisconnect(player->SessionID));
		return;
	}
	QueryPerformanceCounter(&player->LastReceivedTime);
}
