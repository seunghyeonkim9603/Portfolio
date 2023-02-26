#include "stdafx.h"

void InitMonster(Monster* monster, int HP, int AP, int centerTileX, int centerTileY, int moveRange)
{
	monster->ClientID = ClientIDGenerator::Generate();
	monster->ObjectType = EObjectType::OBJECT_TYPE_MONSTER;
	monster->ObjectStatus = OBJECT_STATUS_STOP;
	monster->CenterTileX = centerTileX;
	monster->CenterTileY = centerTileY;
	monster->Rotation = 0;
	monster->Target = nullptr;
	monster->MoveRange = moveRange;

	QueryPerformanceCounter(&monster->LastActionTime);

	InitMonsterPos(monster);
}

void InitMonsterPos(Monster* monster)
{
	int range = monster->MoveRange / 2;

	monster->HP = 50;
	monster->TileX = (monster->CenterTileX - range) + (rand() % monster->MoveRange);
	monster->TileY = (monster->CenterTileY - range) + (rand() % monster->MoveRange);
	monster->PosX = (float)monster->TileX / 2;
	monster->PosY = (float)monster->TileY / 2;
	monster->MoveXMax = monster->CenterTileX + range;
	monster->MoveXMin = monster->CenterTileX - range;
	monster->MoveYMax = monster->CenterTileY + range;
	monster->MoveYMin = monster->CenterTileY - range;
}