#pragma once

class DBConnector final
{
public:
	DBConnector();
	~DBConnector();
	
	bool		TryConnect(const char* hostIp, const char* userName, const char* password, const char* dbName, unsigned int port);
	bool		TryRequestQuery(const char* format, ...);
	bool		TryRequestQueryResult(const char* format, ...);
	void		Disconnect();

	MYSQL_ROW	FetchRowOrNull();
	void		FreeResult();
	
	int			GetLatestErrorCode() const;
	bool		IsConnected() const;

private:
	enum
	{
		MAX_PROPERTY_LENGTH = 64,
		MAX_QUERY_LENGTH = 512
	};

	MYSQL			mConn;
	MYSQL*			mConnection;
	MYSQL_RES*		mSqlResult;

	char			mIP[MAX_PROPERTY_LENGTH];
	char			mUserName[MAX_PROPERTY_LENGTH];
	char			mDBName[MAX_PROPERTY_LENGTH];
	unsigned int	mPort;
	unsigned int	mLatestErrorCode;
	bool			mbIsConnected;
};
