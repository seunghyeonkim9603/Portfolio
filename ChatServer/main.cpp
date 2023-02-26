#pragma comment(lib, "NetworkLib.lib")
#pragma comment(lib, "pdh.lib")

#include "NetworkLib/NetworkLib.h"

#include <Pdh.h>
#include <Psapi.h>

#include "ProcessTaskManager.h"
#include "Profiler.h"
#include "ProfilerManager.h"

#include "CCrashDump.h"
#include "Logger.h"

#include "Pair.h"
#include "FixedHashMap.h"
#include "EPacketType.h"
#include "ChattingServer.h"
#include "MonitoringClient.h"
#include "ProcessDataHelper.h"
#include "MonitorProtocol.h"

#define CHAT_SERVER_PORT (11802)
#define MONITOR_SERVER_PORT (11850)

#define SERVER_NO (1)
#define MAX_PROCESS_NAME (128)

static size_t Hash64(INT64 id)
{
	id ^= id >> 32;
	id *= 0xd6e8feb86659fd93U;
	id ^= id >> 32;
	id *= 0xd6e8feb86659fd93U;
	id ^= id >> 32;

	return id;
}

void RunChatServer()
{
	wchar_t processPath[MAX_PROCESS_NAME];

	GetModuleFileNameW(nullptr, processPath, MAX_PROCESS_NAME);

	wchar_t* processName = wcsrchr(processPath, L'\\') + 1;
	wchar_t* extention = wcsrchr(processName, L'.');

	*extention = L'\0';

	unsigned int numTotalAccept = 0;
	unsigned int numTotalAcceptBefore = 0;
	unsigned int numTotalUpdate = 0;
	unsigned int numTotalUpdateBefore = 0;
	unsigned int numTotalRecv = 0;
	unsigned int numTotalRecvBefore = 0;
	unsigned int numTotalSend = 0;
	unsigned int numTotalSendBefore = 0;

	int cpuUsage;
	int privateBytes;

	NetworkLib::WANServer* wanServer = new NetworkLib::WANServer();
	NetworkLib::LANClient* lanClient = new NetworkLib::LANClient();

	ChattingServer chatServer(wanServer);
	MonitoringClient monitor(lanClient, SERVER_NO);
	ProcessDataHelper pdh(processName);
	ProcessTaskManager taskManager;


	if (!chatServer.TryRun(INADDR_ANY, CHAT_SERVER_PORT, 8, 4, 20000, true, 0x32))
	{
		std::cout << "Failed Run Chat Server" << std::endl;
		return;
	}
	if (!monitor.TryRun(INADDR_LOOPBACK, MONITOR_SERVER_PORT, true))
	{
		std::cout << "Faild Run Monitoring Client" << std::endl;
		return;
	}
	if (!pdh.TryInit())
	{
		std::cout << "Failed Init PDH" << std::endl;
		return;
	}

	while (true)
	{
		pdh.Update();
		taskManager.UpdateProcessInfo();

		numTotalUpdate = chatServer.GetTotalUpdateCount();
		numTotalAccept = wanServer->GetNumAccept();
		numTotalRecv = wanServer->GetNumRecv();
		numTotalSend = wanServer->GetNumSend();

		cpuUsage = (int)taskManager.GetProcessTotalUsage();
		privateBytes = (int)pdh.GetPrivateBytes();

		monitor.Monitor(dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL, (int)wanServer->GetMessagePoolActiveCount());
		monitor.Monitor(dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN, 1);
		monitor.Monitor(dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU, cpuUsage);
		monitor.Monitor(dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM, privateBytes);
		monitor.Monitor(dfMONITOR_DATA_TYPE_CHAT_SESSION, (int)wanServer->GetCurrentSessionCount());
		monitor.Monitor(dfMONITOR_DATA_TYPE_CHAT_PLAYER, (int)chatServer.GetPlayerCount());
		monitor.Monitor(dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS, (int)numTotalUpdate - numTotalUpdateBefore);
		monitor.Monitor(dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL, (int)chatServer.GetMessagePoolAllocCount());

		std::cout << "======================================================" << std::endl;
		std::cout << "Session Count : " << wanServer->GetCurrentSessionCount() << std::endl;
		std::cout << "Active Content Message Count : " << chatServer.GetMessagePoolAllocCount() << std::endl;
		std::cout << "Message Queue Size : " << chatServer.GetMessageQueueSize() << std::endl;
		std::cout << "Player Pool Alloc : " << chatServer.GetPlayerPoolAllocCount() << std::endl;
		std::cout << "Player Count : " << chatServer.GetPlayerCount() << std::endl;
		std::cout << "Timeout Player Count : " << chatServer.GetTimeoutPlayer() << std::endl;
		std::cout << "Active Packet Count : " << wanServer->GetMessagePoolActiveCount() << std::endl;
		std::cout << "Request Disconnect Count : " << wanServer->GetNumRequestDisconnected() << std::endl;
		std::cout << "Send Queue Full Disconnect Count : " << wanServer->GetNumSendQueueFullDisconnected() << std::endl;
		std::cout << "Limit Session Disconnect Count : " << wanServer->GetNumLimitSessionDisconnected() << std::endl;
		std::cout << "Invalid Session Disconnect Count : " << wanServer->GetNumInvalidSessionDisconnected() << std::endl << std::endl;

		std::cout << "Accept Total : " << numTotalAccept << std::endl;
		std::cout << "Accept TPS : " << numTotalAccept - numTotalAcceptBefore << std::endl;
		std::cout << "Update TPS : " << numTotalUpdate - numTotalUpdateBefore << std::endl;
		std::cout << "Recv TPS : " << numTotalRecv - numTotalRecvBefore << std::endl;
		std::cout << "Send TPS : " << numTotalSend - numTotalSendBefore << std::endl;
		std::cout << "TCP Retransmitted Packet/sec : " << pdh.GetNumRetransmittedPacket() << std::endl << std::endl;

		numTotalAcceptBefore = numTotalAccept;
		numTotalUpdateBefore = numTotalUpdate;
		numTotalRecvBefore = numTotalRecv;
		numTotalSendBefore = numTotalSend;

		std::cout << "Login Packet Recv : " << chatServer.GetTotalLoginPacketCount() << std::endl;
		std::cout << "Chat Packet Recv : " << chatServer.GetTotalChattingPacketCount() << std::endl;
		std::cout << "SectorMove Packet Recv : " << chatServer.GetTotalSectorMovePacketCount() << std::endl;

		std::cout << "CPU Usage : " << cpuUsage << std::endl;
		std::cout << "Memory Use MBytes : " << privateBytes << std::endl;

		Sleep(1000);
	}
}

int main(void)
{
	CCrashDump::Init();

	WSADATA wsaData;

	WSAStartup(MAKEWORD(2, 2), &wsaData);

	RunChatServer();

	return 0;
}
