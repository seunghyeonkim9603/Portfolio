#include "stdafx.h"

#define GAME_SERVER_PORT (11803)
#define MONITOR_SERVER_PORT (11850)

#define SERVER_NO (3)
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
	unsigned int numTotalRecv = 0;
	unsigned int numTotalRecvBefore = 0;
	unsigned int numTotalSend = 0;
	unsigned int numTotalSendBefore = 0;
	unsigned int numTotalDBWrite = 0;
	unsigned int numTotalDBWriteBefore = 0;

	int cpuUsage;
	int privateBytes;

	//DBWriterThread

	DBWriterThread logDBWriter("127.0.0.1", "root", "123456", "logdb", 3306);
	DBWriterThread gameDBWriter("127.0.0.1", "root", "123456", "gamedb", 3306);
	{
		logDBWriter.TryRun();
		gameDBWriter.TryRun();
	}

	// WANServer, LANServer »ý¼º
	NetworkLib::WANServer* wanServer = new NetworkLib::WANServer();
	NetworkLib::LANClient* lanClient = new NetworkLib::LANClient();

	GameServer gameServer(wanServer);

	LoginThread* loginThread = new LoginThread(wanServer, &gameDBWriter, &logDBWriter);
	SelectCharacterThread* characterThread = new SelectCharacterThread(wanServer, &gameDBWriter, &logDBWriter);
	GameThread* gameThread = new GameThread(wanServer, &gameDBWriter, &logDBWriter);

	wanServer->RegisterContentThread(loginThread);
	wanServer->RegisterContentThread(characterThread);
	wanServer->RegisterContentThread(gameThread);

	MonitoringClient monitor(lanClient, SERVER_NO);
	ProcessDataHelper pdh(processName);
	ProcessTaskManager taskManager;

	if (!wanServer->TryRun(INADDR_ANY, GAME_SERVER_PORT, 4, 2, 7000, true, 0x32, &gameServer))
	{
		std::cout << "Failed Run Chat Server" << std::endl;
		return 0;
	}
	if (!monitor.TryRun(INADDR_LOOPBACK, MONITOR_SERVER_PORT, true))
	{
		std::cout << "Faild Run Monitoring Client" << std::endl;
		return 0;
	}
	if (!pdh.TryInit())
	{
		std::cout << "Failed Init PDH" << std::endl;
		return 0;
	}

	while (true)
	{
		pdh.Update();
		taskManager.UpdateProcessInfo();

		numTotalAccept = wanServer->GetNumAccept();
		numTotalRecv = wanServer->GetNumRecv();
		numTotalSend = wanServer->GetNumSend();
		numTotalDBWrite = logDBWriter.GetNumWrite() + gameDBWriter.GetNumWrite();

		cpuUsage = (int)taskManager.GetProcessTotalUsage();
		privateBytes = (int)pdh.GetPrivateBytes();

		monitor.Monitor(dfMONITOR_DATA_TYPE_GAME_PACKET_POOL, (int)wanServer->GetMessagePoolActiveCount());
		monitor.Monitor(dfMONITOR_DATA_TYPE_GAME_SERVER_RUN, 1);
		monitor.Monitor(dfMONITOR_DATA_TYPE_GAME_SERVER_CPU, cpuUsage);
		monitor.Monitor(dfMONITOR_DATA_TYPE_GAME_SERVER_MEM, privateBytes);
		monitor.Monitor(dfMONITOR_DATA_TYPE_GAME_SESSION, (int)wanServer->GetCurrentSessionCount());
		monitor.Monitor(dfMONITOR_DATA_TYPE_GAME_AUTH_PLAYER, (int)loginThread->GetNumPlayer());
		monitor.Monitor(dfMONITOR_DATA_TYPE_GAME_GAME_PLAYER, (int)gameThread->GetNumPlayer());
		monitor.Monitor(dfMONITOR_DATA_TYPE_GAME_AUTH_THREAD_FPS, (int)loginThread->GetFPS());
		monitor.Monitor(dfMONITOR_DATA_TYPE_GAME_GAME_THREAD_FPS, (int)gameThread->GetFPS());
		monitor.Monitor(dfMONITOR_DATA_TYPE_GAME_DB_WRITE_TPS, (int)numTotalDBWrite - numTotalDBWriteBefore);
		monitor.Monitor(dfMONITOR_DATA_TYPE_GAME_DB_WRITE_MSG, logDBWriter.GetQueryQueueSize() + gameDBWriter.GetQueryQueueSize());
		monitor.Monitor(dfMONITOR_DATA_TYPE_GAME_ACCEPT_TPS, (int)numTotalAccept - numTotalAcceptBefore);
		monitor.Monitor(dfMONITOR_DATA_TYPE_GAME_PACKET_RECV_TPS, (int)numTotalRecv - numTotalRecvBefore);
		monitor.Monitor(dfMONITOR_DATA_TYPE_GAME_PACKET_SEND_TPS, (int)numTotalSend - numTotalSendBefore);

		std::cout << "======================================================" << std::endl;
		std::cout << "Session Count : " << wanServer->GetCurrentSessionCount() << std::endl;
		std::cout << "Login Thread Player Count : " << loginThread->GetNumPlayer() << std::endl;
		std::cout << "Game Thread Player Count : " << gameThread->GetNumPlayer() << std::endl;
		std::cout << "Select Thread Player Count : " << characterThread->GetNumPlayer() << std::endl;
		std::cout << "Game Thread FPS : " << gameThread->GetFPS() << std::endl;
		std::cout << "Total Login Count : " << loginThread->GetNumLoginSucceed() << std::endl;

		std::cout << "Active Packet Count : " << wanServer->GetMessagePoolActiveCount() << std::endl;
		std::cout << "Request Disconnect Count : " << wanServer->GetNumRequestDisconnected() << std::endl;
		std::cout << "Send Queue Full Disconnect Count : " << wanServer->GetNumSendQueueFullDisconnected() << std::endl;
		std::cout << "Limit Session Disconnect Count : " << wanServer->GetNumLimitSessionDisconnected() << std::endl;
		std::cout << "Invalid Session Disconnect Count : " << wanServer->GetNumInvalidSessionDisconnected() << std::endl << std::endl;

		std::cout << "Accept Total : " << numTotalAccept << std::endl;
		std::cout << "Accept TPS : " << numTotalAccept - numTotalAcceptBefore << std::endl;
		std::cout << "Recv TPS : " << numTotalRecv - numTotalRecvBefore << std::endl;
		std::cout << "Send TPS : " << numTotalSend - numTotalSendBefore << std::endl;
		std::cout << "TCP Retransmitted Packet/sec : " << pdh.GetNumRetransmittedPacket() << std::endl << std::endl;

		numTotalAcceptBefore = numTotalAccept;
		numTotalRecvBefore = numTotalRecv;
		numTotalSendBefore = numTotalSend;
		numTotalDBWriteBefore = numTotalDBWrite;

		std::cout << "CPU Usage : " << cpuUsage << std::endl;
		std::cout << "Memory Use MBytes : " << privateBytes << std::endl;

		Sleep(1000);
	}

	WSACleanup();
	return 0;
}