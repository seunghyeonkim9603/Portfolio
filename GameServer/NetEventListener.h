#pragma once

class NetEventListener : public NetworkLib::INetworkServerEventListener
{
public:
	NetEventListener(NetworkLib::WANServer* server);
	~NetEventListener();

	bool TryRun(const unsigned long IP, const unsigned short port
		, const unsigned int numWorkerThread, const unsigned int numRunningThread
		, const unsigned int maxSessionCount, const bool bSupportsNagle, unsigned char fixed);

	virtual void OnError(const int errorCode, const wchar_t* message) override;
	virtual bool OnConnectionRequest(const unsigned long IP, const unsigned short port) override;

private:
	NetworkLib::WANServer* mNetServer;
};