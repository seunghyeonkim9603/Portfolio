#include <Windows.h>
#include <process.h>
#include <unordered_map>
#include <iostream>

#define PROFILE

#include "Profiler.h"
#include "ProfilerManager.h"

#include "Chunk.h"
#include "LockFreeStack.h"
#include "LockFreeQueue.h"
#include "ObjectPool.h"
#include "TLSObjectPoolMiddleware.h"
#include "TLSObjectPool.h"

#define NUM_THREADS (2)
#define ALLOC_SIZE (32)
#define ALLOC_COUNT (1000)
#define LOOP_COUNT (10000)

#define MAX_STR_LENGTH (256)

template<size_t size>
struct MemoryDummy
{
	char Dummy[size];
};

ObjectPool<MemoryDummy<ALLOC_SIZE>> gLockFreePool;
TLSObjectPool<MemoryDummy<ALLOC_SIZE>> gTLSPool;

unsigned int __stdcall NewDeleteTest(void* param);
unsigned int __stdcall LockFreeObjectPoolTest(void* param);
unsigned int __stdcall TLSObjectPoolTest(void* param);

int main(void)
{
	WCHAR saveFileName[MAX_STR_LENGTH];
	wsprintf(saveFileName, L"MemAllocTest_SIZE%d_THREADS%d.txt", ALLOC_SIZE, NUM_THREADS);

	HANDLE hThreads[NUM_THREADS];

	for (unsigned int i = 0; i < NUM_THREADS; ++i)
	{
		hThreads[i] = (HANDLE)_beginthreadex(nullptr, 0, NewDeleteTest, nullptr, 0, nullptr);
	}
	WaitForMultipleObjects(NUM_THREADS, hThreads, true, INFINITE);

	for (unsigned int i = 0; i < NUM_THREADS; ++i)
	{
		CloseHandle(hThreads[i]);
	}

	for (unsigned int i = 0; i < NUM_THREADS; ++i)
	{
		hThreads[i] = (HANDLE)_beginthreadex(nullptr, 0, LockFreeObjectPoolTest, nullptr, 0, nullptr);
	}
	WaitForMultipleObjects(NUM_THREADS, hThreads, true, INFINITE);

	for (unsigned int i = 0; i < NUM_THREADS; ++i)
	{
		CloseHandle(hThreads[i]);
	}

	for (unsigned int i = 0; i < NUM_THREADS; ++i)
	{
		hThreads[i] = (HANDLE)_beginthreadex(nullptr, 0, TLSObjectPoolTest, nullptr, 0, nullptr);
	}
	WaitForMultipleObjects(NUM_THREADS, hThreads, true, INFINITE);

	for (unsigned int i = 0; i < NUM_THREADS; ++i)
	{
		CloseHandle(hThreads[i]);
	}

	PROFILES_PRINT(saveFileName);

	return 0;
}

unsigned int __stdcall NewDeleteTest(void* param)
{
	MemoryDummy<ALLOC_SIZE>* dummies[ALLOC_COUNT];

	for (unsigned int i = 0; i < LOOP_COUNT; ++i)
	{
		PROFILE_BEGIN(L"NEW MULTIPLES");
		for (unsigned int j = 0; j < ALLOC_COUNT; ++j)
		{
			dummies[j] = new MemoryDummy<ALLOC_SIZE>();
		}
		PROFILE_END(L"NEW MULTIPLES");

		PROFILE_BEGIN(L"DELETE MULTIPLES");
		for (unsigned int j = 0; j < ALLOC_COUNT; ++j)
		{
			delete dummies[j];
		}
		PROFILE_END(L"DELETE MULTIPLES");
	}
	return 0;
}

unsigned int __stdcall LockFreeObjectPoolTest(void* param)
{
	MemoryDummy<ALLOC_SIZE>* dummies[ALLOC_COUNT];

	for (unsigned int i = 0; i < LOOP_COUNT; ++i)
	{
		PROFILE_BEGIN(L"LOCK-FREE ALLOC MULTIPLES");
		for (unsigned int j = 0; j < ALLOC_COUNT; ++j)
		{
			dummies[j] = gLockFreePool.GetObject();
		}
		PROFILE_END(L"LOCK-FREE ALLOC MULTIPLES");

		PROFILE_BEGIN(L"LOCK-FREE RELEASE MULTIPLES");
		for (unsigned int j = 0; j < ALLOC_COUNT; ++j)
		{
			gLockFreePool.ReleaseObject(dummies[j]);
		}
		PROFILE_END(L"LOCK-FREE RELEASE MULTIPLES");
	}
	return 0;
}

unsigned int __stdcall TLSObjectPoolTest(void* param)
{
	MemoryDummy<ALLOC_SIZE>* dummies[ALLOC_COUNT];

	for (unsigned int i = 0; i < LOOP_COUNT; ++i)
	{
		PROFILE_BEGIN(L"TLS ALLOC MULTIPLES");
		for (unsigned int j = 0; j < ALLOC_COUNT; ++j)
		{
			dummies[j] = gTLSPool.GetObject();
		}
		PROFILE_END(L"TLS ALLOC MULTIPLES");

		PROFILE_BEGIN(L"TLS RELEASE MULTIPLES");
		for (unsigned int j = 0; j < ALLOC_COUNT; ++j)
		{
			gTLSPool.ReleaseObject(dummies[j]);
		}
		PROFILE_END(L"TLS RELEASE MULTIPLES");
	}
	return 0;
}