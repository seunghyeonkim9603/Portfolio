#include "stdafx.h"

#define LOG_FILE_NAME (L"LoginServerMonitorClientLog.txt")

static bool gExit;

MonitoringClient::MonitoringClient(NetworkLib::LANClient* lanClient, int serverNo)
	: mLANClient(lanClient),
	mServerNo(serverNo)
{
}

MonitoringClient::~MonitoringClient()
{
}

bool MonitoringClient::TryRun(const unsigned long IP, const unsigned short port, const bool bSupportsNagle)
{
	if (!mLANClient->TryRun(IP, port, 2, 2, true, this))
	{
		int a = WSAGetLastError();
		return false;
	}
	return true;
}

void MonitoringClient::Monitor(BYTE type, int value)
{
	NetworkLib::Message* message = mLANClient->CreateMessage();
	{
		*message << (WORD)en_PACKET_TYPE::en_PACKET_SS_MONITOR_DATA_UPDATE;
		*message << type;
		*message << value;
		*message << (int)time(nullptr);

		mLANClient->TrySendMessage(message);
	}
	mLANClient->ReleaseMessage(message);
}

void MonitoringClient::OnError(const int errorCode, const wchar_t* message)
{
	Logger::AppendLine(L"NetworkLib Error::%s, ERROR_CODE:%d", message, errorCode);
	Logger::Log(LOG_FILE_NAME);
}

void MonitoringClient::OnRecv(NetworkLib::Message* message)
{
}

void MonitoringClient::OnConnect()
{
	NetworkLib::Message* message = mLANClient->CreateMessage();
	{
		*message << (WORD)en_PACKET_TYPE::en_PACKET_SS_MONITOR_LOGIN;
		*message << (int)mServerNo;

		mLANClient->TrySendMessage(message);
	}
	mLANClient->ReleaseMessage(message);
}

void MonitoringClient::OnDisconnect()
{
}

