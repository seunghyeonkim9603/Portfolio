#include "stdafx.h"

unsigned int ClientIDGenerator::mGen;

unsigned int ClientIDGenerator::Generate()
{
	return InterlockedIncrement(&mGen) - 1;
}
