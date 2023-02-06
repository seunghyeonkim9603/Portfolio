#include <stdio.h>
#include <stdarg.h>

#include "mysql.h"
#include "errmsg.h"

#include "DBConnector.h"

DBConnector::DBConnector()
	:	mConn({0, }),
	mSqlResult(nullptr),
	mConnection(nullptr),
	mPort(0),
	mLatestErrorCode(0),
	mbIsConnected(false)
{
	mysql_init(&mConn);
}

DBConnector::~DBConnector()
{
	mysql_close(mConnection);
}

bool DBConnector::TryConnect(const char* hostIp, const char* userName, const char* password, const char* dbName, unsigned int port)
{
	strcpy_s(mIP, hostIp);
	strcpy_s(mUserName, userName);
	strcpy_s(mDBName, dbName);
	mPort = port;

	mConnection = mysql_real_connect(&mConn, hostIp, userName, password, dbName, port, nullptr, 0);
	if (mConnection == nullptr)
	{
		mLatestErrorCode = mysql_errno(mConnection);

		return false;
	}
	mbIsConnected = true;

	return true;
}

bool DBConnector::TryRequestQuery(const char* format, ...)
{
	va_list arg;
	char query[MAX_QUERY_LENGTH];

	va_start(arg, format);
	{
		vsprintf_s(query, format, arg);
	}
	va_end(arg);

	int queryStat = mysql_query(mConnection, query);
	if (queryStat != 0)
	{
		mLatestErrorCode = mysql_errno(mConnection);

		return false;
	}
	return true;
}

bool DBConnector::TryRequestQueryResult(const char* format, ...)
{
	va_list arg;
	char query[MAX_QUERY_LENGTH];

	va_start(arg, format);
	{
		vsprintf_s(query, format, arg);
	}
	va_end(arg);

	int queryStat = mysql_query(mConnection, query);
	if (queryStat != 0)
	{
		mLatestErrorCode = mysql_errno(mConnection);

		return false;
	}
	mSqlResult = mysql_store_result(mConnection);

	if (mSqlResult == nullptr)
	{
		mLatestErrorCode = mysql_errno(mConnection);

		return false;
	}
	return true;
}

void DBConnector::Disconnect()
{
	mysql_close(mConnection);

	mConnection = nullptr;
}

MYSQL_ROW DBConnector::FetchRowOrNull()
{	
	MYSQL_ROW row = mysql_fetch_row(mSqlResult);
	if (row == nullptr)
	{
		mSqlResult = nullptr;
	}
	return row;
}

void DBConnector::FreeResult()
{
	mysql_free_result(mSqlResult);
}

int DBConnector::GetLatestErrorCode() const
{
	return mLatestErrorCode;
}

bool DBConnector::IsConnected() const
{
	return mbIsConnected;
}
