#include <Windows.h>
#include <process.h>
#include <unordered_map>
#include <queue>
#include <stack>

#define CACHE_ALIGN __declspec(align(64))
#define PROFILE

#include "Profiler.h"
#include "ProfilerManager.h"
#include "ObjectPool.h"
#include "LockFreeQueue.h"
#include "LockFreeStack.h"

#define NUM_THREADS (16)
#define LOOP_COUNT (100)
#define NUM_POP (1000)
#define NUM_PUSH (NUM_POP)

std::queue<int> gQueue;
std::stack<int> gStack;
LockFreeQueue<int> gLockFreeQueue;
LockFreeStack<int> gLockFreeStack;

SRWLOCK gSRWLock;
INT16 gSpinLockFlag = 0;

unsigned int __stdcall DummyThread(void* param);
unsigned int __stdcall SRWLockQueueTest(void* param);
unsigned int __stdcall SRWLockStackTest(void* param);
unsigned int __stdcall SpinLockQueueTest(void* param);
unsigned int __stdcall SpinLockStackTest(void* param);
unsigned int __stdcall LockFreeQueueTest(void* param);
unsigned int __stdcall LockFreeStackTest(void* param);

void Lock();
void Unlock();

int main(void)
{
	InitializeSRWLock(&gSRWLock);

	WCHAR saveFileName[128];
	wsprintf(saveFileName, L"Lock-free_THREADS%d.txt", NUM_THREADS);

	HANDLE hThreads[NUM_THREADS];

	for (unsigned int i = 0; i < NUM_THREADS * 2; ++i)
	{
		_beginthreadex(nullptr, 0, DummyThread, nullptr, 0, nullptr);
	}

	for (unsigned int i = 0; i < NUM_THREADS; ++i)
	{
		hThreads[i] = (HANDLE)_beginthreadex(nullptr, 0, SRWLockQueueTest, nullptr, 0, nullptr);
	}
	WaitForMultipleObjects(NUM_THREADS, hThreads, true, INFINITE);

	for (unsigned int i = 0; i < NUM_THREADS; ++i)
	{
		CloseHandle(hThreads[i]);
	}

	for (unsigned int i = 0; i < NUM_THREADS; ++i)
	{
		hThreads[i] = (HANDLE)_beginthreadex(nullptr, 0, SRWLockStackTest, nullptr, 0, nullptr);
	}
	WaitForMultipleObjects(NUM_THREADS, hThreads, true, INFINITE);

	for (unsigned int i = 0; i < NUM_THREADS; ++i)
	{
		CloseHandle(hThreads[i]);
	}

	for (unsigned int i = 0; i < NUM_THREADS; ++i)
	{
		hThreads[i] = (HANDLE)_beginthreadex(nullptr, 0, SpinLockQueueTest, nullptr, 0, nullptr);
	}
	WaitForMultipleObjects(NUM_THREADS, hThreads, true, INFINITE);

	for (unsigned int i = 0; i < NUM_THREADS; ++i)
	{
		CloseHandle(hThreads[i]);
	}

	for (unsigned int i = 0; i < NUM_THREADS; ++i)
	{
		hThreads[i] = (HANDLE)_beginthreadex(nullptr, 0, SpinLockStackTest, nullptr, 0, nullptr);
	}
	WaitForMultipleObjects(NUM_THREADS, hThreads, true, INFINITE);

	for (unsigned int i = 0; i < NUM_THREADS; ++i)
	{
		CloseHandle(hThreads[i]);
	}

	for (unsigned int i = 0; i < NUM_THREADS; ++i)
	{
		hThreads[i] = (HANDLE)_beginthreadex(nullptr, 0, LockFreeQueueTest, nullptr, 0, nullptr);
	}
	WaitForMultipleObjects(NUM_THREADS, hThreads, true, INFINITE);

	for (unsigned int i = 0; i < NUM_THREADS; ++i)
	{
		CloseHandle(hThreads[i]);
	}

	for (unsigned int i = 0; i < NUM_THREADS; ++i)
	{
		hThreads[i] = (HANDLE)_beginthreadex(nullptr, 0, LockFreeStackTest, nullptr, 0, nullptr);
	}
	WaitForMultipleObjects(NUM_THREADS, hThreads, true, INFINITE);

	for (unsigned int i = 0; i < NUM_THREADS; ++i)
	{
		CloseHandle(hThreads[i]);
	}

	PROFILES_PRINT(saveFileName);

	return 0;
}

unsigned int __stdcall SRWLockQueueTest(void* param)
{
	for (int i = 0; i < LOOP_COUNT; ++i)
	{
		PROFILE_BEGIN(L"SRWLock Multiple Queue Push");
		AcquireSRWLockExclusive(&gSRWLock);
		for (int j = 0; j < NUM_PUSH; ++j)
		{
			gQueue.push(1);
		}
		ReleaseSRWLockExclusive(&gSRWLock);
		PROFILE_END(L"SRWLock Multiple Queue Push");

		PROFILE_BEGIN(L"SRWLock Multiple Queue Pop");
		AcquireSRWLockExclusive(&gSRWLock);
		for (int j = 0; j < NUM_POP; ++j)
		{
			gQueue.pop();
		}
		ReleaseSRWLockExclusive(&gSRWLock);
		PROFILE_END(L"SRWLock Multiple Queue Pop");
	}
	return 0;
}

unsigned int __stdcall SRWLockStackTest(void* param)
{
	for (int i = 0; i < LOOP_COUNT; ++i)
	{
		PROFILE_BEGIN(L"SRWLock Multiple Stack Push");
		AcquireSRWLockExclusive(&gSRWLock);
		for (int j = 0; j < NUM_PUSH; ++j)
		{
			gStack.push(1);
		}
		ReleaseSRWLockExclusive(&gSRWLock);
		PROFILE_END(L"SRWLock Multiple Stack Push");

		PROFILE_BEGIN(L"SRWLock Multiple Stack Pop");
		AcquireSRWLockExclusive(&gSRWLock);
		for (int j = 0; j < NUM_POP; ++j)
		{
			gStack.pop();
		}
		ReleaseSRWLockExclusive(&gSRWLock);
		PROFILE_END(L"SRWLock Multiple Stack Pop");
	}
	return 0;
}

unsigned int __stdcall SpinLockQueueTest(void* param)
{
	for (int i = 0; i < LOOP_COUNT; ++i)
	{
		PROFILE_BEGIN(L"SpinLock Multiple Queue Push");
		Lock();
		for (int j = 0; j < NUM_PUSH; ++j)
		{
			gQueue.push(1);
		}
		Unlock();
		PROFILE_END(L"SpinLock Multiple Queue Push");

		PROFILE_BEGIN(L"SpinLock Multiple Queue Pop");
		Lock();
		for (int j = 0; j < NUM_POP; ++j)
		{
			gQueue.pop();
		}
		Unlock();
		PROFILE_END(L"SpinLock Multiple Queue Pop");
	}
	return 0;
}

unsigned int __stdcall DummyThread(void* param)
{
	int i = 0;
	while (true)
	{
		++i;
	}
}
unsigned int __stdcall SpinLockStackTest(void* param)
{
	for (int i = 0; i < LOOP_COUNT; ++i)
	{
		PROFILE_BEGIN(L"SpinLock Multiple Stack Push");
		Lock();
		for (int j = 0; j < NUM_PUSH; ++j)
		{
			gStack.push(1);
		}
		Unlock();
		PROFILE_END(L"SpinLock Multiple Stack Push");

		PROFILE_BEGIN(L"SpinLock Multiple Stack Pop");
		Lock();
		for (int j = 0; j < NUM_POP; ++j)
		{
			gStack.pop();
		}
		Unlock();
		PROFILE_END(L"SpinLock Multiple Stack Pop");
	}
	return 0;
}

unsigned int __stdcall LockFreeQueueTest(void* param)
{
	int outData;

	for (int i = 0; i < LOOP_COUNT; ++i)
	{
		PROFILE_BEGIN(L"Lock-free Queue Push");
		for (int j = 0; j < NUM_PUSH; ++j)
		{
			gLockFreeQueue.Enqueue(1);
		}
		PROFILE_END(L"Lock-free Queue Push");

		PROFILE_BEGIN(L"Lock-free Queue Pop");
		for (int j = 0; j < NUM_POP; ++j)
		{
			gLockFreeQueue.Dequeue(&outData);
		}
		PROFILE_END(L"Lock-free Queue Pop");
	}
	return 0;
}

unsigned int __stdcall LockFreeStackTest(void* param)
{
	int outData;

	for (int i = 0; i < LOOP_COUNT; ++i)
	{
		PROFILE_BEGIN(L"Lock-free Stack Push");
		for (int j = 0; j < NUM_PUSH; ++j)
		{
			gLockFreeStack.Push(1);
		}
		PROFILE_END(L"Lock-free Stack Push");

		PROFILE_BEGIN(L"Lock-free Stack Pop");
		for (int j = 0; j < NUM_POP; ++j)
		{
			gLockFreeStack.Pop(&outData);
		}
		PROFILE_END(L"Lock-free Stack Pop");
	}
	return 0;
}

void Lock()
{
	while (InterlockedCompareExchange16(&gSpinLockFlag, 1, 0) != 0)
	{
		Sleep(0);
	}
}

void Unlock()
{
	gSpinLockFlag = 0;
}
