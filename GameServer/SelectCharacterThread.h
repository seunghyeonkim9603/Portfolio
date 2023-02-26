#pragma once

class SelectCharacterThread : public NetworkLib::ContentThread
{
public:
	SelectCharacterThread(NetworkLib::WANServer* wanServer, DBWriterThread* gameDB, DBWriterThread* logDB);
	virtual ~SelectCharacterThread();

	virtual void OnThreadStart() override;
	virtual void OnThreadEnd() override;
	virtual void OnUpdate() override;
	virtual void OnClientThreadJoin(sessionID_t id, void* transffered) override;
	virtual void OnClientThreadLeave(sessionID_t id) override;
	virtual void OnClientDisconnect(sessionID_t id) override;
	virtual void OnRecv(sessionID_t id, NetworkLib::Message* message) override;

	unsigned int GetNumPlayer() const;

private:
	enum
	{
		THREAD_FPS = 60,
	};

	NetworkLib::TLSObjectPool<Player> mPlayerPool;
	std::unordered_map<sessionID_t, Player*> mPlayers;

	DBWriterThread* mGameDBWriter;
	DBWriterThread* mLogDBWriter;

	unsigned int mNumPlayer;
};