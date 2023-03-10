#pragma once

class LoginThread : public NetworkLib::ContentThread
{
public:
	LoginThread(NetworkLib::WANServer* wanServer, DBWriterThread* gameDB, DBWriterThread* logDB);
	virtual ~LoginThread();

	virtual void OnThreadStart() override;
	virtual void OnThreadEnd() override;
	virtual void OnUpdate() override;
	virtual void OnClientThreadJoin(sessionID_t id, void* transffered) override;
	virtual void OnClientThreadLeave(sessionID_t id) override;
	virtual void OnClientDisconnect(sessionID_t id) override;
	virtual void OnRecv(sessionID_t id, NetworkLib::Message* message) override;

	unsigned int GetNumPlayer() const;
	unsigned int GetNumLoginSucceed() const;
	unsigned int GetNumLoginFailed() const;

private:
	enum
	{
		THREAD_FPS = 60,
		SESSION_KEY_LENGTH = 64,
		ACCOUNT_NO_BUF_LENGTH = 64
	};

	NetworkLib::TLSObjectPool<Player> mPlayerPool;
	std::unordered_map<sessionID_t, Player*> mPlayers;

	DBConnector			mDBConnector;
	cpp_redis::client	mRedis;

	DBWriterThread*		mGameDBWriter;
	DBWriterThread*		mLogDBWriter;

	unsigned int mNumPlayer;
	unsigned int mNumLoginSucceed;
	unsigned int mNumLoginFailed;
};