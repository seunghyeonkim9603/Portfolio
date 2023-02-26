#pragma once

class DBWriterThread final
{
public:
	DBWriterThread(const char* hostIp, const char* userName, const char* password, const char* dbName, unsigned int port);
	~DBWriterThread();

	bool TryRun();
	void Write(const char* format, ...);

	unsigned int GetQueryQueueSize() const;
	unsigned int GetNumWrite() const;

private:
	static unsigned int writeThread(void* param);

private:
	enum
	{
		QUERY_LENGTH = 2056,
		INFO_LENGTH = 64
	};

	struct Query
	{
		char str[QUERY_LENGTH];
	};

	NetworkLib::LockFreeQueue<Query*> mQueryQueue;
	NetworkLib::TLSObjectPool<Query> mQueryPool;

	DBConnector mDBConnector;

	char mHostIP[INFO_LENGTH];
	char mUserName[INFO_LENGTH];
	char mPassword[INFO_LENGTH];
	char mDBName[INFO_LENGTH];
	unsigned int mPort;

	HANDLE mhWriteEvent;
	HANDLE mhWriteThread;

	unsigned int mNumWrite;
};