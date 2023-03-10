#include "stdafx.h"

#define LOG_FILE_NAME (L"NetEventListenerLog.txt")

NetEventListener::NetEventListener(NetworkLib::WANServer* server)
	: mNetServer(server)
{
}

NetEventListener::~NetEventListener()
{
	mNetServer->Terminate();
}

bool NetEventListener::TryRun(const unsigned long IP, const unsigned short port, const unsigned int numWorkerThread, const unsigned int numRunningThread, const unsigned int maxSessionCount, const bool bSupportsNagle, unsigned char fixed)
{
	if (!mNetServer->TryRun(IP, port, numWorkerThread, numRunningThread, maxSessionCount, bSupportsNagle, fixed, this))
	{
		return false;
	}
	return true;
}

void NetEventListener::OnError(const int errorCode, const wchar_t* message)
{
	Logger::AppendLine(L"NetworkLib Error::%s, ERROR_CODE:%d", message, errorCode);
	Logger::Log(LOG_FILE_NAME);
}

bool NetEventListener::OnConnectionRequest(const unsigned long IP, const unsigned short port)
{
	return true;
}

