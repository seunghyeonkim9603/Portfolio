#pragma once

namespace NetworkLib
{
	class ContentThread
	{
		friend class WANServer;
	public:
		ContentThread(WANServer* server, unsigned int threadId, unsigned int fps);
		virtual ~ContentThread();

		unsigned int GetThreadId() const;

		void Run();
		bool TrySendMessage(sessionID_t id, Message* message);
		bool TryDisconnect(sessionID_t id);
		void MoveThread(sessionID_t id, unsigned int toThreadId, void* trans);
		unsigned int GetFPS();

		Message* AllocMessage();
		void		ReleaseMessage(Message* message);

		virtual void OnThreadStart() = 0;
		virtual void OnThreadEnd() = 0;
		virtual void OnUpdate() = 0;
		virtual void OnClientThreadJoin(sessionID_t id, void* transffered) = 0;
		virtual void OnClientThreadLeave(sessionID_t id) = 0;
		virtual void OnClientDisconnect(sessionID_t id) = 0;
		virtual void OnRecv(sessionID_t id, Message* message) = 0;

	private:
		static unsigned int __stdcall workerThread(void* param);

	private:
		WANServer* mServer;

		HANDLE			mThreadHandle;

		unsigned int	mThreadId;
		unsigned int	mFPS;
		unsigned int	mCurrentFPS;

		LockFreeQueue<Session*> mJoinedSessions;
		std::unordered_map<sessionID_t, Session*> mSessions;
	};
}