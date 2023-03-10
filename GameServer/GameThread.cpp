#include "stdafx.h"

#define INTERACTION_RANGE (1.0f)
#define LOG_FILE_NAME (L"GameThreadLog.txt")

LARGE_INTEGER gFreq;

GameThread::GameThread(NetworkLib::WANServer* wanServer, DBWriterThread* gameDB, DBWriterThread* logDB)
	: ContentThread(wanServer, EContentType::CONTENT_TYPE_GAME, THREAD_FPS),
	mGameDBWriter(gameDB),
	mLogDBWriter(logDB),
	mNumPlayer(0)
{
	for (unsigned int y = 0; y < SECTOR_Y_MAX; ++y)
	{
		for (unsigned int x = 0; x < SECTOR_X_MAX; ++x)
		{
			Sector* sector = &mSectors[y][x];

			sector->SetX(x);
			sector->SetY(y);

			if (x != 0)
			{
				sector->RegisterAdjSector(&mSectors[y][x - 1]);
				if (y != 0)
				{
					sector->RegisterAdjSector(&mSectors[y - 1][x - 1]);
				}
			}
			if (y != 0)
			{
				sector->RegisterAdjSector(&mSectors[y - 1][x]);
				if (x != MAP_TILE_X_MAX - 1)
				{
					sector->RegisterAdjSector(&mSectors[y - 1][x + 1]);
				}
			}
			if (x != MAP_TILE_X_MAX - 1)
			{
				sector->RegisterAdjSector(&mSectors[y][x + 1]);
				if (y != MAP_TILE_Y_MAX - 1)
				{
					sector->RegisterAdjSector(&mSectors[y + 1][x + 1]);
				}
			}
			if (y != MAP_TILE_Y_MAX - 1)
			{
				sector->RegisterAdjSector(&mSectors[y + 1][x]);
				if (x != 0)
				{
					sector->RegisterAdjSector(&mSectors[y + 1][x - 1]);
				}
			}
		}
	}
}

GameThread::~GameThread()
{
}

void GameThread::OnThreadStart()
{
	QueryPerformanceFrequency(&gFreq);

	for (unsigned int i = 0; i < MONSTERS_PER_AREA; ++i)
	{
		Monster* monster = mMonsterPool.GetObject();
		InitMonster(monster, 50, 10, 26, 169, 21);

		Sector* sector = &mSectors[monster->TileY / TILES_PER_SECTOR][monster->TileX / TILES_PER_SECTOR];

		mMonsters.insert({ monster->ClientID, monster });
		sector->InsertObject(monster);
	}
	for (unsigned int i = 0; i < MONSTERS_PER_AREA; ++i)
	{
		Monster* monster = mMonsterPool.GetObject();

		InitMonster(monster, 50, 10, 95, 169, 24);

		Sector* sector = &mSectors[monster->TileY / TILES_PER_SECTOR][monster->TileX / TILES_PER_SECTOR];

		mMonsters.insert({ monster->ClientID, monster });
		sector->InsertObject(monster);
	}
	for (unsigned int i = 0; i < MONSTERS_PER_AREA; ++i)
	{
		Monster* monster = mMonsterPool.GetObject();

		InitMonster(monster, 50, 10, 163, 152, 17);

		Sector* sector = &mSectors[monster->TileY / TILES_PER_SECTOR][monster->TileX / TILES_PER_SECTOR];

		mMonsters.insert({ monster->ClientID, monster });
		sector->InsertObject(monster);
	}
	for (unsigned int i = 0; i < MONSTERS_PER_AREA; ++i)
	{
		Monster* monster = mMonsterPool.GetObject();

		InitMonster(monster, 50, 10, 171, 33, 25);

		Sector* sector = &mSectors[monster->TileY / TILES_PER_SECTOR][monster->TileX / TILES_PER_SECTOR];

		mMonsters.insert({ monster->ClientID, monster });
		sector->InsertObject(monster);
	}
	for (unsigned int i = 0; i < MONSTERS_PER_AREA; ++i)
	{
		Monster* monster = mMonsterPool.GetObject();

		InitMonster(monster, 50, 10, 34, 61, 17);

		Sector* sector = &mSectors[monster->TileY / TILES_PER_SECTOR][monster->TileX / TILES_PER_SECTOR];

		mMonsters.insert({ monster->ClientID, monster });
		sector->InsertObject(monster);
	}
}

void GameThread::OnThreadEnd()
{
}

void GameThread::OnUpdate()
{
	LARGE_INTEGER cur;

	QueryPerformanceCounter(&cur);

	for (auto iter : mMonsters)
	{
		Monster* monster = iter.second;

		float monPosX = monster->PosX;
		float monPosY = monster->PosY;

		int monSectorX = monster->TileX / TILES_PER_SECTOR;
		int monSectorY = monster->TileY / TILES_PER_SECTOR;

		if (cur.QuadPart - monster->LastActionTime.QuadPart < gFreq.QuadPart)
		{
			continue;
		}
		int random = rand() % 100;

		int range = monster->MoveRange / 2;

		float targetPosX = (float)((monster->CenterTileX - range) + (rand() % monster->MoveRange)) / 2;
		float targetPosY = (float)((monster->CenterTileY - range) + (rand() % monster->MoveRange)) / 2;

		if (monster->Target == nullptr)
		{
			if (98 < random)
			{
				moveMonster(monster, targetPosX, targetPosY);
			}
		}
		else
		{
			Player* target = monster->Target;

			targetPosX = target->PosX;
			targetPosY = target->PosY;

			int targetX = target->TileX;
			int targetY = target->TileY;

			int targetSectorX = targetX / TILES_PER_SECTOR;
			int targetSectorY = targetY / TILES_PER_SECTOR;

			if (targetX < monster->MoveXMin || monster->MoveXMax < targetX || targetY < monster->MoveYMin || monster->MoveYMax < targetY)
			{
				monster->Target = nullptr;
			}
			else
			{
				Point rect[4];

				float range = INTERACTION_RANGE / 2;

				rect[0] = { monPosX - range, monPosY - INTERACTION_RANGE + 0.2};
				rect[1] = { monPosX - range, monPosY  + 0.2};
				rect[2] = { monPosX + range, monPosY + 0.2};
				rect[3] = { monPosX + range, monPosY - INTERACTION_RANGE + 0.2};

				RotateRectangle(rect, monster->Rotation, monPosX, monPosY);

				if (IsPointInRectangle(rect, {targetPosX, targetPosY}))
				{
					onMonsterAttack(monster, target);
				}
				else
				{
					moveMonster(monster, targetPosX, targetPosY);
				}
			}
		}
	}

	while (true)
	{
		if (mDeadMonsters.empty())
		{
			break;
		}
		Monster* deadMonster = mDeadMonsters.front();

		if (cur.QuadPart - deadMonster->DeadTime.QuadPart < gFreq.QuadPart * MONSTER_RESPAWN_TIME_SEC)
		{
			break;
		}
		deadMonster->Target = nullptr;
		InitRespawnMonster(deadMonster);

		int sectorX = deadMonster->TileX / TILES_PER_SECTOR;
		int sectorY = deadMonster->TileY / TILES_PER_SECTOR;

		Sector* sector = &mSectors[sectorY][sectorX];
		const std::vector<Sector*>* adjSectors = sector->GetAdjSectors();

		sector->InsertObject(deadMonster);
		onSectorJoin(deadMonster, sector, true);

		for (Sector* adj : *adjSectors)
		{
			onSectorJoin(deadMonster, adj, true);
		}
		mMonsters.insert({ deadMonster->ClientID, deadMonster });

		mDeadMonsters.pop();
	}
}

void GameThread::OnClientThreadJoin(sessionID_t id, void* transffered)
{
	Player* joined = (Player*)transffered;

	Logger::AppendLine(L"Rot : %d", joined->Rotation);
	Logger::Log(LOG_FILE_NAME);

	onPlayerSpawn(joined);
	mPlayers.insert({ id, joined });

	++mNumPlayer;
}

void GameThread::OnClientThreadLeave(sessionID_t id)
{
	Player* leaved = mPlayers.find(id)->second;

	int sectorX = leaved->TileX / TILES_PER_SECTOR;
	int sectorY = leaved->TileY / TILES_PER_SECTOR;

	Sector* sector = &mSectors[sectorY][sectorX];

	const std::vector<Sector*>* adjSectors = sector->GetAdjSectors();

	sector->RemoveObject(leaved->ClientID);
	mPlayers.erase(id);

	onSectorLeave(leaved, sector);

	for (Sector* adj : *adjSectors)
	{
		onSectorLeave(leaved, adj);
	}
	--mNumPlayer;
}

void GameThread::OnClientDisconnect(sessionID_t id)
{
	Player* leaved = mPlayers.find(id)->second;

	int sectorX = leaved->TileX / TILES_PER_SECTOR;
	int sectorY = leaved->TileY / TILES_PER_SECTOR;

	Sector* sector = &mSectors[sectorY][sectorX];

	const std::vector<Sector*>* adjSectors = sector->GetAdjSectors();

	sector->RemoveObject(leaved->ClientID);
	mPlayers.erase(id);

	onSectorLeave(leaved, sector);

	for (Sector* adj : *adjSectors)
	{
		onSectorLeave(leaved, adj);
	}
	mLogDBWriter->Write("INSERT INTO `logdb`.`gamelog`(type, code, accountno, servername, param1, param2, param3, param4) VALUES(1, 12, %lld, \"game\", %d, %d, %d, %d)",
		leaved->AccountNo, leaved->TileX, leaved->TileY, leaved->Cristal, leaved->HP);
	mGameDBWriter->Write("UPDATE `gamedb`.`character` SET charactertype = %d, posx = %f, posy = %f, tilex = %d, tiley = %d, rotation = %d, cristal = %d, hp = %d, exp = %d, level = %d, die = %d WHERE accountno = %d",
		leaved->CharacterType, leaved->PosX, leaved->PosY, leaved->TileX, leaved->TileY, leaved->Rotation, leaved->Cristal, leaved->HP, leaved->Exp, leaved->Level, leaved->bIsDie, leaved->AccountNo);

	mPlayerPool.ReleaseObject(leaved);

	--mNumPlayer;
}

void GameThread::OnRecv(sessionID_t id, NetworkLib::Message* message)
{
	Player* target = mPlayers.find(id)->second;

	WORD type;

	*message >> type;

	switch (type)
	{
	case en_PACKET_CS_GAME_REQ_MOVE_CHARACTER:
		processMovePacket(target, message);
		break;
	case en_PACKET_CS_GAME_REQ_STOP_CHARACTER:
		processMoveStopPacket(target, message);
		break;
	case en_PACKET_CS_GAME_REQ_ATTACK1:
		processAttackPacket(target, message, 10, en_PACKET_CS_GAME_RES_ATTACK1);
		break;
	case en_PACKET_CS_GAME_REQ_ATTACK2:
		processAttackPacket(target, message, 20, en_PACKET_CS_GAME_RES_ATTACK2);
		break;
	case en_PACKET_CS_GAME_REQ_PICK:
		processPickPacket(target, message);
		break;
	case en_PACKET_CS_GAME_REQ_SIT:
		processSitPacket(target, message);
		break;
	case en_PACKET_CS_GAME_REQ_PLAYER_RESTART:
		processRestartPacket(target, message);
		break;
	case en_PACKET_CS_GAME_REQ_ECHO:
		break;
	case en_PACKET_CS_GAME_REQ_HEARTBEAT:
		QueryPerformanceCounter(&target->LastRecvedTime);
		break;
	default:
		break;
	}
}

void GameThread::sendSector(Sector& sector, NetworkLib::Message* message, sessionID_t excepted)
{
	auto hashMap = sector.GetObjects();

	for (auto iter : *hashMap)
	{
		Object* obj = iter.second;

		if (obj->ObjectType == EObjectType::OBJECT_TYPE_PLAYER) // 보내는 사람 제외 안함
		{
			Player* p = (Player*)obj;

			if (p->SessionID != excepted)
			{
				TrySendMessage(p->SessionID, message);
			}
		}
	}
}

void GameThread::sendAroundSector(int x, int y, NetworkLib::Message* message, sessionID_t excepted)
{
	Sector* sector = &mSectors[y][x];
	auto adjVector = sector->GetAdjSectors();

	sendSector(*sector, message, excepted);

	for (Sector* adj : *adjVector)
	{
		sendSector(*adj, message, excepted);
	}
}

void GameThread::onPlayerSpawn(Player* player)
{
	Player* joined = player;

	int sectorX = joined->TileX / TILES_PER_SECTOR;
	int sectorY = joined->TileY / TILES_PER_SECTOR;

	Sector* sector = &mSectors[sectorY][sectorX];

	const std::vector<Sector*>* adjSectors = sector->GetAdjSectors();

	NetworkLib::Message* sendMessage = AllocMessage();
	{
		*sendMessage << (WORD)en_PACKET_CS_GAME_RES_CREATE_MY_CHARACTER;
		*sendMessage << joined->ClientID;
		*sendMessage << (BYTE)joined->CharacterType;
		sendMessage->Write((const char*)joined->Nickname, sizeof(WCHAR) * 20);
		*sendMessage << joined->PosX;
		*sendMessage << joined->PosY;
		*sendMessage << joined->Rotation;
		*sendMessage << joined->Cristal;
		*sendMessage << joined->HP;
		*sendMessage << joined->Exp;
		*sendMessage << joined->Level;

		TrySendMessage(joined->SessionID, sendMessage);
	}
	ReleaseMessage(sendMessage);

	onSectorJoin(joined, sector, true);

	for (Sector* adj : *adjSectors)
	{
		onSectorJoin(joined, adj, true);
	}
	sector->InsertObject(joined);
}

void GameThread::onSectorMove(Object* obj, int fromX, int fromY, int toX, int toY)
{
	Sector* fromSector = &mSectors[fromY][fromX];
	Sector* toSector = &mSectors[toY][toX];

	const std::vector<Sector*>* fromAdjSectors = fromSector->GetAdjSectors();
	const std::vector<Sector*>* toAdjSectors = toSector->GetAdjSectors();

	for (Sector* fromAdj : *fromAdjSectors)
	{
		if (1 < abs(fromAdj->GetX() - toX) || 1 < abs(fromAdj->GetY() - toY))
		{
			onSectorLeave(obj, fromAdj);
		}
	}
	for (Sector* toAdj : *toAdjSectors)
	{
		if (1 < abs(toAdj->GetX() - fromX) || 1 < abs(toAdj->GetY() - fromY))
		{
			onSectorJoin(obj, toAdj, false);
		}
	}
}

void GameThread::onSectorJoin(Object* joined, Sector* sector, bool bIsRespawn)
{
	const std::unordered_map<INT64, Object*>* objs = sector->GetObjects();

	NetworkLib::Message* otherSendMessage = AllocMessage();

	Player* other;
	Monster* mon;
	Cristal* cri;

	if (joined->ObjectType == OBJECT_TYPE_PLAYER)
	{
		Player* player = (Player*)joined;

		*otherSendMessage << (WORD)en_PACKET_CS_GAME_RES_CREATE_OTHER_CHARACTER;
		*otherSendMessage << player->ClientID;
		*otherSendMessage << (BYTE)player->CharacterType;
		otherSendMessage->Write((const char*)player->Nickname, sizeof(WCHAR) * 20);
		*otherSendMessage << player->PosX;
		*otherSendMessage << player->PosY;
		*otherSendMessage << player->Rotation;
		*otherSendMessage << player->Level;
		*otherSendMessage << (BYTE)bIsRespawn;
		*otherSendMessage << (BYTE)player->bIsSit;
		*otherSendMessage << (BYTE)player->bIsDie;

		for (auto iter : *objs)
		{
			Object* obj = iter.second;

			NetworkLib::Message* sendMessage = AllocMessage();

			switch (obj->ObjectType)
			{
			case EObjectType::OBJECT_TYPE_PLAYER:

				if (obj->ClientID == player->ClientID)
				{
					ReleaseMessage(sendMessage);

					continue;
				}
				other = (Player*)obj;

				*sendMessage << (WORD)en_PACKET_CS_GAME_RES_CREATE_OTHER_CHARACTER;
				*sendMessage << other->ClientID;
				*sendMessage << (BYTE)other->CharacterType;
				sendMessage->Write((const char*)other->Nickname, sizeof(WCHAR) * 20);
				*sendMessage << other->PosX;
				*sendMessage << other->PosY;
				*sendMessage << other->Rotation;
				*sendMessage << other->Level;
				*sendMessage << (BYTE)0;
				*sendMessage << (BYTE)other->bIsSit;
				*sendMessage << (BYTE)other->bIsDie;

				TrySendMessage(other->SessionID, otherSendMessage);

				break;
			case EObjectType::OBJECT_TYPE_MONSTER:

				mon = (Monster*)obj;

				*sendMessage << (WORD)en_PACKET_CS_GAME_RES_CREATE_MONSTER_CHARACTER;
				*sendMessage << mon->ClientID;
				*sendMessage << mon->PosX;
				*sendMessage << mon->PosY;
				*sendMessage << mon->Rotation;
				*sendMessage << (BYTE)0;

				break;
			case EObjectType::OBJECT_TYPE_CRISTAL:

				cri = (Cristal*)obj;

				*sendMessage << (WORD)en_PACKET_CS_GAME_RES_CREATE_CRISTAL;
				*sendMessage << cri->ClientID;
				*sendMessage << (BYTE)1;
				*sendMessage << cri->PosX;
				*sendMessage << cri->PosY;

				break;
			default:
				Logger::AppendLine(L"Unhandled Object Type error");
				Logger::Log(LOG_FILE_NAME);
				break;
			}
			TrySendMessage(player->SessionID, sendMessage);
			ReleaseMessage(sendMessage);
		}
	}
	else
	{
		*otherSendMessage << (WORD)en_PACKET_CS_GAME_RES_CREATE_MONSTER_CHARACTER;
		*otherSendMessage << joined->ClientID;
		*otherSendMessage << joined->PosX;
		*otherSendMessage << joined->PosY;
		*otherSendMessage << joined->Rotation;
		*otherSendMessage << bIsRespawn;

		for (auto iter : *objs)
		{
			Object* obj = iter.second;

			if (obj->ObjectType == OBJECT_TYPE_PLAYER)
			{
				Player* otherPlayer = (Player*)obj;

				TrySendMessage(otherPlayer->SessionID, otherSendMessage);
			}
		}
	}
	ReleaseMessage(otherSendMessage);
}

void GameThread::onSectorLeave(Object* obj, Sector* sector)
{
	const std::unordered_map<INT64, Object*>* objs = sector->GetObjects();

	NetworkLib::Message* otherSendMessage = AllocMessage();

	*otherSendMessage << (WORD)en_PACKET_CS_GAME_RES_REMOVE_OBJECT;
	*otherSendMessage << obj->ClientID;

	if (obj->ObjectType == OBJECT_TYPE_PLAYER)
	{
		Player* player = (Player*)obj;

		for (auto iter : *objs)
		{
			Object* other = iter.second;

			if (obj->ClientID == other->ClientID)
			{
				continue;
			}
			if (other->ObjectType == OBJECT_TYPE_PLAYER)
			{
				Player* otherPlayer = (Player*)other;

				TrySendMessage(otherPlayer->SessionID, otherSendMessage);
			}
			NetworkLib::Message* sendMessage = AllocMessage();

			*sendMessage << (WORD)en_PACKET_CS_GAME_RES_REMOVE_OBJECT;
			*sendMessage << other->ClientID;

			TrySendMessage(player->SessionID, sendMessage);

			ReleaseMessage(sendMessage);
		}
	}
	else
	{
		for (auto iter : *objs)
		{
			Object* other = iter.second;

			if (other->ObjectType == OBJECT_TYPE_PLAYER)
			{
				Player* otherPlayer = (Player*)other;

				TrySendMessage(otherPlayer->SessionID, otherSendMessage);
			}
		}
	}
	ReleaseMessage(otherSendMessage);
}

void GameThread::onSit(Player* player)
{
	player->bIsSit = true;
	mSitPlayers.push_back(player);

	QueryPerformanceCounter(&player->SitBeginTime);

	NetworkLib::Message* sitRes = AllocMessage();
	{
		*sitRes << (WORD)en_PACKET_CS_GAME_RES_SIT;
		*sitRes << player->ClientID;

		sendAroundSector(player->TileX / TILES_PER_SECTOR, player->TileY / TILES_PER_SECTOR, sitRes, player->SessionID);
	}
	ReleaseMessage(sitRes);
}

void GameThread::onStand(Player* player)
{
	LARGE_INTEGER cur;

	QueryPerformanceCounter(&cur);

	player->bIsSit = false;

	int oldHp = player->HP;
	int elapsedSec = (cur.QuadPart - player->SitBeginTime.QuadPart) / gFreq.QuadPart;

	player->HP += elapsedSec * 100;

	NetworkLib::Message* resHp = AllocMessage();
	{
		*resHp << (WORD)en_PACKET_CS_GAME_RES_PLAYER_HP;
		*resHp << player->HP;

		TrySendMessage(player->SessionID, resHp);
	}
	ReleaseMessage(resHp);

	mLogDBWriter->Write("INSERT INTO `logdb`.`gamelog`(type, code, accountno, servername, param1, param2, param3) VALUES(5, 51, %lld, \"game\", %d, %d)",
		player->AccountNo, oldHp, player->HP, elapsedSec);
}

void GameThread::onMonsterAttack(Monster* monster, Player* target)
{
	int monSectorX = monster->TileX / TILES_PER_SECTOR;
	int monSectorY = monster->TileY / TILES_PER_SECTOR;

	int targetSectorX = target->TileX / TILES_PER_SECTOR;
	int targetSectorY = target->TileY / TILES_PER_SECTOR;

	QueryPerformanceCounter(&monster->LastActionTime);

	if (target->HP <= 0)
	{
		monster->Target = nullptr;
		return;
	}
	target->HP -= 100;

	NetworkLib::Message* resMonAttack = AllocMessage();
	{
		*resMonAttack << (WORD)en_PACKET_CS_GAME_RES_MONSTER_ATTACK;
		*resMonAttack << monster->ClientID;

		sendAroundSector(monSectorX, monSectorY, resMonAttack, 0);
	}
	ReleaseMessage(resMonAttack);

	NetworkLib::Message* resDamage = AllocMessage();
	{
		*resDamage << (WORD)en_PACKET_CS_GAME_RES_DAMAGE;
		*resDamage << monster->ClientID;
		*resDamage << target->ClientID;
		*resDamage << 100;

		sendAroundSector(targetSectorX, targetSectorY, resDamage, 0);
	}
	ReleaseMessage(resDamage);

	if (target->HP <= 0)
	{
		target->bIsDie = true;
		target->Cristal -= 10;

		NetworkLib::Message* resPlayerDie = AllocMessage();
		{
			*resPlayerDie << (WORD)en_PACKET_CS_GAME_RES_PLAYER_DIE;
			*resPlayerDie << target->ClientID;
			*resPlayerDie << 10;

			sendAroundSector(targetSectorX, targetSectorY, resPlayerDie, 0);
		}
		ReleaseMessage(resPlayerDie);

		mLogDBWriter->Write("INSERT INTO `logdb`.`gamelog`(type, code, accountno, servername, param1, param2, param3) VALUES(3, 31, %lld, \"game\", %d, %d, %d)",
			target->AccountNo, target->TileX, target->TileY, target->Cristal);
	}
}

void GameThread::getObjectsInInteractionRange(Object* obj, std::vector<Object*>* outObjects, EObjectType targetType)
{
	Point rect[4];

	float posX = obj->PosX;
	float posY = obj->PosY;
	float range = INTERACTION_RANGE / 2;

	rect[0] = { posX - range, posY - INTERACTION_RANGE + 0.2};
	rect[1] = { posX - range, posY + 0.2 };
	rect[2] = { posX + range, posY + 0.2 };
	rect[3] = { posX + range, posY - INTERACTION_RANGE + 0.2};

	RotateRectangle(rect, obj->Rotation, posX, posY);

	int left = INT_MAX;
	int right = INT_MIN;
	int top = INT_MAX;
	int bottom = INT_MIN;

	for (int i = 0; i < 4; ++i)
	{
		int x = rect[i].x * 2;
		int y = rect[i].y * 2;

		left = x < left ? x : left;
		right = right < x ? x : right;
		top = y < top ? y : top;
		bottom = bottom < y ? y : bottom;
	}

	left = left < 0 ? 0 : left;
	right = MAP_TILE_X_MAX - 1 < right ? MAP_TILE_X_MAX - 1 : right;
	top = top < 0 ? 0 : top;
	bottom = MAP_TILE_Y_MAX - 1 < bottom ? MAP_TILE_Y_MAX - 1 : bottom;

	left = left / TILES_PER_SECTOR;
	right = right / TILES_PER_SECTOR;
	top = top / TILES_PER_SECTOR;
	bottom = bottom / TILES_PER_SECTOR;

	std::vector<Sector*> sectors;

	sectors.push_back(&mSectors[top][left]);
	if (left != right)
	{
		sectors.push_back(&mSectors[top][right]);
		if (top != bottom)
		{
			sectors.push_back(&mSectors[bottom][left]);
			sectors.push_back(&mSectors[bottom][right]);
		}
	}
	else if (top != bottom)
	{
		sectors.push_back(&mSectors[bottom][left]);
	}

	for (Sector* sector : sectors)
	{
		const std::unordered_map<INT64, Object*>* objs = sector->GetObjects();

		for (auto iter : *objs)
		{
			Object* other = iter.second;

			if (other->ClientID != obj->ClientID && other->ObjectType == targetType && IsPointInRectangle(rect, { other->PosX, other->PosY }))
			{
				outObjects->push_back(other);
			}
		}
	}
}

unsigned int GameThread::GetNumPlayer() const
{
	return mNumPlayer;
}

void GameThread::processMovePacket(Player* target, NetworkLib::Message* message)
{
	if (target->bIsDie)
	{
		return;
	}
	if (target->bIsSit)
	{
		onStand(target);
	}

	INT64 clientId;
	float x;
	float y;
	USHORT rotation;
	BYTE vKey;
	BYTE hKey;

	*message >> clientId >> x >> y >> rotation >> vKey >> hKey;

	target->PosX = x;
	target->PosY = y;

	int sectorXBefore = target->TileX / TILES_PER_SECTOR;
	int sectorYBefore = target->TileY / TILES_PER_SECTOR;

	target->TileX = x * 2;
	target->TileY = y * 2;
	target->Rotation = rotation;

	int sectorX = target->TileX / TILES_PER_SECTOR;
	int sectorY = target->TileY / TILES_PER_SECTOR;

	if (sectorXBefore != sectorX || sectorYBefore != sectorY)
	{
		Sector* currentSector = &mSectors[sectorY][sectorX];
		Sector* beforeSector = &mSectors[sectorYBefore][sectorXBefore];

		beforeSector->RemoveObject(clientId);
		currentSector->InsertObject(target);

		onSectorMove(target, sectorXBefore, sectorYBefore, sectorX, sectorY);
	}

	NetworkLib::Message* sectorMoveRes = AllocMessage();
	{
		*sectorMoveRes << (WORD)en_PACKET_CS_GAME_RES_MOVE_CHARACTER;
		*sectorMoveRes << clientId;
		*sectorMoveRes << x;
		*sectorMoveRes << y;
		*sectorMoveRes << rotation;
		*sectorMoveRes << vKey;
		*sectorMoveRes << hKey;

		sendAroundSector(sectorX, sectorY, sectorMoveRes, target->SessionID);
	}
	ReleaseMessage(sectorMoveRes);
}

void GameThread::processMoveStopPacket(Player* target, NetworkLib::Message* message)
{
	if (target->bIsDie)
	{
		return;
	}
	if (target->bIsSit)
	{
		onStand(target);
	}

	INT64 clientId;
	float x;
	float y;
	USHORT rotation;

	*message >> clientId >> x >> y >> rotation;

	target->PosX = x;
	target->PosY = y;

	int sectorXBefore = target->TileX / TILES_PER_SECTOR;
	int sectorYBefore = target->TileY / TILES_PER_SECTOR;

	target->TileX = x * 2;
	target->TileY = y * 2;
	target->Rotation = rotation;

	int sectorX = target->TileX / TILES_PER_SECTOR;
	int sectorY = target->TileY / TILES_PER_SECTOR;

	if (sectorXBefore != sectorX || sectorYBefore != sectorY)
	{
		Sector* currentSector = &mSectors[sectorY][sectorX];
		Sector* beforeSector = &mSectors[sectorYBefore][sectorXBefore];

		beforeSector->RemoveObject(clientId);
		currentSector->InsertObject(target);

		onSectorMove(target, sectorXBefore, sectorYBefore, sectorX, sectorY);
	}

	NetworkLib::Message* sectorMoveStopRes = AllocMessage();
	{
		*sectorMoveStopRes << (WORD)en_PACKET_CS_GAME_RES_STOP_CHARACTER;
		*sectorMoveStopRes << clientId;
		*sectorMoveStopRes << x;
		*sectorMoveStopRes << y;
		*sectorMoveStopRes << rotation;

		sendAroundSector(sectorX, sectorY, sectorMoveStopRes, target->SessionID);
	}
	ReleaseMessage(sectorMoveStopRes);
}

void GameThread::processAttackPacket(Player* target, NetworkLib::Message* message, int damage, WORD type)
{
	if (target->bIsDie)
	{
		return;
	}
	if (target->bIsSit)
	{
		onStand(target);
	}
	std::vector<Object*> objects;

	INT64 clientID;

	*message >> clientID;

	NetworkLib::Message* resAttack = AllocMessage();

	*resAttack << type;
	*resAttack << clientID;

	int sectorX = target->TileX / TILES_PER_SECTOR;
	int sectorY = target->TileY / TILES_PER_SECTOR;

	sendAroundSector(sectorX, sectorY, resAttack, target->SessionID);

	ReleaseMessage(resAttack);

	getObjectsInInteractionRange(target, &objects, EObjectType::OBJECT_TYPE_MONSTER);

	for (Object* obj : objects)
	{
		Monster* monster = (Monster*)obj;

		monster->Target = target;

		int monSectorX = monster->TileX / TILES_PER_SECTOR;
		int monSectorY = monster->TileY / TILES_PER_SECTOR;

		
		monster->HP -= damage;

		NetworkLib::Message* resDamage = AllocMessage();
		{
			*resDamage << (WORD)en_PACKET_CS_GAME_RES_DAMAGE;
			*resDamage << target->ClientID;
			*resDamage << monster->ClientID;
			*resDamage << (int)damage;

			sendAroundSector(monSectorX, monSectorY, resDamage, 0);
		}
		ReleaseMessage(resDamage);

		if (monster->HP <= 0)
		{
			target->Exp += 10;

			Cristal* cristal = mCristalPool.GetObject();
			{
				cristal->ClientID = ClientIDGenerator::Generate();
				cristal->PosX = monster->PosX;
				cristal->PosY = monster->PosY;
				cristal->TileX = cristal->PosX * 2;
				cristal->TileY = cristal->PosY * 2;
				cristal->Amount = 10;
				cristal->ObjectType = EObjectType::OBJECT_TYPE_CRISTAL;
				cristal->Rotation = 0;
			}

			NetworkLib::Message* resMonsterDie = AllocMessage();
			{
				*resMonsterDie << (WORD)en_PACKET_CS_GAME_RES_MONSTER_DIE;
				*resMonsterDie << monster->ClientID;

				sendAroundSector(monSectorX, monSectorY, resMonsterDie, 0);
			}
			ReleaseMessage(resMonsterDie);

			NetworkLib::Message* resCreateCristal = AllocMessage();
			{
				*resCreateCristal << (WORD)en_PACKET_CS_GAME_RES_CREATE_CRISTAL;
				*resCreateCristal << cristal->ClientID;
				*resCreateCristal << (BYTE)1;
				*resCreateCristal << cristal->PosX;
				*resCreateCristal << cristal->PosY;

				sendAroundSector(monSectorX, monSectorY, resCreateCristal, 0);
			}
			ReleaseMessage(resCreateCristal);

			mSectors[monSectorY][monSectorX].RemoveObject(monster->ClientID);
			mSectors[monSectorY][monSectorX].InsertObject(cristal);

			QueryPerformanceCounter(&monster->DeadTime);

			mMonsters.erase(monster->ClientID);
			mDeadMonsters.push(monster);
		}
	}

}

void GameThread::processPickPacket(Player* target, NetworkLib::Message* message)
{
	if (target->bIsDie)
	{
		return;
	}
	if (target->bIsSit)
	{
		onStand(target);
	}
	std::vector<Object*> objects;
	INT64 clientID;

	*message >> clientID;

	NetworkLib::Message* resPick = AllocMessage();

	*resPick << (WORD)en_PACKET_CS_GAME_RES_PICK;
	*resPick << clientID;

	int sectorX = target->TileX / TILES_PER_SECTOR;
	int sectorY = target->TileY / TILES_PER_SECTOR;

	sendAroundSector(sectorX, sectorY, resPick, target->SessionID);

	ReleaseMessage(resPick);

	getObjectsInInteractionRange(target, &objects, EObjectType::OBJECT_TYPE_CRISTAL);

	for (Object* obj : objects)
	{
		Cristal* cristal = (Cristal*)obj;

		Logger::AppendLine(L"Player[X: %f, Y: %f, ROT: %d] Pick Cristal[X: %f, Y:%f]", target->PosX, target->PosY, target->Rotation, cristal->PosX, cristal->PosY);;
		Logger::Log(LOG_FILE_NAME);

		target->Cristal += cristal->Amount;

		int cristalX = cristal->TileX / TILES_PER_SECTOR;
		int cristalY = cristal->TileY / TILES_PER_SECTOR;

		mSectors[cristalY][cristalX].RemoveObject(cristal->ClientID);

		NetworkLib::Message* resCristal = AllocMessage();

		*resCristal << (WORD)en_PACKET_CS_GAME_RES_PICK_CRISTAL;
		*resCristal << clientID;
		*resCristal << cristal->ClientID;
		*resCristal << target->Cristal;

		sendAroundSector(sectorX, sectorY, resCristal, 0);

		ReleaseMessage(resCristal);

		mCristalPool.ReleaseObject(cristal);

		mLogDBWriter->Write("INSERT INTO `logdb`.`gamelog`(type, code, accountno, servername, param1, param2) VALUES(4, 41, %lld, \"game\", %d, %d)",
			target->AccountNo, 10, target->Cristal);
	}
}

void GameThread::processSitPacket(Player* target, NetworkLib::Message* message)
{
	if (target->bIsDie)
	{
		return;
	}
	onSit(target);
}

void GameThread::processRestartPacket(Player* target, NetworkLib::Message* message)
{
	if (!target->bIsDie)
	{
		return;
	}
	int sectorX = target->TileX / TILES_PER_SECTOR;
	int sectorY = target->TileY / TILES_PER_SECTOR;

	Sector* sector = &mSectors[sectorY][sectorX];

	NetworkLib::Message* resRemoveObj = AllocMessage();
	{
		*resRemoveObj << (WORD)en_PACKET_CS_GAME_RES_REMOVE_OBJECT;
		*resRemoveObj << target->ClientID;

		sendAroundSector(sectorX, sectorY, resRemoveObj, target->SessionID);
	}
	ReleaseMessage(resRemoveObj);

	sector->RemoveObject(target->ClientID);

	NetworkLib::Message* resRestart = AllocMessage();
	{
		*resRestart << (WORD)en_PACKET_CS_GAME_RES_PLAYER_RESTART;

		TrySendMessage(target->SessionID, resRestart);
	}
	ReleaseMessage(resRestart);

	target->TileX = 85 + (rand() % 21);
	target->TileY = 93 + (rand() % 31);
	target->Rotation = rand() % 360;
	target->PosX = target->TileX / 2;
	target->PosY = target->TileY / 2;
	target->HP = 5000;
	target->bIsSit = false;
	target->bIsDie = false;

	onPlayerSpawn(target);

	mLogDBWriter->Write("INSERT INTO `logdb`.`gamelog`(type, code, accountno, servername, param1, param2) VALUES(`3`, `33`, `%lld`, \"game\", %d, %d)",
		target->AccountNo, target->TileX, target->TileY);
}

void GameThread::moveMonster(Monster* monster, float toX, float toY)
{
	QueryPerformanceCounter(&monster->LastActionTime);

	float monPosX = monster->PosX;
	float monPosY = monster->PosY;

	int monSectorX = monster->TileX / TILES_PER_SECTOR;
	int monSectorY = monster->TileY / TILES_PER_SECTOR;

	double dx = toX - monPosX + 1.0;
	double dy = toY - monPosY + 1.0;
	double radians = atan2(dy, dx);
	double degrees = radians * 180.0 / PI;

	if (toX < monPosX)
	{
		degrees += 180 - degrees;
	}

	if (degrees < 0)
	{
		degrees += 360.0;
	}
	monster->Rotation = (USHORT)degrees;

	double x = toX - monPosX;
	double y = toY - monPosY;

	double magnitude = sqrt(x * x + y * y);

	if (magnitude == 0)
	{
		return;
	}

	double unitX = x / magnitude;
	double unitY = y / magnitude;

	if (abs(monPosX - toX) < abs(unitX))
	{
		monster->PosX = toX;
	}
	else
	{
		monster->PosX += unitX;
	}

	if (abs(monPosY - toY) < abs(unitY))
	{
		monster->PosY = toY;
	}
	else
	{
		monster->PosY += unitY;
	}

	monster->TileX = monster->PosX * 2;
	monster->TileY = monster->PosY * 2;

	int toSectorX = (int)monster->TileX / TILES_PER_SECTOR;
	int toSectorY = (int)monster->TileY / TILES_PER_SECTOR;

	NetworkLib::Message* resMonMove = AllocMessage();
	{
		*resMonMove << (WORD)en_PACKET_CS_GAME_RES_MOVE_MONSTER;
		*resMonMove << monster->ClientID;
		*resMonMove << monster->PosX;
		*resMonMove << monster->PosY;
		*resMonMove << monster->Rotation;

		sendAroundSector(monSectorX, monSectorY, resMonMove, 0);
	}
	ReleaseMessage(resMonMove);

	if (monSectorX != toSectorX || monSectorY != toSectorY)
	{
		Sector* currentSector = &mSectors[toSectorY][toSectorX];
		Sector* beforeSector = &mSectors[monSectorY][monSectorX];

		beforeSector->RemoveObject(monster->ClientID);
		currentSector->InsertObject(monster);

		onSectorMove(monster, monSectorX, monSectorY, toSectorX, toSectorY);
	}
}
