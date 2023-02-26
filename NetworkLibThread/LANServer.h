#pragma once

namespace NetworkLib
{
	class LANServer final
	{
	public:
		LANServer();
		~LANServer();
		LANServer(const LANServer& other) = delete;
		LANServer(LANServer&& other) = delete;

		LANServer& operator=(const LANServer& other) = delete;
		LANServer& operator=(LANServer&& other) noexcept = delete;

		bool			TryRun(const unsigned long IP, const unsigned short port
			, const unsigned int numWorkerThread, const unsigned int numRunningThread
			, const unsigned int maxSessionCount, const bool bSupportsNagle
			, INetworkServerEventListener* listener);

		void			Terminate();
		bool			TrySendMessage(const sessionID_t ID, Message* messagePtr);
		bool			TryDisconnect(const sessionID_t ID);

		unsigned long	GetIP() const;
		unsigned short	GetPort() const;

		unsigned int	GetNumAccept() const;
		unsigned int	GetNumRecv() const;
		unsigned int	GetNumSend() const;
		unsigned int	GetMaximumSessionCount() const;
		unsigned int	GetCurrentSessionCount() const;

		Message*		CreateMessage();
		void			ReleaseMessage(Message* message);

	private:
		static unsigned int __stdcall	acceptThread(void* param);
		static unsigned int __stdcall	workerThread(void* param);

		void		sendPost(Session* session);
		void		recvPost(Session* session);

		void		releaseSession(Session* session);
		Session*	acquireSessionOrNull(const sessionID_t ID);

	private:
		enum
		{
			PACKET_CODE = 0x77,
			MAX_WORKER_THREADS = 256,
			DISCONNECTION_SEND_QUEUE_SIZE = 1024,
			INVALID_SESSION_ID = 0xFFFFFFFFFFFFFFFF
		};

		unsigned long					mIP;
		unsigned short					mPort;
		SOCKET							mListenSocket;

		Session*						mSessions;
		INetworkServerEventListener*	mListener;
		LockFreeStack<uint64_t>*		mUseableIndexes;

		HANDLE							mhCompletionPort;
		HANDLE							mhAcceptThread;
		HANDLE							mhWorkerThreads[MAX_WORKER_THREADS];

		unsigned int					mNumWorkerThreads;

		CACHE_ALIGN unsigned int		mNumAccept;
		CACHE_ALIGN unsigned int		mNumRecv;
		CACHE_ALIGN unsigned int		mNumSend;
		CACHE_ALIGN unsigned int		mCurrentSessionCount;
		CACHE_ALIGN unsigned int		mMaximumSessionCount;
	};
}