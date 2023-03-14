#pragma once

namespace NetworkLib
{
	class LANClient final
	{
	public:
		LANClient();
		~LANClient();
		LANClient(const LANClient& other) = delete;
		LANClient(LANClient&& other) = delete;

		LANClient& operator=(const LANClient& other) = delete;
		LANClient& operator=(LANClient&& other) noexcept = delete;

		bool			TryRun(const unsigned long IP, const unsigned short port
			, const unsigned int numWorkerThread, const unsigned int numRunningThread
			, const bool bSupportsNagle, INetworkClientEventListener* listener);

		void			Terminate();
		bool			TryDisconnect();
		bool			TrySendMessage(Message* messagePtr);

		unsigned long	GetIP() const;
		unsigned short	GetPort() const;

		unsigned int	GetNumRecv() const;
		unsigned int	GetNumSend() const;
		unsigned int	GetMessagePoolActiveCount() const;

		Message*		CreateMessage();
		void			ReleaseMessage(Message* message);

	private:
		static unsigned int __stdcall	workerThread(void* param);

		void		sendPost();
		void		recvPost();

		void		releaseClient();
		bool		tryAcquireClient();

	private:
		enum
		{
			PACKET_CODE = 0x77,
			MAX_WORKER_THREADS = 256,
			DISCONNECTION_SEND_QUEUE_SIZE = 1024,
		};

		unsigned long					mIP;
		unsigned short					mPort;
		Session							mClient;

		INetworkClientEventListener*	mListener;

		HANDLE							mhCompletionPort;
		HANDLE							mhWorkerThread;
		HANDLE							mhWorkerThreads[MAX_WORKER_THREADS];

		unsigned int					mNumWorkerThreads;

		CACHE_ALIGN unsigned int		mNumRecv;
		CACHE_ALIGN unsigned int		mNumSend;
	};
}