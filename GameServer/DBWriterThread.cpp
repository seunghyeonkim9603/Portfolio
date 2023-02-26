#include "stdafx.h"

#define LOG_FILE_NAME (L"DBWriterLog.txt")

DBWriterThread::DBWriterThread(const char* hostIp, const char* userName, const char* password, const char* dbName, unsigned int port)
    : mNumWrite(0),
    mPort(port)
{
    mhWriteEvent = CreateEvent(nullptr, false, false, nullptr);

    strcpy_s(mHostIP, hostIp);
    strcpy_s(mUserName, userName);
    strcpy_s(mPassword, password);
    strcpy_s(mDBName, dbName);
}

DBWriterThread::~DBWriterThread()
{
}

bool DBWriterThread::TryRun()
{
    mhWriteThread = (HANDLE)_beginthreadex(nullptr, 0, writeThread, this, 0, nullptr);

    return true;
}

void DBWriterThread::Write(const char* format, ...)
{
    Query* query = mQueryPool.GetObject();

    va_list arg;

    va_start(arg, format);
    {
        vsprintf_s(query->str, format, arg);
    }
    va_end(arg);

    mQueryQueue.Enqueue(query);

    SetEvent(mhWriteEvent);
}

unsigned int DBWriterThread::GetQueryQueueSize() const
{
    return mQueryQueue.GetSize();
}

unsigned int DBWriterThread::GetNumWrite() const
{
    return mNumWrite;
}

unsigned int DBWriterThread::writeThread(void* param)
{
    DBWriterThread* writer = (DBWriterThread*)param;
    Query* query;

    if (!writer->mDBConnector.IsConnected())
    {
        if (!writer->mDBConnector.TryConnect(writer->mHostIP, writer->mUserName, writer->mPassword, writer->mDBName, writer->mPort))
        {
            Logger::AppendLine(L"DBWriter Failed DB connection");
            Logger::Log(LOG_FILE_NAME);

            return 0;
        }
    }
    while (true)
    {
        if (writer->mQueryQueue.IsEmpty())
        {
            WaitForSingleObject(writer->mhWriteEvent, INFINITE);
        }
        writer->mQueryQueue.Dequeue(&query);

        if (!writer->mDBConnector.TryRequestQuery(query->str))
        {
            Logger::AppendLine(L"DBWriter Failed write... errno : %d, query : %s", writer->mDBConnector.GetLatestErrorCode(), query->str);
            Logger::Log(LOG_FILE_NAME);
        }

        writer->mQueryPool.ReleaseObject(query);
        ++writer->mNumWrite;
    }
    return 0;
}
