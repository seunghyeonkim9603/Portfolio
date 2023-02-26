#pragma once

#include "ObjectPool.h"

namespace NetworkLib
{
	template<typename T>
	class LockFreeStack final
	{
		struct Node
		{
			T Data;
			Node* Next;
		};
	public:
		LockFreeStack();
		~LockFreeStack();

		void			Push(T data);
		bool			Pop(T* outData);
		int				GetSize() const;

	private:
		ObjectPool<Node>	mPool;
		Node*				mTop;
		int					mSize;
	};

	template<typename T>
	inline LockFreeStack<T>::LockFreeStack()
		: mPool(),
		mTop(nullptr),
		mSize(0)
	{
	}

	template<typename T>
	inline LockFreeStack<T>::~LockFreeStack()
	{
		Node* cur;
		Node* next;

		cur = REMOVE_OP_COUNT_FROM(mTop);

		while (cur != nullptr)
		{
			next = cur->Next;
			mPool.ReleaseObject(cur);
			cur = next;
		}
	}

	template<typename T>
	inline void LockFreeStack<T>::Push(T data)
	{
		Node* newTop = mPool.GetObject();
		Node* oldTop;

		newTop->Data = data;

		while (true)
		{
			oldTop = mTop;
			newTop->Next = REMOVE_OP_COUNT_FROM(oldTop);

			if (InterlockedCompareExchangePointer((PVOID*)&mTop, MAKE_TOP(newTop, EXTRACT_OP_COUNT_FROM(oldTop) + 1), oldTop) == oldTop)
			{
				break;
			}
		}
		InterlockedIncrement((long*)&mSize);
	}

	template<typename T>
	inline bool LockFreeStack<T>::Pop(T* outData)
	{
		Node* oldTop;
		Node* newTop;

		if (InterlockedDecrement((long*)&mSize) < 0)
		{
			InterlockedIncrement((long*)&mSize);
			return false;
		}

		while (true)
		{
			oldTop = mTop;
			newTop = MAKE_TOP(REMOVE_OP_COUNT_FROM(oldTop)->Next, EXTRACT_OP_COUNT_FROM(oldTop) + 1);

			if (InterlockedCompareExchangePointer((PVOID*)&mTop, newTop, oldTop) == oldTop)
			{
				break;
			}
		}
		oldTop = REMOVE_OP_COUNT_FROM(oldTop);

		*outData = oldTop->Data;

		mPool.ReleaseObject(oldTop);

		return true;
	}

	template<typename T>
	inline int LockFreeStack<T>::GetSize() const
	{
		return mSize;
	}
}