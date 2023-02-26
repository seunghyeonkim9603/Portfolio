#include "stdafx.h"

Sector::Sector()
	: mNumObject(0)
{
}

Sector::~Sector()
{
}

void Sector::InsertObject(Object* obj)
{
	mHashMap.insert({ obj->ClientID, obj });
	++mNumObject;
}

void Sector::RemoveObject(INT64 clientId)
{
	mHashMap.erase(clientId);
	--mNumObject;
}

void Sector::RegisterAdjSector(Sector* sector)
{
	mAdjSectors.push_back(sector);
}

const std::unordered_map<INT64, Object*>* Sector::GetObjects() const
{
	return &mHashMap;
}

const std::vector<Sector*>* Sector::GetAdjSectors() const
{
	return &mAdjSectors;
}

void Sector::SetX(int x)
{
	mX = x;
}

void Sector::SetY(int y)
{
	mY = y;
}

int Sector::GetX() const
{
	return mX;
}

int Sector::GetY() const
{
	return mY;
}

unsigned int Sector::GetNumObject() const
{
	return mNumObject;
}


