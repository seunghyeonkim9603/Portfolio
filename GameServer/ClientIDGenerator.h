#pragma once

class ClientIDGenerator final
{
public:
	static unsigned int Generate();

private:
	static unsigned int mGen;
};