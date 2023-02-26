#pragma once

class MonitoringClient : NetworkLib::INetworkClientEventListener
{
public:
	MonitoringClient(NetworkLib::LANClient* lanClient, int serverNo);
	~MonitoringClient();

	bool TryRun(const unsigned long IP, const unsigned short port, const bool bSupportsNagle);
	void Monitor(BYTE type, int value);

	virtual void OnError(const int errorCode, const wchar_t* message) override;
	virtual void OnRecv(NetworkLib::Message* message) override;
	virtual void OnConnect() override;
	virtual void OnDisconnect() override;

private:

	NetworkLib::LANClient*	mLANClient;
	int						mServerNo;
};