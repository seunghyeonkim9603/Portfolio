#include "pch.h"

namespace NetworkLib
{
	static bool gExit;

	unsigned int __stdcall ContentThread::workerThread(void* param)
	{
		ContentThread* content = (ContentThread*)param;
		std::vector<sessionID_t> leavedSession;

		unsigned int lastFPS = 0;
		unsigned int frame = 0;

		LARGE_INTEGER freq;
		LARGE_INTEGER lastUpdateTime;
		LARGE_INTEGER currentTime;
		LARGE_INTEGER perFrameTime;
		LARGE_INTEGER lastFPSUpdateTime;

		QueryPerformanceFrequency(&freq);
		QueryPerformanceCounter(&lastFPSUpdateTime);
		QueryPerformanceCounter(&lastUpdateTime);

		perFrameTime.QuadPart = freq.QuadPart / content->mFPS;

		content->OnThreadStart();

		while (!gExit)
		{
			Session* joined;

			QueryPerformanceCounter(&currentTime);

			LONGLONG elapsedTime = currentTime.QuadPart - lastUpdateTime.QuadPart;
			LONGLONG fpsElapsedTime = currentTime.QuadPart - lastFPSUpdateTime.QuadPart;

			if (perFrameTime.QuadPart < elapsedTime)
			{
				QueryPerformanceCounter(&lastUpdateTime);

				content->OnUpdate();

				lastUpdateTime.QuadPart -= elapsedTime - perFrameTime.QuadPart;

				++frame;
			}
			if (freq.QuadPart < fpsElapsedTime)
			{
				QueryPerformanceCounter(&lastFPSUpdateTime);

				content->mCurrentFPS = frame - lastFPS;
				lastFPS = frame;
			}

			if (!content->mJoinedSessions.IsEmpty())
			{
				while (content->mJoinedSessions.Dequeue(&joined))
				{
					content->mSessions.insert({ joined->Id, joined });

					content->OnClientThreadJoin(joined->Id, joined->ThreadSendData);
				}
			}

			for (auto iter : content->mSessions)
			{
				sessionID_t id = iter.first;
				Session* session = iter.second;

				Message* recvedMessage;

				if (session->Verifier.ReleaseFlag)
				{
					leavedSession.push_back(id);
					continue;
				}

				if (!session->RecvQueue.IsEmpty())
				{
					while (session->RecvQueue.Dequeue(&recvedMessage))
					{
						content->OnRecv(id, recvedMessage);
						content->mServer->ReleaseMessage(recvedMessage);

						if (session->CurrentThreadId != content->mThreadId || session->Verifier.ReleaseFlag)
						{
							leavedSession.push_back(id);
							break;
						}
					}
				}
			}
			for (sessionID_t id : leavedSession)
			{
				Session* leaved = content->mSessions.find(id)->second;

				content->mSessions.erase(id);

				if (leaved->Verifier.ReleaseFlag)
				{
					content->OnClientDisconnect(id);
					content->mServer->onLeaveSessionFromContentThread(leaved);
				}
				else
				{
					content->OnClientThreadLeave(id);
					content->mServer->moveSessionTo(leaved->CurrentThreadId, leaved);
				}
			}
			leavedSession.clear();

			QueryPerformanceCounter(&currentTime);
		}

		content->OnThreadEnd();

		return 0;
	}

	ContentThread::ContentThread(WANServer* server, unsigned int threadId, unsigned int fps)
		: mServer(server),
		mThreadId(threadId),
		mFPS(fps),
		mCurrentFPS(0),
		mJoinedSessions()
	{
	}

	ContentThread::~ContentThread()
	{
		gExit = true;

		WaitForSingleObject(mThreadHandle, INFINITE);
		CloseHandle(mThreadHandle);
	}

	unsigned int ContentThread::GetThreadId() const
	{
		return mThreadId;
	}

	void ContentThread::Run()
	{
		mThreadHandle = (HANDLE)_beginthreadex(nullptr, 0, &workerThread, this, 0, nullptr);
	}

	bool ContentThread::TrySendMessage(sessionID_t id, Message* message)
	{
		return mServer->TrySendMessage(id, message);
	}

	bool ContentThread::TryDisconnect(sessionID_t id)
	{
		return mServer->TryDisconnect(id);
	}

	void ContentThread::MoveThread(sessionID_t id, unsigned int toThreadId, void* transData)
	{
		auto iter = mSessions.find(id);

		if (iter == mSessions.end())
		{
			return;
		}
		Session* session = iter->second;

		session->ThreadSendData = transData;
		session->CurrentThreadId = toThreadId;
	}

	unsigned int ContentThread::GetFPS()
	{
		return mCurrentFPS;
	}

	Message* ContentThread::AllocMessage()
	{
		return mServer->CreateMessage();
	}

	void ContentThread::ReleaseMessage(Message* message)
	{
		mServer->ReleaseMessage(message);
	}
}