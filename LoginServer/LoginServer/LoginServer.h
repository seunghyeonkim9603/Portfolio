#pragma once

class LoginServer : public NetworkLib::INetworkServerEventListener
{
public:
	struct User
	{
		unsigned int	IP;
		unsigned short	Port;
		sessionID_t		SessionID;
		LARGE_INTEGER	AccessTime;
		bool			bIsRequested;
	};

public:
	LoginServer(NetworkLib::WANServer* server, const WCHAR* gameServerIP, USHORT gameServerPort, const WCHAR* chatServerIP, USHORT chatServerPort);
	virtual ~LoginServer();

	bool TryRun(const unsigned long IP, const unsigned short port
				, const unsigned int numWorkerThread, const unsigned int numRunningThread
				, const unsigned int maxSessionCount, const bool bSupportsNagle);

	unsigned int GetNumAuthCompeltedSession() const;
	unsigned int GetNumAuthFailedSession() const;
	unsigned int GetNumSessionBeforeLogin() const;
	unsigned int GetNumUsers() const;
	unsigned int GetUserPoolSize() const;
	
	virtual void OnError(const int errorCode, const wchar_t* message) override;
	virtual void OnRecv(const sessionID_t ID, NetworkLib::Message* message) override;
	virtual bool OnConnectionRequest(const unsigned long IP, const unsigned short port) override;
	virtual void OnClientJoin(const sessionID_t ID, const unsigned long IP, const unsigned short port) override;
	virtual void OnClientLeave(const sessionID_t ID) override;

private:
	static unsigned int __stdcall disconnectThread(void* param);

private:
	enum
	{
		QUERY_LENGTH = 256,
		SESSION_KEY_LENGTH = 64,
		USER_ID_LENGTH = 20,
		USER_NICKNAME_LENGTH = 20,
		IP_LENGTH = 16
	};
	static thread_local DBConnector DbConnector;

	NetworkLib::WANServer* mNetServer;
	cpp_redis::client mRedis;

	NetworkLib::ObjectPool<User> mUserPool;

	SRWLOCK mUsersLock;
	std::unordered_map<sessionID_t, User*> mUsers;

	HANDLE mhDisconnectThread;
	WCHAR mGameServerIP[IP_LENGTH];
	WCHAR mChatServerIP[IP_LENGTH];
	USHORT mGameServerPort;
	USHORT mChatServerPort;

	unsigned int mNumAuthCompletedSession;
	unsigned int mNumAuthFailedSession;
	unsigned int mNumSessionBeforeLogin;
};