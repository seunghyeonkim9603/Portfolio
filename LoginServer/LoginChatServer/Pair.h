#pragma once

template<typename T, typename U>
class FixedHashMap;

template<typename T, typename U>
class Pair final
{
	friend class FixedHashMap<T, U>;
public:
	Pair();
	~Pair();

	T GetKey() const;
	U GetValue() const;
private:
	T		mKey;
	U		mValue;
	size_t	mIndex;
	size_t	mHashCode;
};

template<typename T, typename U>
inline Pair<T, U>::Pair()
	: mKey((T)0),
	mValue((U)0),
	mIndex(0),
	mHashCode(0)
{
}

template<typename T, typename U>
inline Pair<T, U>::~Pair()
{
}

template<typename T, typename U>
inline T Pair<T, U>::GetKey() const
{
	return mKey;
}

template<typename T, typename U>
inline U Pair<T, U>::GetValue() const
{
	return mValue;
}

