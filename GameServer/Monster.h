#pragma once


struct Monster : public Object
{
	int HP;
	int CenterTileX;
	int CenterTileY;
	int MoveRange;
	int MoveXMax;
	int MoveXMin;
	int MoveYMax;
	int MoveYMin;
	Player* Target;
	LARGE_INTEGER DeadTime;
	LARGE_INTEGER LastActionTime;
};

void InitRespawnMonster(Monster* monster);
void InitMonster(Monster* monster, int HP, int AP, int centerTileX, int centerTileY, int moveRange);
