#pragma once

#include "ObjectPool.h"


template<typename T>
class LockFreeQueue
{
private:
	struct Node
	{
		T Data;
		Node* Next;
	};

public:
	LockFreeQueue();
	~LockFreeQueue();

	void Enqueue(T data);
	bool Dequeue(T* outData);
	void Clear();
	bool IsEmpty() const;
	long GetSize() const;

private:
	ObjectPool<Node> mPool;

	CACHE_ALIGN Node* mHead;
	CACHE_ALIGN Node* mTail;

	CACHE_ALIGN long mSize;
};

template<typename T>
LockFreeQueue<T>::LockFreeQueue()
	: mPool(),
	mHead(nullptr),
	mTail(nullptr),
	mSize(0)
{
	mHead = mPool.GetObject();
	mTail = mHead;
	mHead->Next = nullptr;
}

template<typename T>
LockFreeQueue<T>::~LockFreeQueue()
{
	Node* head;
	Node* next;

	head = REMOVE_OP_COUNT_FROM(mHead);

	while (head != nullptr)
	{
		next = head->Next;

		mPool.ReleaseObject(head);
		head = next;
	}
}

template<typename T>
void LockFreeQueue<T>::Enqueue(T data)
{
	Node* newNode = mPool.GetObject();

	newNode->Data = data;
	newNode->Next = nullptr;

	while (true)
	{
		Node* tail = mTail;
		Node* next = REMOVE_OP_COUNT_FROM(tail)->Next;

		if (next == nullptr)
		{
			if (InterlockedCompareExchangePointer((PVOID*)&REMOVE_OP_COUNT_FROM(tail)->Next, newNode, next) == next)
			{
				InterlockedCompareExchangePointer((PVOID*)&mTail, MAKE_TOP(newNode, EXTRACT_OP_COUNT_FROM(tail) + 1), tail);
				break;
			}
		}
		else
		{
			InterlockedCompareExchangePointer((PVOID*)&mTail, MAKE_TOP(next, EXTRACT_OP_COUNT_FROM(tail) + 1), tail);
		}
	}
	InterlockedIncrement(&mSize);
}

template<typename T>
bool LockFreeQueue<T>::Dequeue(T* outData)
{
	if (InterlockedDecrement(&mSize) < 0)
	{
		InterlockedIncrement(&mSize);
		return false;
	}

	while (true)
	{
		Node* head = mHead;
		Node* tail = mTail;
		Node* next = REMOVE_OP_COUNT_FROM(head)->Next;

		if (next != nullptr)
		{
			if (REMOVE_OP_COUNT_FROM(head) == REMOVE_OP_COUNT_FROM(tail))
			{
				InterlockedCompareExchangePointer((PVOID*)&mTail, MAKE_TOP(next, EXTRACT_OP_COUNT_FROM(tail) + 1), tail);
			}
			*outData = next->Data;

			if (InterlockedCompareExchangePointer((PVOID*)&mHead, MAKE_TOP(next, EXTRACT_OP_COUNT_FROM(head) + 1), head) == head)
			{
				head = REMOVE_OP_COUNT_FROM(head);
				mPool.ReleaseObject(head);
				break;
			}
		}
	}
	return true;
}

template<typename T>
void LockFreeQueue<T>::Clear()
{
	Node* head;
	Node* tail;
	Node* next;

	head = REMOVE_OP_COUNT_FROM(mHead);
	tail = REMOVE_OP_COUNT_FROM(mTail);

	while (head != tail)
	{
		next = head->Next;

		mPool.ReleaseObject(head);
		head = next;
	}
	mSize = 0;
}

template<typename T>
bool LockFreeQueue<T>::IsEmpty() const
{
	if (mSize <= 0)
	{
		return true;
	}
	return false;
}

template<typename T>
long LockFreeQueue<T>::GetSize() const
{
	return mSize;
}
