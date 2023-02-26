#pragma once

class GameServer : public NetworkLib::INetworkServerEventListener
{
public:
	GameServer(NetworkLib::WANServer* server);
	~GameServer();

	bool TryRun(const unsigned long IP, const unsigned short port
		, const unsigned int numWorkerThread, const unsigned int numRunningThread
		, const unsigned int maxSessionCount, const bool bSupportsNagle, unsigned char fixed);

	virtual void OnError(const int errorCode, const wchar_t* message) override;
	virtual void OnRecv(const sessionID_t ID, NetworkLib::Message* message) override;
	virtual bool OnConnectionRequest(const unsigned long IP, const unsigned short port) override;
	virtual void OnClientJoin(const sessionID_t ID, const unsigned long IP, const unsigned short port) override;
	virtual void OnClientLeave(const sessionID_t ID) override;

private:
	NetworkLib::WANServer* mNetServer;
};