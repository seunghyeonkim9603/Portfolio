#include "stdafx.h"

#define LOG_FILE_NAME (L"SELECT_THREAD_ERR_LOG.txt")

SelectCharacterThread::SelectCharacterThread(NetworkLib::WANServer* wanServer, DBWriterThread* gameDB, DBWriterThread* logDB)
    : ContentThread(wanServer, EContentType::CONTENT_TYPE_SELECT_CHARACTER, THREAD_FPS),
    mGameDBWriter(gameDB),
    mLogDBWriter(logDB),
    mNumPlayer(0)
{
}

SelectCharacterThread::~SelectCharacterThread()
{
}

void SelectCharacterThread::OnThreadStart() //수정 예정
{
}

void SelectCharacterThread::OnThreadEnd()
{
}

void SelectCharacterThread::OnUpdate()
{
}

void SelectCharacterThread::OnClientThreadJoin(sessionID_t id, void* transffered)
{
    Player* joined = (Player*)transffered;
    {
        joined->Cristal = 0;
        joined->Exp = 0;
        joined->Level = 1;
    }
    mPlayers.insert({ id, joined });

    ++mNumPlayer;
}

void SelectCharacterThread::OnClientThreadLeave(sessionID_t id)
{
    mPlayers.erase(id);

    --mNumPlayer;
}

void SelectCharacterThread::OnClientDisconnect(sessionID_t id)
{
    Player* leaved = mPlayers.find(id)->second;

    mPlayers.erase(id);
   
    mPlayerPool.ReleaseObject(leaved);

    --mNumPlayer;
}

void SelectCharacterThread::OnRecv(sessionID_t id, NetworkLib::Message* message)
{
    Player* target = mPlayers.find(id)->second;

    NetworkLib::Message* sendMessage;

    WORD type;

    *message >> type;

    switch (type)
    {
    case en_PACKET_CS_GAME_REQ_CHARACTER_SELECT:

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
        break;
    default:
        Logger::AppendLine(L"Select Character Unhandled Packet Type");
        Logger::Log(LOG_FILE_NAME);
        break;
    }
}

unsigned int SelectCharacterThread::GetNumPlayer() const
{
    return mNumPlayer;
}
