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
        char accountNoBuff[ACCOUNT_NO_BUF_LENGTH];

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
        break;
    case en_PACKET_CS_GAME_REQ_CHARACTER_SELECT:
        if (!target->bIsAuthSucceed)
        {
            break;
        }
        BYTE characterType;

        *message >> characterType;

        target->CharacterType = (ECharacterType)characterType;
        target->TileX = 85 + (rand() % 21);
        target->TileY = 93 + (rand() % 31);
        target->Rotation = rand() % 360;
        target->PosX = target->TileX / 2;
        target->PosY = target->TileY / 2;
        target->HP = 5000;
        target->Exp = 0;
        target->Cristal = 0;
        target->Level = 0;
        target->bIsSit = false;
        target->bIsDie = false;

        mLogDBWriter->Write("INSERT INTO `logdb`.`gamelog`(type, code, accountno, servername, param1) VALUES(3, 32, %lld, \"game\", %d)",
            target->AccountNo, target->CharacterType);
        mGameDBWriter->Write("INSERT INTO `gamedb`.`character`(accountno, charactertype, posx, posy, tilex, tiley, rotation, cristal, hp, exp, level, die) VALUES(%d, %d, %f, %f, %d, %d, %d, %d, %d, %d, %d, %d)",
            target->AccountNo, target->CharacterType, target->PosX, target->PosY, target->TileX, target->TileY, target->Rotation, target->Cristal, target->HP, target->Exp, target->Level, target->bIsDie);

        sendMessage = AllocMessage();
        {
            *sendMessage << (WORD)en_PACKET_CS_GAME_RES_CHARACTER_SELECT;
            *sendMessage << (BYTE)1;

            TrySendMessage(id, sendMessage);
        }
        ReleaseMessage(sendMessage);

        MoveThread(id, EContentType::CONTENT_TYPE_GAME, target);
        break;
    case en_PACKET_CS_GAME_REQ_HEARTBEAT:
        QueryPerformanceCounter(&target->LastRecvedTime);
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

