#pragma once

struct Object
{
	EObjectType		ObjectType;
	EObjectStatus	ObjectStatus;
	INT64			ClientID;
	float			PosX;
	float			PosY;
	int				TileX;
	int				TileY;
	USHORT			Rotation;
};