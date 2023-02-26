#pragma once

#define MONSTER_HIT_BOX_X (0.5f)
#define MONSTER_HIT_BOX_Y (0.5f)

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

void InitMonsterPos(Monster* monster);
void InitMonster(Monster* monster, int HP, int AP, int centerTileX, int centerTileY, int moveRange);
