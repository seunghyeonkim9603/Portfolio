#pragma once

#define PLAYER_HIT_BOX_X (0.5f)
#define PLAYER_HIT_BOX_Y (0.5f)

struct Player : public Object
{
	sessionID_t		SessionID;
	INT64			AccountNo;
	ECharacterType	CharacterType;
	WCHAR			Nickname[20];
	int				Cristal;
	int				HP;
	INT64			Exp;
	USHORT			Level;
	bool			bIsSit;
	bool			bIsDie;
	bool			bIsAuthSucceed;
	LARGE_INTEGER	LastRecvedTime;
	LARGE_INTEGER	SitBeginTime;
};