#pragma once

class Sector
{
public:
	Sector();
	~Sector();

	void InsertObject(Object* obj);
	void RemoveObject(INT64 clientId);
	void RegisterAdjSector(Sector* sector);

	const std::unordered_map<INT64, Object*>* GetObjects() const;
	const std::vector<Sector*>* GetAdjSectors() const;
	void SetX(int x);
	void SetY(int y);
	int GetX() const;
	int GetY() const;
	unsigned int GetNumObject() const;

private:
	std::unordered_map<INT64, Object*> mHashMap;
	std::vector<Sector*> mAdjSectors;

	int mX;
	int mY;

	unsigned int mNumObject;
};