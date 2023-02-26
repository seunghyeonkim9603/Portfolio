#include "stdafx.h"

#define LOG_FILE_NAME (L"GameServerLog.txt")

GameServer::GameServer(NetworkLib::WANServer* server)
	: mNetServer(server)
{
}

GameServer::~GameServer()
{
	mNetServer->Terminate();
}

bool GameServer::TryRun(const unsigned long IP, const unsigned short port, const unsigned int numWorkerThread, const unsigned int numRunningThread, const unsigned int maxSessionCount, const bool bSupportsNagle, unsigned char fixed)
{
	if (!mNetServer->TryRun(IP, port, numWorkerThread, numRunningThread, maxSessionCount, bSupportsNagle, fixed, this))
	{
		return false;
	}
	return true;
}

void GameServer::OnError(const int errorCode, const wchar_t* message)
{
	Logger::AppendLine(L"NetworkLib Error::%s, ERROR_CODE:%d", message, errorCode);
	Logger::Log(LOG_FILE_NAME);
}

void GameServer::OnRecv(const sessionID_t ID, NetworkLib::Message* message)
{
}

bool GameServer::OnConnectionRequest(const unsigned long IP, const unsigned short port)
{
	return true;
}

void GameServer::OnClientJoin(const sessionID_t ID, const unsigned long IP, const unsigned short port)
{
}

void GameServer::OnClientLeave(const sessionID_t ID)
{
}

