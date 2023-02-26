#pragma once

namespace NetworkLib
{
    class Message final
    {
        friend class Chunk<Message>;
        friend class WANServer;
        friend class LANServer;
        friend class WANClient;
        friend class LANClient;
    public:
        void     Reserve(unsigned int capacity);
        void     Clear();
        unsigned int		GetCapacity() const;
        unsigned int		GetSize() const;
        char*    GetBuffer() const;
        char*    GetFront() const;
        char*    GetRear() const;
        unsigned int		MoveWritePos(unsigned int offset);
        unsigned int		MoveReadPos(unsigned int offset);

        Message& operator=(const Message& other);
        Message& operator=(Message&& other) noexcept;

        Message& operator<<(unsigned char val);
        Message& operator<<(char val);
        Message& operator<<(bool val);
        Message& operator<<(short val);
        Message& operator<<(unsigned short val);
        Message& operator<<(int val);
        Message& operator<<(unsigned int val);
        Message& operator<<(long val);
        Message& operator<<(unsigned long val);
        Message& operator<<(long long val);
        Message& operator<<(unsigned long long val);
        Message& operator<<(float val);
        Message& operator<<(double val);

        Message& operator>>(unsigned char& val);
        Message& operator>>(char& val);
        Message& operator>>(bool& val);
        Message& operator>>(short& val);
        Message& operator>>(unsigned short& val);
        Message& operator>>(int& val);
        Message& operator>>(unsigned int& val);
        Message& operator>>(long& val);
        Message& operator>>(unsigned long& val);
        Message& operator>>(long long& val);
        Message& operator>>(unsigned long long& val);
        Message& operator>>(float& val);
        Message& operator>>(double& val);

        Message& Write(const char* str, unsigned int size);
        Message& Read(char* outBuffer, unsigned int size);

        void AddReferenceCount();

    private:
        static Message* Create();
        static void     Release(Message* message);

        Message();
        Message(int capacity);
        Message(const Message& other);
        Message(Message&& other) noexcept;
        ~Message();

        bool isPending() const;
        void dispend();
        unsigned int  getEnquableSize() const;

    private:
        enum { DEFAULT_SIZE = 128 };

        static TLSObjectPool<Message>   MessagePool;

        unsigned int    mCapacity;
        char*           mBuffer;
        char*           mFront;
        char*           mRear;
        unsigned int*   mRefCount;
        bool            mbIsPending;
    };
}
