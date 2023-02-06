#pragma comment(lib, "NetworkLib.lib")
#pragma comment (lib, "cpp_redis.lib")
#pragma comment (lib, "tacopie.lib")
#pragma comment (lib, "DbgHelp.Lib")
#pragma comment(lib, "pdh.lib")

#include "NetworkLib/NetworkLib.h"

#include <cpp_redis/cpp_redis>
#include <codecvt>
#include <Psapi.h>
#include <Pdh.h>
#include <Psapi.h>

#include "CCrashDump.h"
#include "ProcessDataHelper.h"
#include "ProcessTaskManager.h"
#include "mysql.h"
#include "errmsg.h"
#include "DBConnector.h"
#include "MonitorProtocol.h"
#include "MonitoringClient.h"
#include "LoginServer.h"

#define LOGIN_SERVER_NO (5)
#define LOGIN_SERVER_PORT (11803)
#define CHAT_SERVER_IP (L"10.0.1.1")
#define CHAT_SERVER_PORT (11802)
#define GAME_SERVER_IP (L"10.0.1.1")
#define GAME_SERVER_PORT (11802)

#define MONITOR_SERVER_PORT (11850)
#define MAX_PROCESS_NAME (128)

int main(void)
{
	WSADATA wsaData;

	WSAStartup(MAKEWORD(2, 2), &wsaData);
	CCrashDump::Init();

	wchar_t processPath[MAX_PROCESS_NAME];

	GetModuleFileNameW(nullptr, processPath, MAX_PROCESS_NAME);

	wchar_t* processName = wcsrchr(processPath, L'\\') + 1;
	wchar_t* extention = wcsrchr(processName, L'.');

	*extention = L'\0';

	unsigned int numTotalAccept = 0;
	unsigned int numTotalAcceptBefore = 0;
	unsigned int numTotalAuthorizeCompleted = 0;
	unsigned int numTotalAuthorizeCompletedBefore = 0;
	unsigned int numTotalRecv = 0;
	unsigned int numTotalRecvBefore = 0;
	unsigned int numTotalSend = 0;
	unsigned int numTotalSendBefore = 0;

	int cpuUsage;
	int privateBytes;

	NetworkLib::WANServer* wanServer = new NetworkLib::WANServer();
	NetworkLib::LANClient* lanClient = new NetworkLib::LANClient();

	MonitoringClient monitor(lanClient, LOGIN_SERVER_NO);
	LoginServer loginServer(wanServer, GAME_SERVER_IP, GAME_SERVER_PORT, CHAT_SERVER_IP, CHAT_SERVER_PORT);
	ProcessDataHelper pdh(processName);
	ProcessTaskManager taskManager;

	if (!pdh.TryInit())
	{
		std::cout << "Failed Init PDH" << std::endl;
		return 0;
	}
	if (!loginServer.TryRun(INADDR_ANY, LOGIN_SERVER_PORT, 4, 2, 10000, true))
	{
		std::cout << "Failed Run Login Server" << std::endl;
		return 0;
	}
	if (!monitor.TryRun(INADDR_LOOPBACK, MONITOR_SERVER_PORT, true))
	{
		std::cout << "Failed Run Monitor Client" << std::endl;
		return 0;
	}
	while (true)
	{
		pdh.Update();
		taskManager.UpdateProcessInfo();

		numTotalAccept = wanServer->GetNumAccept();
		numTotalRecv = wanServer->GetNumRecv();
		numTotalSend = wanServer->GetNumSend();
		numTotalAuthorizeCompleted = loginServer.GetNumAuthCompeltedSession();
		cpuUsage = (int)taskManager.GetProcessTotalUsage();
		privateBytes = (int)pdh.GetPrivateBytes();

		monitor.Monitor(dfMONITOR_DATA_TYPE_LOGIN_PACKET_POOL, (int)wanServer->GetMessagePoolActiveCount());
		monitor.Monitor(dfMONITOR_DATA_TYPE_LOGIN_SERVER_RUN, 1);
		monitor.Monitor(dfMONITOR_DATA_TYPE_LOGIN_SERVER_CPU, cpuUsage);
		monitor.Monitor(dfMONITOR_DATA_TYPE_LOGIN_SERVER_MEM, privateBytes);
		monitor.Monitor(dfMONITOR_DATA_TYPE_LOGIN_SESSION, (int)wanServer->GetCurrentSessionCount());
		monitor.Monitor(dfMONITOR_DATA_TYPE_LOGIN_AUTH_TPS, (int)numTotalAuthorizeCompleted - numTotalAuthorizeCompletedBefore);


		std::cout << "======================================================" << std::endl;
		std::cout << "Session Count : " << wanServer->GetCurrentSessionCount() << std::endl;
		std::cout << "Working Thread : " << wanServer->GetNumWorkingThread() << std::endl;
		std::cout << "User Count : " << loginServer.GetNumUsers() << std::endl;
		std::cout << "User Pool Size : " << loginServer.GetUserPoolSize() << std::endl;
		std::cout << "Authorize Completed Session : " << numTotalAuthorizeCompleted << std::endl;
		std::cout << "Authorize Failed Session : " << loginServer.GetNumAuthFailedSession() << std::endl;
		std::cout << "Active Packet Count : " << wanServer->GetMessagePoolActiveCount() << std::endl;
		std::cout << "Request Disconnect Count : " << wanServer->GetNumRequestDisconnected() << std::endl;
		std::cout << "Send Queue Full Disconnect Count : " << wanServer->GetNumSendQueueFullDisconnected() << std::endl;
		std::cout << "Limit Session Disconnect Count : " << wanServer->GetNumLimitSessionDisconnected() << std::endl;
		std::cout << "Invalid Session Disconnect Count : " << wanServer->GetNumInvalidSessionDisconnected() << std::endl << std::endl;

		std::cout << "Accept Total : " << numTotalAccept << std::endl;
		std::cout << "Accept TPS : " << numTotalAccept - numTotalAcceptBefore << std::endl;
		std::cout << "Recv TPS : " << numTotalRecv - numTotalRecvBefore << std::endl;
		std::cout << "Send TPS : " << numTotalSend - numTotalSendBefore << std::endl << std::endl;
		std::cout << "Authorize Completed TPS : " << numTotalAuthorizeCompleted - numTotalAuthorizeCompletedBefore << std::endl;
		std::cout << "TCP Retransmitted Packet/sec : " << pdh.GetNumRetransmittedPacket() << std::endl << std::endl;

		std::cout << "CPU Usage : " << cpuUsage << std::endl;
		std::cout << "Memory Use MBytes : " << privateBytes << std::endl;

		numTotalAcceptBefore = numTotalAccept;
		numTotalAuthorizeCompletedBefore = numTotalAuthorizeCompleted;
		numTotalRecvBefore = numTotalRecv;
		numTotalSendBefore = numTotalSend;

		Sleep(1000);
	}

	return 0;
}