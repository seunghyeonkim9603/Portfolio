#include "stdafx.h"

#define LOG_FILE_NAME (L"LOGIN_THREAD_ERR_LOG.txt")

LoginThread::LoginThread(NetworkLib::WANServer* wanServer, DBWriterThread* gameDB, DBWriterThread* logDB)
    : ContentThread(wanServer, EContentType::CONTENT_TYPE_LOGIN, THREAD_FPS),
    mGameDBWriter(gameDB),
    mLogDBWriter(logDB),
    mNumLoginFailed(0),
    mNumLoginSucceed(0),
    mNumPlayer(0)
{
}

LoginThread::~LoginThread()
{
}

void LoginThread::OnThreadStart()
{
    mRedis.connect();

    if (!mDBConnector.TryConnect("127.0.0.1", "root", "123456", "gamedb", 3306))
    {
        Logger::AppendLine(L"Login thread DBConnector failed connect");
        Logger::Log(LOG_FILE_NAME);
    }
}

void LoginThread::OnThreadEnd()
{
}

void LoginThread::OnUpdate()
{
}

void LoginThread::OnClientThreadJoin(sessionID_t id, void* transffered)
{
    Player* joined = mPlayerPool.GetObject();
    {
        joined->ObjectType = EObjectType::OBJECT_TYPE_PLAYER;
        joined->SessionID = id;
        joined->bIsAuthSucceed = false;
    }
    mPlayers.insert({ id, joined });

    ++mNumPlayer;
}

void LoginThread::OnClientThreadLeave(sessionID_t id)
{
    mPlayers.erase(id);

    --mNumPlayer;
}

void LoginThread::OnClientDisconnect(sessionID_t id)
{
    Player* leaved = mPlayers.find(id)->second;

    mPlayers.erase(id);

    mLogDBWriter->Write("INSERT INTO `logdb`.`gamelog`(type, code, accountno, servername, param1, param2, param3, param4) VALUES(1, 12, %lld, \"game\", %d, %d, %d, %d)",
        leaved->AccountNo, leaved->TileX, leaved->TileY, leaved->Cristal, leaved->HP);

    mPlayerPool.ReleaseObject(leaved);

    --mNumPlayer;
}

void LoginThread::OnRecv(sessionID_t id, NetworkLib::Message* message)
{
    Player* target = mPlayers.find(id)->second;

    NetworkLib::Message* sendMessage;
    BYTE status;

    WORD type;

    *message >> type;

    switch (type)
    {
    case en_PACKET_CS_GAME_REQ_LOGIN:

        INT64 accountNo;
        char sessionKey[SESSION_KEY_LENGTH];
        char accountNoBuff[64];

        *message >> accountNo;
        message->Read(sessionKey, SESSION_KEY_LENGTH);

        sprintf_s(accountNoBuff, "%lld", accountNo);

        mRedis.get(accountNoBuff);

        target->bIsAuthSucceed = true;

        status = 0;

        mDBConnector.TryRequestQueryResult("SELECT * FROM `gamedb`.`character` WHERE accountno = %lld", accountNo);

        MYSQL_ROW sqlRow;

        sqlRow = mDBConnector.FetchRowOrNull();

        if (sqlRow == nullptr)
        {
            status = 2;
        }
        else
        {
            status = 1;

            sscanf_s(sqlRow[1], "%d", &target->CharacterType);
            sscanf_s(sqlRow[2], "%f", &target->PosX);
            sscanf_s(sqlRow[3], "%f", &target->PosY);
            sscanf_s(sqlRow[4], "%d", &target->TileX);
            sscanf_s(sqlRow[5], "%d", &target->TileY);
            sscanf_s(sqlRow[6], "%d", &target->Rotation);
            sscanf_s(sqlRow[7], "%d", &target->Cristal);
            sscanf_s(sqlRow[8], "%d", &target->HP);
            sscanf_s(sqlRow[9], "%lld", &target->Exp);
            sscanf_s(sqlRow[10], "%d", &target->Level);
            sscanf_s(sqlRow[11], "%d", &target->bIsDie);

            mDBConnector.FreeResult();
        }
        target->AccountNo = accountNo;
        target->ClientID = ClientIDGenerator::Generate();

        mLogDBWriter->Write("INSERT INTO `logdb`.`gamelog`(type, code, accountno, servername, param1, param2, param3, param4) VALUES(1, 11, %lld, \"game\", %d, %d, %d, %d)",
            accountNo, target->TileX, target->TileY, target->Cristal, target->HP);

        sendMessage = AllocMessage();
        {
            *sendMessage << (WORD)en_PACKET_CS_GAME_RES_LOGIN;
            *sendMessage << status;
            *sendMessage << accountNo;

            TrySendMessage(id, sendMessage);
        }
        ReleaseMessage(sendMessage);

        if (status == 1)
        {
            MoveThread(id, EContentType::CONTENT_TYPE_GAME, target);
        }
        else
        {
            MoveThread(id, EContentType::CONTENT_TYPE_SELECT_CHARACTER, target);
        }
        break;
    case en_PACKET_CS_GAME_REQ_HEARTBEAT:
        break;
    default:
        std::cout << "Login Unhandled Packet Type" << std::endl;
        CCrashDump::Crash();
        break;
    }
}

unsigned int LoginThread::GetNumPlayer() const
{
    return mNumPlayer;
}

unsigned int LoginThread::GetNumLoginSucceed() const
{
    return mNumLoginSucceed;
}

unsigned int LoginThread::GetNumLoginFailed() const
{
    return mNumLoginFailed;
}

