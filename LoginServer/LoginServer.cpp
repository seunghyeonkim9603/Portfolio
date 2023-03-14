#pragma warning(disable : 4996 6031)
#pragma comment(lib, "NetworkLib.lib")
#pragma comment (lib, "cpp_redis.lib")
#pragma comment (lib, "tacopie.lib")

#include "NetworkLib/NetworkLib.h"

#include <cpp_redis/cpp_redis>
#include <codecvt>
#include <string>

#include "mysql.h"
#include "errmsg.h"

#include "Logger.h"
#include "DBConnector.h"
#include "CommonProtocol.h"
#include "LoginServer.h"

bool gExit = false;

#define LOG_FILE_NAME (L"LoginServerLog.txt")

thread_local DBConnector LoginServer::DbConnector;

LoginServer::LoginServer(NetworkLib::WANServer* server, const WCHAR* gameServerIP, USHORT gameServerPort, const WCHAR* chatServerIP, USHORT chatServerPort)
	: mNetServer(server),
	mNumAuthCompletedSession(0),
	mNumSessionBeforeLogin(0),
	mGameServerPort(gameServerPort),
	mChatServerPort(chatServerPort)
{
	InitializeSRWLock(&mUsersLock);
	lstrcpyW(mGameServerIP, gameServerIP);
	lstrcpyW(mChatServerIP, chatServerIP);
}

LoginServer::~LoginServer()
{
	gExit = true;

	WaitForSingleObject(mhDisconnectThread, INFINITE);
	CloseHandle(mhDisconnectThread);

	for (auto iter : mUsers)
	{
		User* user = iter.second;

		mUserPool.ReleaseObject(user);
	}
}

bool LoginServer::TryRun(const unsigned long IP, const unsigned short port, const unsigned int numWorkerThread, const unsigned int numRunningThread, const unsigned int maxSessionCount, const bool bSupportsNagle)
{
	mRedis.connect();

	mhDisconnectThread = (HANDLE)_beginthreadex(nullptr, 0, &disconnectThread, this, 0, nullptr);

	if (mhDisconnectThread == INVALID_HANDLE_VALUE)
	{
		return false;
	}
	if (!mNetServer->TryRun(IP, port, numWorkerThread, numRunningThread, maxSessionCount, bSupportsNagle, 0x32, this))
	{
		return false;
	}
	return true;
}

unsigned int LoginServer::GetNumAuthCompeltedSession() const
{
	return mNumAuthCompletedSession;
}

unsigned int LoginServer::GetNumAuthFailedSession() const
{
	return mNumAuthFailedSession;
}

unsigned int LoginServer::GetNumSessionBeforeLogin() const
{
	return mNumSessionBeforeLogin;
}

unsigned int LoginServer::GetNumUsers() const
{
	return (unsigned int)mUsers.size();
}

unsigned int LoginServer::GetUserPoolSize() const
{
	return mUserPool.GetAllCount();
}


void LoginServer::OnError(const int errorCode, const wchar_t* message)
{
	Logger::AppendLine(L"NetworkLib Error::%s, ERROR_CODE:%d", message, errorCode);
	Logger::Log(LOG_FILE_NAME);
}

void LoginServer::OnRecv(const sessionID_t ID, NetworkLib::Message* message)
{
	User* user;
	
	if (!DbConnector.IsConnected())
	{
		if (!DbConnector.TryConnect("127.0.0.1", "root", "123456", "accountdb", 3306))
		{
			std::cout << "db connection Fail" << std::endl;
			return;
		}
	}

	AcquireSRWLockShared(&mUsersLock);
	{
		user = mUsers.find(ID)->second;
	}
	ReleaseSRWLockShared(&mUsersLock);

	if (user->bIsRequested)
	{
		return;
	}
	user->bIsRequested = true;

	WORD	type;
	INT64	accountNo;
	char	sessionKey[SESSION_KEY_LENGTH + 1];

	sessionKey[SESSION_KEY_LENGTH] = '\0';

	*message >> type;
	*message >> accountNo;
	message->Read(sessionKey, SESSION_KEY_LENGTH);

	if (type != en_PACKET_CS_LOGIN_REQ_LOGIN)
	{
		InterlockedDecrement(&mNumSessionBeforeLogin);
		mNetServer->TryDisconnect(ID);

		return;
	}
	MYSQL_ROW sqlRow;
	BYTE resStatus;
	BYTE curStatus;

	if (!DbConnector.TryRequestQueryResult("SELECT * FROM status WHERE accountno = %lld", accountNo))
	{
		resStatus = dfLOGIN_STATUS_STATUS_MISS;

		goto SEND_PACKET;
	}

	sqlRow = DbConnector.FetchRowOrNull();
	{
		sscanf(sqlRow[1], "%c", &curStatus);

		if (curStatus == dfLOGIN_STATUS_GAME)
		{
			resStatus = dfLOGIN_STATUS_GAME;

			goto SEND_PACKET;
		}
	}
	DbConnector.FreeResult();

	if (!DbConnector.TryRequestQueryResult("SELECT * FROM sessionkey WHERE accountno = %lld", accountNo))
	{
		resStatus = dfLOGIN_STATUS_SESSION_MISS;

		goto SEND_PACKET;
	}
	DbConnector.FreeResult();

	if (!DbConnector.TryRequestQueryResult("SELECT * FROM account WHERE accountno = %lld", accountNo))
	{
		resStatus = dfLOGIN_STATUS_ACCOUNT_MISS;

		goto SEND_PACKET;
	}
	WCHAR userID[USER_ID_LENGTH];
	WCHAR userNickname[USER_NICKNAME_LENGTH];

	sqlRow = DbConnector.FetchRowOrNull();
	{
		MultiByteToWideChar(CP_ACP, 0, sqlRow[1], USER_ID_LENGTH + 1, userID, USER_ID_LENGTH);
		MultiByteToWideChar(CP_ACP, 0, sqlRow[3], USER_NICKNAME_LENGTH + 1, userNickname, USER_NICKNAME_LENGTH);
	}
	DbConnector.FreeResult();
	resStatus = dfLOGIN_STATUS_OK;

	//sprintf(queryString, "UPDATE status SET status = %d WHERE accountno = %lld", dfLOGIN_STATUS_OK, accountNo);

	/*if (!DbConnector.TryRequestQuery(queryString))
	{
		resStatus = dfLOGIN_STATUS_FAIL;

		goto SEND_PACKET;
	}*/
	char accountNoBuff[ACCOUNT_NO_BUFF_LENGTH];

	sprintf(accountNoBuff, "%lld", accountNo);
	
	mRedis.setex(accountNoBuff, REDIS_DURATION_TIME_SEC, sessionKey);
	mRedis.sync_commit();

SEND_PACKET:

	if (resStatus == dfLOGIN_STATUS_OK)
	{
		InterlockedIncrement(&mNumAuthCompletedSession);
	}
	else
	{
		InterlockedIncrement(&mNumAuthFailedSession);
	}

	NetworkLib::Message* sendMessage = mNetServer->CreateMessage();
	{
		*sendMessage << (WORD)en_PACKET_CS_LOGIN_RES_LOGIN;
		*sendMessage << accountNo;
		*sendMessage << resStatus;

		sendMessage->Write((char*)userID, sizeof(userID));
		sendMessage->Write((char*)userNickname, sizeof(userNickname));

		sendMessage->Write((char*)mGameServerIP, sizeof(mGameServerIP));
		*sendMessage << mGameServerPort;
		sendMessage->Write((char*)mChatServerIP, sizeof(mChatServerIP));
		*sendMessage << mChatServerPort;
		
		QueryPerformanceCounter(&user->AccessTime);

		mNetServer->TrySendMessage(ID, sendMessage);
	}
	mNetServer->ReleaseMessage(sendMessage);

	InterlockedDecrement(&mNumSessionBeforeLogin);
}

bool LoginServer::OnConnectionRequest(const unsigned long IP, const unsigned short port)
{
	return true;
}

void LoginServer::OnClientJoin(const sessionID_t ID, const unsigned long IP, const unsigned short port)
{
	User* user = mUserPool.GetObject();
	{
		user->IP = IP;
		user->Port = port;
		user->SessionID = ID;
		user->bIsRequested = false;
		QueryPerformanceCounter(&user->AccessTime);
	}
	AcquireSRWLockExclusive(&mUsersLock);
	mUsers.insert({ ID, user });
	ReleaseSRWLockExclusive(&mUsersLock);

	InterlockedIncrement(&mNumSessionBeforeLogin);
}

void LoginServer::OnClientLeave(const sessionID_t ID)
{
	User* user;

	AcquireSRWLockShared(&mUsersLock);
	{
		user = mUsers.find(ID)->second;
	}
	ReleaseSRWLockShared(&mUsersLock);

	AcquireSRWLockExclusive(&mUsersLock);
	{
		mUsers.erase(ID);
	}
	ReleaseSRWLockExclusive(&mUsersLock);

	mUserPool.ReleaseObject(user);
}


unsigned int __stdcall LoginServer::disconnectThread(void* param)
{
	LoginServer* server = (LoginServer*)param;

	std::vector<sessionID_t> disconnectedSession;

	LARGE_INTEGER freq;
	LARGE_INTEGER cur;

	QueryPerformanceFrequency(&freq);
	
	while (!gExit)
	{
		Sleep(DISCONNECT_PERIOD_TIME_MS);

		QueryPerformanceCounter(&cur);

		AcquireSRWLockShared(&server->mUsersLock);
		{
			for (auto iter : server->mUsers)
			{
				User* user = iter.second;
				if (freq.QuadPart * DISCONNECT_TIME_SEC_AFTER_RES < cur.QuadPart - user->AccessTime.QuadPart)
				{
					disconnectedSession.push_back(iter.first);
				}
			}
		}
		ReleaseSRWLockShared(&server->mUsersLock);

		for (sessionID_t id : disconnectedSession)
		{
			server->mNetServer->TryDisconnect(id);
		}
		disconnectedSession.clear();
	}
}