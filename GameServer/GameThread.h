#pragma once

class GameThread : public NetworkLib::ContentThread
{
public:
	GameThread(NetworkLib::WANServer* wanServer, DBWriterThread* gameDB, DBWriterThread* logDB);
	virtual ~GameThread();

	virtual void OnThreadStart() override;
	virtual void OnThreadEnd() override;
	virtual void OnUpdate() override;
	virtual void OnClientThreadJoin(sessionID_t id, void* transffered) override;
	virtual void OnClientThreadLeave(sessionID_t id) override;
	virtual void OnClientDisconnect(sessionID_t id) override;
	virtual void OnRecv(sessionID_t id, NetworkLib::Message* message) override;

	void SendSector(Sector& sector, NetworkLib::Message* message, sessionID_t excepted);
	void SendAroundSector(int x, int y, NetworkLib::Message* message, sessionID_t excepted);

	void OnPlayerSpawn(Player* player);
	void OnSectorMove(Object* obj, int fromX, int fromY, int toX, int toY);
	void OnSectorJoin(Object* obj, Sector* sector, bool bIsRespawn);
	void OnSectorLeave(Object* obj, Sector* sector);
	void OnSit(Player* player);
	void OnStand(Player* player);
	void OnMonsterAttack(Monster* monster, Player* target);
	void GetObjectsInInteractionRange(Object* obj, std::vector<Object*>* outObjects, EObjectType targetType);

	unsigned int GetNumPlayer() const;
private:
	void processMovePacket(Player* target, NetworkLib::Message* message);
	void processMoveStopPacket(Player* target, NetworkLib::Message* message);
	void processAttackPacket(Player* target, NetworkLib::Message* message, int damage, WORD type);
	void processPickPacket(Player* target, NetworkLib::Message* message);
	void processSitPacket(Player* target, NetworkLib::Message* message);
	void processRestartPacket(Player* target, NetworkLib::Message* message);
	void moveMonster(Monster* monster, float toX, float toY);
private:
	enum
	{
		THREAD_FPS = 60,
		MAP_TILE_X_MAX = 400,
		MAP_TILE_Y_MAX = 200,
		TILES_PER_SECTOR = 4,
		SECTOR_X_MAX = MAP_TILE_X_MAX / TILES_PER_SECTOR,
		SECTOR_Y_MAX = MAP_TILE_Y_MAX / TILES_PER_SECTOR,
		MONSTERS_PER_AREA = 10,
		MONSTER_RESPAWN_TIME_SEC = 5
	};
	NetworkLib::TLSObjectPool<Player> mPlayerPool;
	NetworkLib::ObjectPool<Monster> mMonsterPool;
	NetworkLib::ObjectPool<Cristal> mCristalPool;

	std::unordered_map<sessionID_t, Player*> mPlayers;
	std::unordered_map<INT64, Monster*> mMonsters;

	std::queue<Monster*> mDeadMonsters;
	std::vector<Player*> mSitPlayers;

	Sector mSectors[SECTOR_Y_MAX][SECTOR_X_MAX];

	DBWriterThread* mGameDBWriter;
	DBWriterThread* mLogDBWriter;

	unsigned int mNumPlayer;
};