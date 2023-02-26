#include "pch.h"

namespace NetworkLib
{
    TLSObjectPool<Message> Message::MessagePool;

    Message::Message()
        : Message(DEFAULT_SIZE)
    {
    }

    Message::Message(int capacity)
        : mCapacity(capacity),
        mBuffer(new char[capacity]),
        mFront(mBuffer),
        mRear(mBuffer),
        mRefCount(new unsigned int(1)),
        mbIsPending(true)
    {
    }

    Message::Message(const Message& other)
        : mCapacity(other.mCapacity),
        mBuffer(new char[other.mCapacity]),
        mFront(mBuffer + (other.mFront - other.mBuffer)),
        mRear(mBuffer + (other.mRear - other.mBuffer)),
        mRefCount(new unsigned int(*other.mRefCount))
    {
        memcpy(mBuffer, other.mBuffer, mCapacity);
    }

    Message::Message(Message&& other) noexcept
        : mCapacity(other.mCapacity),
        mBuffer(other.mBuffer),
        mFront(other.mFront),
        mRear(other.mRear),
        mRefCount(other.mRefCount)
    {
        other.mBuffer = nullptr;
        other.mRefCount = nullptr;
    }

    Message::~Message()
    {
        delete mBuffer;
        delete mRefCount;
    }

    void Message::Reserve(unsigned int capacity)
    {
        if (capacity <= mCapacity)
        {
            return;
        }
        Message temp(capacity);

        temp.Write(mFront, GetSize());

        *this = std::move(temp);
    }

    void Message::Clear()
    {
        mFront = mBuffer;
        mRear = mBuffer;
    }

    unsigned int Message::GetCapacity() const
    {
        return mCapacity;
    }

    unsigned int Message::GetSize() const
    {
        return static_cast<int>(mRear - mFront);
    }

    char* Message::GetBuffer() const
    {
        return mBuffer;
    }

    char* Message::GetRear() const
    {
        return mRear;
    }

    char* Message::GetFront() const
    {
        return mFront;
    }

    unsigned int Message::MoveWritePos(unsigned int offset)
    {
        mRear += offset;
        return offset;
    }

    unsigned int Message::MoveReadPos(unsigned int offset)
    {
        mFront += offset;
        return offset;
    }

    Message& Message::operator=(const Message& other)
    {
        if (mBuffer != nullptr)
        {
            delete mBuffer;
        }
        mBuffer = new char[other.mCapacity];
        memcpy(mBuffer, other.mBuffer, other.mCapacity);
        mCapacity = other.mCapacity;
        mFront = mBuffer + (other.mFront - other.mBuffer);
        mRear = mBuffer + (other.mRear - other.mBuffer);

        return *this;
    }

    Message& Message::operator=(Message&& other) noexcept
    {
        if (mBuffer != nullptr)
        {
            delete mBuffer;
        }
        mBuffer = other.mBuffer;
        mCapacity = other.mCapacity;
        mFront = other.mFront;
        mRear = other.mRear;

        delete other.mBuffer;
        other.mBuffer = nullptr;

        return *this;
    }

    Message& Message::operator<<(unsigned char val)
    {
        int enquableSize = getEnquableSize();

        if (enquableSize < sizeof(val))
        {
            Reserve(mCapacity * 2);
        }
        unsigned char* rear = reinterpret_cast<unsigned char*>(mRear);

        *rear = val;
        ++rear;
        mRear = reinterpret_cast<char*>(rear);

        return *this;
    }

    Message& Message::operator<<(char val)
    {
        int enquableSize = getEnquableSize();

        if (enquableSize < sizeof(val))
        {
            Reserve(mCapacity * 2);
        }
        char* rear = reinterpret_cast<char*>(mRear);

        *rear = val;
        ++rear;
        mRear = reinterpret_cast<char*>(rear);

        return *this;
    }

    Message& Message::operator<<(bool val)
    {
        int enquableSize = getEnquableSize();

        if (enquableSize < sizeof(val))
        {
            Reserve(mCapacity * 2);
        }
        bool* rear = reinterpret_cast<bool*>(mRear);

        *rear = val;
        ++rear;
        mRear = reinterpret_cast<char*>(rear);

        return *this;
    }

    Message& Message::operator<<(short val)
    {
        int enquableSize = getEnquableSize();

        if (enquableSize < sizeof(val))
        {
            Reserve(mCapacity * 2);
        }
        short* rear = reinterpret_cast<short*>(mRear);

        *rear = val;
        ++rear;
        mRear = reinterpret_cast<char*>(rear);

        return *this;
    }

    Message& Message::operator<<(unsigned short val)
    {
        int enquableSize = getEnquableSize();

        if (enquableSize < sizeof(val))
        {
            Reserve(mCapacity * 2);
        }
        unsigned short* rear = reinterpret_cast<unsigned short*>(mRear);

        *rear = val;
        ++rear;
        mRear = reinterpret_cast<char*>(rear);

        return *this;
    }

    Message& Message::operator<<(int val)
    {
        int enquableSize = getEnquableSize();

        if (enquableSize < sizeof(val))
        {
            Reserve(mCapacity * 2);
        }
        int* rear = reinterpret_cast<int*>(mRear);

        *rear = val;
        ++rear;
        mRear = reinterpret_cast<char*>(rear);

        return *this;
    }

    Message& Message::operator<<(unsigned int val)
    {
        int enquableSize = getEnquableSize();

        if (enquableSize < sizeof(val))
        {
            Reserve(mCapacity * 2);
        }
        unsigned int* rear = reinterpret_cast<unsigned int*>(mRear);

        *rear = val;
        ++rear;
        mRear = reinterpret_cast<char*>(rear);

        return *this;
    }

    Message& Message::operator<<(long val)
    {
        int enquableSize = getEnquableSize();

        if (enquableSize < sizeof(val))
        {
            Reserve(mCapacity * 2);
        }
        long* rear = reinterpret_cast<long*>(mRear);

        *rear = val;
        ++rear;
        mRear = reinterpret_cast<char*>(rear);

        return *this;
    }

    Message& Message::operator<<(unsigned long val)
    {
        int enquableSize = getEnquableSize();

        if (enquableSize < sizeof(val))
        {
            Reserve(mCapacity * 2);
        }
        unsigned long* rear = reinterpret_cast<unsigned long*>(mRear);

        *rear = val;
        ++rear;
        mRear = reinterpret_cast<char*>(rear);

        return *this;
    }

    Message& Message::operator<<(long long val)
    {
        int enquableSize = getEnquableSize();

        if (enquableSize < sizeof(val))
        {
            Reserve(mCapacity * 2);
        }
        long long* rear = reinterpret_cast<long long*>(mRear);

        *rear = val;
        ++rear;
        mRear = reinterpret_cast<char*>(rear);

        return *this;
    }

    Message& Message::operator<<(unsigned long long val)
    {
        int enquableSize = getEnquableSize();

        if (enquableSize < sizeof(val))
        {
            Reserve(mCapacity * 2);
        }
        unsigned long long* rear = reinterpret_cast<unsigned long long*>(mRear);

        *rear = val;
        ++rear;
        mRear = reinterpret_cast<char*>(rear);

        return *this;
    }

    Message& Message::operator<<(float val)
    {
        int enquableSize = getEnquableSize();

        if (enquableSize < sizeof(val))
        {
            Reserve(mCapacity * 2);
        }
        float* rear = reinterpret_cast<float*>(mRear);

        *rear = val;
        ++rear;
        mRear = reinterpret_cast<char*>(rear);

        return *this;
    }

    Message& Message::operator<<(double val)
    {
        int enquableSize = getEnquableSize();

        if (enquableSize < sizeof(val))
        {
            Reserve(mCapacity * 2);
        }
        double* rear = reinterpret_cast<double*>(mRear);

        *rear = val;
        ++rear;
        mRear = reinterpret_cast<char*>(rear);

        return *this;
    }

    Message& Message::operator>>(unsigned char& outVal)
    {
        outVal = *reinterpret_cast<unsigned char*>(mFront);
        mFront += sizeof(outVal);

        return *this;
    }

    Message& Message::operator>>(char& outVal)
    {
        outVal = *reinterpret_cast<char*>(mFront);
        mFront += sizeof(outVal);

        return *this;
    }

    Message& Message::operator>>(bool& outVal)
    {
        outVal = *reinterpret_cast<bool*>(mFront);
        mFront += sizeof(outVal);

        return *this;
    }

    Message& Message::operator>>(short& outVal)
    {
        outVal = *reinterpret_cast<short*>(mFront);
        mFront += sizeof(outVal);

        return *this;
    }

    Message& Message::operator>>(unsigned short& outVal)
    {
        outVal = *reinterpret_cast<unsigned short*>(mFront);
        mFront += sizeof(outVal);

        return *this;
    }

    Message& Message::operator>>(int& outVal)
    {
        outVal = *reinterpret_cast<int*>(mFront);
        mFront += sizeof(outVal);

        return *this;
    }

    Message& Message::operator>>(unsigned int& outVal)
    {
        outVal = *reinterpret_cast<unsigned int*>(mFront);
        mFront += sizeof(outVal);

        return *this;
    }

    Message& Message::operator>>(long& outVal)
    {
        outVal = *reinterpret_cast<long*>(mFront);
        mFront += sizeof(outVal);

        return *this;
    }

    Message& Message::operator>>(unsigned long& outVal)
    {
        outVal = *reinterpret_cast<unsigned long*>(mFront);
        mFront += sizeof(outVal);

        return *this;
    }

    Message& Message::operator>>(long long& outVal)
    {
        outVal = *reinterpret_cast<long long*>(mFront);
        mFront += sizeof(outVal);

        return *this;
    }

    Message& Message::operator>>(unsigned long long& outVal)
    {
        outVal = *reinterpret_cast<unsigned long long*>(mFront);
        mFront += sizeof(outVal);

        return *this;
    }

    Message& Message::operator>>(float& outVal)
    {
        outVal = *reinterpret_cast<float*>(mFront);
        mFront += sizeof(outVal);

        return *this;
    }

    Message& Message::operator>>(double& outVal)
    {
        outVal = *reinterpret_cast<double*>(mFront);
        mFront += sizeof(outVal);

        return *this;
    }

    Message& Message::Write(const char* str, unsigned int size)
    {
        unsigned int enquableSize = getEnquableSize();

        if (enquableSize < size)
        {
            Reserve(mCapacity * 2);
        }
        memcpy(mRear, str, size);
        mRear += size;

        return *this;
    }

    Message& Message::Read(char* outBuffer, unsigned int size)
    {
        memcpy(outBuffer, mFront, size);
        mFront += size;

        return *this;
    }

    void Message::AddReferenceCount()
    {
        InterlockedIncrement(mRefCount);
    }

    Message* Message::Create()
    {
        Message* message = MessagePool.GetObject();

        return message;
    }

    void Message::Release(Message* message)
    {
        if (InterlockedDecrement(message->mRefCount) == 0)
        {
            message->mbIsPending = true;
            message->Clear();

            *message->mRefCount = 1;

            MessagePool.ReleaseObject(message);
        }
    }

    bool Message::isPending() const
    {
        return mbIsPending;
    }

    void Message::dispend()
    {
        mbIsPending = false;
    }

    unsigned int Message::getEnquableSize() const
    {
        return static_cast<unsigned int>(mBuffer + mCapacity - mRear);
    }

}