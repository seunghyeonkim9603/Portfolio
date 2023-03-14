#pragma warning( disable : 26495 )

#pragma once


#define MAKE_TOP(ptr, op_count) (reinterpret_cast<Node*>((LONG_PTR)(ptr) | ((op_count) << 48)))
#define REMOVE_OP_COUNT_FROM(ptr) (reinterpret_cast<Node*>((LONG_PTR)(ptr) & 0x00007FFFFFFFFFFF))
#define EXTRACT_OP_COUNT_FROM(ptr) (static_cast<int64_t>((LONG_PTR)(ptr) >> 48))

namespace NetworkLib
{
	template<typename T>
	class ObjectPool final
	{
	public:
		struct Node
		{
			T		Obj;
			Node* Next;
		};

	public:
		ObjectPool();
		ObjectPool(unsigned int size);
		ObjectPool(ObjectPool& other) = delete;
		ObjectPool(ObjectPool&& other) = delete;
		~ObjectPool();

		T*		GetObject();
		void	ReleaseObject(T* obj);

		unsigned int GetAllCount() const;
		unsigned int GetActiveCount() const;
		unsigned int GetInactiveCount() const;

	private:
		Node*			mTop;
		int				mSize;
		unsigned int	mAllCount;
	};


	template<typename T>
	inline ObjectPool<T>::ObjectPool()
		: ObjectPool(0)
	{
	}

	template<typename T>
	inline ObjectPool<T>::ObjectPool(unsigned int size)
		: mTop(nullptr),
		mSize(size),
		mAllCount(size)
	{
		for (size_t i = 0; i < size; ++i)
		{
			Node* newNode = new Node();
			{
				newNode->Next = mTop;
			}
			mTop = newNode;
		}
	}

	template<typename T>
	inline ObjectPool<T>::~ObjectPool()
	{
		Node* cur = REMOVE_OP_COUNT_FROM(mTop);
		Node* next;

		while (cur != nullptr)
		{
			next = cur->Next;
			delete cur;
			cur = next;
		}
	}

	template<typename T>
	inline T* ObjectPool<T>::GetObject()
	{
		Node* oldTop;
		Node* newTop;

		if (InterlockedDecrement((long*)&mSize) < 0)
		{
			InterlockedIncrement((long*)&mSize);
			InterlockedIncrement(&mAllCount);

			return  reinterpret_cast<T*>(new Node());
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

		return reinterpret_cast<T*>(oldTop);
	}

	template<typename T>
	inline void ObjectPool<T>::ReleaseObject(T* obj)
	{
		Node* oldTop;
		Node* newTop = reinterpret_cast<Node*>(obj);

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
	inline unsigned int ObjectPool<T>::GetAllCount() const
	{
		return mAllCount;
	}

	template<typename T>
	inline unsigned int ObjectPool<T>::GetActiveCount() const
	{
		return mAllCount - mSize;
	}

	template<typename T>
	inline unsigned int ObjectPool<T>::GetInactiveCount() const
	{
		return (unsigned int)mSize;
	}
}