#pragma once


template<typename T>
class Chunk final
{
public:
	Chunk();
	~Chunk();
	void	Fill();
	T* Pop();
	void	Push(T* value);
	bool	IsFull() const;
	bool	IsEmpty() const;

	static unsigned int GetActiveCount();

private:
	enum { CHUNK_SIZE = 512 };

	static unsigned int ActiveCount;

	unsigned int	mTop;
	T* mValues[CHUNK_SIZE];
};

template<typename T>
unsigned int Chunk<T>::ActiveCount = 0;

template<typename T>
inline Chunk<T>::Chunk()
	: mTop(0)
{
}

template<typename T>
inline Chunk<T>::~Chunk()
{
	for (unsigned int i = 0; i < mTop; ++i)
	{
		delete mValues[i];
	}
}

template<typename T>
inline void Chunk<T>::Fill()
{
	while (mTop < CHUNK_SIZE)
	{
		mValues[mTop] = new T();
		++mTop;
	}
}

template<typename T>
inline T* Chunk<T>::Pop()
{
	//InterlockedIncrement(&ActiveCount);
	return mValues[--mTop];
}

template<typename T>
inline void Chunk<T>::Push(T* value)
{
	//InterlockedDecrement(&ActiveCount);
	mValues[mTop++] = value;
}

template<typename T>
inline bool Chunk<T>::IsFull() const
{
	if (mTop == CHUNK_SIZE)
	{
		return true;
	}
	return false;
}

template<typename T>
inline bool Chunk<T>::IsEmpty() const
{
	if (mTop == 0)
	{
		return true;
	}
	return false;
}

template<typename T>
inline unsigned int Chunk<T>::GetActiveCount()
{
	return ActiveCount;
}
