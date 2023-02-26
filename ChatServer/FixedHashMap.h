#pragma once

template<typename T, typename U>
class FixedHashMap final
{
	typedef size_t(*HASH_FUNC)(T);
public:
	FixedHashMap(const size_t bucketCount, const size_t bucketSize, HASH_FUNC function);
	~FixedHashMap();

	void			Insert(T key, U value);
	bool			Remove(T key);
	bool			Contains(T key) const;
	U				Find(T key) const;
	void			Clear();

	Pair<T, U>**	GetPairs() const;
	size_t			GetSize() const;

private:
	struct Bucket
	{
		size_t			NumPair;
		Pair<T, U>*		Pairs;
	};

	Bucket*			mBuckets;
	Pair<T, U>**	mPairs;
	size_t			mNumData;
	size_t			mBucketCount;
	size_t			mBucketSize;
	HASH_FUNC		mHashFunction;
};

template<typename T, typename U>
inline FixedHashMap<T, U>::FixedHashMap(const size_t bucketCount, const size_t bucketSize, HASH_FUNC function)
	: mBuckets(nullptr),
	mPairs(nullptr),
	mBucketCount(bucketCount),
	mBucketSize(bucketSize),
	mHashFunction(function),
	mNumData(0)
{
	mBuckets = new Bucket[bucketCount];
	mPairs = new Pair<T, U>*[bucketCount * bucketSize];

	for (size_t i = 0; i < bucketCount; ++i)
	{
		mBuckets[i].NumPair = 0;
		mBuckets[i].Pairs = new Pair<T, U>[bucketSize];
	}
}

template<typename T, typename U>
inline FixedHashMap<T, U>::~FixedHashMap()
{
}

template<typename T, typename U>
inline void FixedHashMap<T, U>::Insert(T key, U value)
{
	size_t hashCode = mHashFunction(key);
	size_t index = hashCode % mBucketCount;

	Bucket* bucket = &mBuckets[index];
	size_t numPair = bucket->NumPair;
	
	for (size_t i = 0; i < numPair; ++i)
	{
		Pair<T, U>* pair = &bucket->Pairs[i];

		if (pair->mHashCode == hashCode && pair->mKey == key)
		{
			bucket->Pairs[i].mValue = value;
			return;
		}
	}
	Pair<T, U>* inserted = &bucket->Pairs[numPair];

	inserted->mKey = key;
	inserted->mValue = value;
	inserted->mIndex = mNumData;
	inserted->mHashCode = hashCode;

	++bucket->NumPair;

	mPairs[mNumData] = inserted;
	++mNumData;
}

template<typename T, typename U>
inline bool FixedHashMap<T, U>::Remove(T key)
{
	size_t hashCode = mHashFunction(key);
	size_t index = hashCode % mBucketCount;

	Bucket* bucket = &mBuckets[index];
	size_t numPair = bucket->NumPair;

	for (size_t i = 0; i < numPair; ++i)
	{
		Pair<T, U>* pair = &bucket->Pairs[i];

		if (pair->mHashCode == hashCode && pair->mKey == key)
		{
			Pair<T, U>* endPair = &bucket->Pairs[numPair - 1];

			--mNumData;
			mPairs[pair->mIndex] = mPairs[mNumData];

			pair->mKey = endPair->mKey;
			pair->mValue = endPair->mValue;
			pair->mIndex = endPair->mIndex;
			pair->mHashCode = endPair->mHashCode;

			--bucket->NumPair;

			return true;
		}
	}
	return false;
}

template<typename T, typename U>
inline bool FixedHashMap<T, U>::Contains(T key) const
{
	size_t hashCode = mHashFunction(key);
	size_t index = hashCode % mBucketCount;

	Bucket* bucket = &mBuckets[index];
	size_t numPair = bucket->NumPair;

	for (size_t i = 0; i < numPair; ++i)
	{
		Pair<T, U>* pair = &bucket->Pairs[i];

		if (pair->mHashCode == hashCode && pair->mKey == key)
		{
			return true;
		}
	}
	return false;
}

template<typename T, typename U>
inline U FixedHashMap<T, U>::Find(T key) const
{
	size_t hashCode = mHashFunction(key);
	size_t index = hashCode % mBucketCount;

	const Bucket* bucket = &mBuckets[index];
	size_t numPair = bucket->NumPair;

	for (size_t i = 0; i < numPair; ++i)
	{
		const Pair<T, U>* pair = &bucket->Pairs[i];

		if (pair->mHashCode == hashCode && pair->mKey == key)
		{
			return pair->mValue;
		}
	}
	return U();
}

template<typename T, typename U>
inline void FixedHashMap<T, U>::Clear()
{
	for (size_t i = 0; i < mBucketCount; ++i)
	{
		mBuckets[i].NumPair = 0;
	}
}

template<typename T, typename U>
inline Pair<T, U>** FixedHashMap<T, U>::GetPairs() const
{
	return mPairs;
}

template<typename T, typename U>
inline size_t FixedHashMap<T, U>::GetSize() const
{
	return mNumData;
}
