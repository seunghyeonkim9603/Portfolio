#include "pch.h"

namespace NetworkLib
{
	LANServer::LANServer()
		: mIP(0),
		mPort(0),
		mListenSocket(INVALID_SOCKET),
		mSessions(nullptr),
		mListener(nullptr),
		mUseableIndexes(nullptr),
		mhCompletionPort(INVALID_HANDLE_VALUE),
		mhAcceptThread(INVALID_HANDLE_VALUE),
		mNumWorkerThreads(0),
		mNumAccept(0),
		mNumRecv(0),
		mNumSend(0),
		mMaximumSessionCount(0),
		mCurrentSessionCount(0)
	{
	}

	LANServer::~LANServer()
	{
		Terminate();
	}

	bool LANServer::TryRun(const unsigned long IP, const unsigned short port
		, const unsigned int numWorkerThread, const unsigned int numRunningThread
		, const unsigned int maxSessionCount, const bool bSupportsNagle
		, INetworkServerEventListener* listener)
	{
		int retval;

		mIP = IP;
		mPort = port;
		mListener = listener;
		mMaximumSessionCount = maxSessionCount;

		mSessions = new Session[maxSessionCount];
		mUseableIndexes = new LockFreeStack<uint64_t>();

		mNumWorkerThreads = numWorkerThread;

		// Initialize Listen Socket
		{
			mListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (mListenSocket == INVALID_SOCKET)
			{
				return false;
			}

			SOCKADDR_IN serverAddr;
			{
				ZeroMemory(&serverAddr, sizeof(serverAddr));
				serverAddr.sin_family = AF_INET;
				serverAddr.sin_addr.s_addr = htonl(IP);
				serverAddr.sin_port = htons(port);
			}
			retval = bind(mListenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));

			if (retval == SOCKET_ERROR)
			{
				return false;
			}

			LINGER lingerOptval;
			{
				lingerOptval.l_onoff = 1;
				lingerOptval.l_linger = 0;
			}
			retval = setsockopt(mListenSocket, SOL_SOCKET, SO_LINGER, (char*)&lingerOptval, sizeof(lingerOptval));

			if (retval == SOCKET_ERROR)
			{
				return false;
			}
			if (!bSupportsNagle)
			{
				bool flag = true;
				retval = setsockopt(mListenSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));
				if (retval == SOCKET_ERROR)
				{
					return false;
				}
			}
			retval = listen(mListenSocket, SOMAXCONN);

			if (retval == SOCKET_ERROR)
			{
				return false;
			}
		}

		mhCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, numRunningThread);
		if (mhCompletionPort == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		for (unsigned int index = 0; index < maxSessionCount; ++index)
		{
			mUseableIndexes->Push(index);
		}
		for (unsigned int i = 0; i < numWorkerThread; ++i)
		{
			mhWorkerThreads[i] = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, &workerThread, this, 0, nullptr));
		}
		mhAcceptThread = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, &acceptThread, this, 0, nullptr));

		return true;
	}

	void LANServer::Terminate()
	{
		closesocket(mListenSocket);

		mListenSocket = INVALID_SOCKET;

		for (unsigned int i = 0; i < mMaximumSessionCount; ++i)
		{
			closesocket(mSessions[i].Socket);

			mSessions[i].Socket = INVALID_SOCKET;
		}
		for (unsigned int i = 0; i < mNumWorkerThreads; ++i)
		{
			PostQueuedCompletionStatus(mhCompletionPort, 0, 0, nullptr);
		}
		WaitForMultipleObjects(static_cast<DWORD>(mNumWorkerThreads), mhWorkerThreads, true, INFINITE);
		WaitForSingleObject(mhAcceptThread, INFINITE);

		for (unsigned int i = 0; i < mNumWorkerThreads; ++i)
		{
			CloseHandle(mhWorkerThreads[i]);

			mhWorkerThreads[i] = INVALID_HANDLE_VALUE;
		}
		CloseHandle(mhAcceptThread);
		CloseHandle(mhCompletionPort);

		mhAcceptThread = INVALID_HANDLE_VALUE;
		mhCompletionPort = INVALID_HANDLE_VALUE;

		delete mSessions;
		delete mUseableIndexes;

		mSessions = nullptr;
		mUseableIndexes = nullptr;
	}

	bool LANServer::TrySendMessage(const sessionID_t ID, Message* messagePtr)
	{
		Session* target = acquireSessionOrNull(ID);

		if (target == nullptr)
		{
			return false;
		}
		if (DISCONNECTION_SEND_QUEUE_SIZE < target->SendQueue.GetSize())
		{
			target->bShouldDisconnect = true;
			CancelIoEx((HANDLE)target->Socket, nullptr);

			if (InterlockedDecrement16(&target->Verifier.CurrentAsyncIOCount) == 0)
			{
				releaseSession(target);
			}
			return false;
		}
		messagePtr->AddReferenceCount();

		LANPacketHeader* header = (LANPacketHeader*)messagePtr->GetBuffer();

		if (messagePtr->isPending())
		{
			header->Length = messagePtr->GetSize();

			messagePtr->dispend();
		}
		target->SendQueue.Enqueue(messagePtr);

		sendPost(target);

		if (InterlockedDecrement16(&target->Verifier.CurrentAsyncIOCount) == 0)
		{
			releaseSession(target);
		}
		return true;
	}

	bool LANServer::TryDisconnect(const sessionID_t ID)
	{
		Session* target = acquireSessionOrNull(ID);

		if (target == nullptr)
		{
			return false;
		}
		target->bShouldDisconnect = true;
		CancelIoEx((HANDLE)target->Socket, nullptr);

		if (InterlockedDecrement16(&target->Verifier.CurrentAsyncIOCount) == 0)
		{
			releaseSession(target);
		}
		return true;
	}

	unsigned long LANServer::GetIP() const
	{
		return mIP;
	}

	unsigned short LANServer::GetPort() const
	{
		return mPort;
	}

	unsigned int LANServer::GetMaximumSessionCount() const
	{
		return mMaximumSessionCount;
	}

	unsigned int LANServer::GetCurrentSessionCount() const
	{
		return mCurrentSessionCount;
	}

	unsigned int LANServer::GetNumAccept() const
	{
		return mNumAccept;
	}

	unsigned int LANServer::GetNumRecv() const
	{
		return mNumRecv;
	}

	unsigned int LANServer::GetNumSend() const
	{
		return mNumSend;
	}

	Message* LANServer::CreateMessage()
	{
		Message* msg = Message::Create();

		msg->MoveReadPos(sizeof(LANPacketHeader));
		msg->MoveWritePos(sizeof(LANPacketHeader));

		return msg;
	}

	void LANServer::ReleaseMessage(Message* message)
	{
		Message::Release(message);
	}


	unsigned int __stdcall LANServer::acceptThread(void* param)
	{
		LANServer* server = static_cast<LANServer*>(param);

		UINT64 uniqueId = 0;
		int addrLen = sizeof(SOCKADDR_IN);

		while (true)
		{
			SOCKET clientSocket;
			SOCKADDR_IN clientAddr;

			clientSocket = accept(server->mListenSocket, (SOCKADDR*)&clientAddr, &addrLen);

			InterlockedIncrement(&server->mNumAccept);

			if (clientSocket == INVALID_SOCKET)
			{
				break;
			}
			if (server->mCurrentSessionCount == server->mMaximumSessionCount)
			{
				closesocket(clientSocket);
				continue;
			}
			unsigned long IP = ntohl(clientAddr.sin_addr.S_un.S_addr);
			unsigned short port = ntohs(clientAddr.sin_port);

			bool bJoinable = server->mListener->OnConnectionRequest(IP, port);

			if (!bJoinable)
			{
				continue;
			}
			uint64_t index;

			server->mUseableIndexes->Pop(&index);

			Session* session = &server->mSessions[index];
			{
				session->Addr = clientAddr;
				session->Socket = clientSocket;
				session->Id = MAKE_ID(uniqueId, index);
				session->Verifier.ReleaseFlag = 0;
			}
			InterlockedIncrement(&server->mCurrentSessionCount);

			CreateIoCompletionPort((HANDLE)session->Socket, server->mhCompletionPort, (ULONG_PTR)session, 0);

			server->mListener->OnClientJoin(session->Id, IP, port);

			server->recvPost(session);

			if (InterlockedDecrement16(&session->Verifier.CurrentAsyncIOCount) == 0)
			{
				server->releaseSession(session);
			}
			++uniqueId;
		}
		return 0;
	}


	unsigned int __stdcall LANServer::workerThread(void* param)
	{
		LANServer* server = (LANServer*)param;

		while (true)
		{
			DWORD cbTransferred = 0;
			Session* session = nullptr;
			Session::OverlappedExtension* overlapped;

			bool bSucceded = GetQueuedCompletionStatus(server->mhCompletionPort
				, &cbTransferred, (PULONG_PTR)&session, (LPOVERLAPPED*)&overlapped, INFINITE);

			if (bSucceded == true && overlapped == nullptr)
			{
				break;
			}
			else
			{
				int errorCode = GetLastError();

				if (overlapped == nullptr)
				{
					server->mListener->OnError(errorCode, L"Failed GQCS and Overlapped is null");
					break;
				}
				else if (errorCode != ERROR_NETNAME_DELETED && errorCode != ERROR_IO_PENDING
					&& errorCode != WSAECONNRESET && errorCode != ERROR_OPERATION_ABORTED && errorCode != 0)
				{
					server->mListener->OnError(errorCode, L"Failed I/O Operation");
				}
			}

			//Overlapped I/O Completion Processing
			if (cbTransferred != 0 && overlapped != nullptr && !session->bShouldDisconnect)
			{
				switch (overlapped->Type)
				{
				case EIOType::Recv:
				{
					LANPacketHeader header;
					RingBuffer& recvBuffer = session->ReceiveBuffer;

					recvBuffer.MoveRear(cbTransferred);

					while (!recvBuffer.IsEmpty())
					{
						if (!recvBuffer.TryPeek((char*)&header, sizeof(header)))
						{
							break;
						}
						if (recvBuffer.GetSize() < header.Length + sizeof(header))
						{
							break;
						}
						InterlockedIncrement(&server->mNumRecv);

						Message* message = Message::Create();
						{
							recvBuffer.MoveFront(sizeof(header));
							recvBuffer.TryDequeue(message->GetRear(), header.Length);

							message->MoveWritePos(header.Length);

							server->mListener->OnRecv(session->Id, message);
						}
						Message::Release(message);
					}
					server->recvPost(session);
				}
				break;
				case EIOType::Send:
				{
					int numSent = session->NumSent;

					InterlockedAdd((LONG*)&server->mNumSend, numSent);

					for (int i = 0; i < numSent; ++i)
					{
						Message::Release(session->SentMessages[i]);
					}
					session->NumSent = 0;
					session->bIsSending = false;

					server->sendPost(session);
				}
				break;
				default:
					std::cout << "Unhandled IO Type Error" << std::endl;
					break;
				}
			}

			if (InterlockedDecrement16(&session->Verifier.CurrentAsyncIOCount) == 0)
			{
				server->releaseSession(session);
			}
		}
		return 0;
	}

	void LANServer::sendPost(Session* session)
	{
		int		retval;
		WSABUF	buffers[Session::MAX_ASYNC_SENDS];

		if (session->bShouldDisconnect)
		{
			return;
		}
		if (!session->SendQueue.IsEmpty() && InterlockedExchange8(reinterpret_cast<char*>(&session->bIsSending), true) == false)
		{
			InterlockedIncrement16(&session->Verifier.CurrentAsyncIOCount);

			ZeroMemory(&session->SendOverlapped.Overlapped, sizeof(session->SendOverlapped.Overlapped));

			Message* sendMessage;
			DWORD numSend = 0;

			while (session->SendQueue.Dequeue(&sendMessage))
			{
				LANPacketHeader* header = (LANPacketHeader*)sendMessage->GetBuffer();

				buffers[numSend].buf = (char*)header;
				buffers[numSend].len = sizeof(*header) + header->Length;

				session->SentMessages[numSend] = sendMessage;
				++numSend;
			}
			session->NumSent = numSend;

			retval = WSASend(session->Socket, buffers, numSend, nullptr, 0, &session->SendOverlapped.Overlapped, nullptr);

			if (retval == SOCKET_ERROR)
			{
				int errorCode = WSAGetLastError();
				if (errorCode != ERROR_IO_PENDING)
				{
					for (DWORD i = 0; i < session->NumSent; ++i)
					{
						Message::Release(session->SentMessages[i]);
					}
					session->NumSent = 0;
					session->bIsSending = false;

					if (errorCode != WSAECONNRESET && errorCode != WSAEINVAL)
					{
						mListener->OnError(errorCode, L"WASSend Error");
					}
					if (InterlockedDecrement16(&session->Verifier.CurrentAsyncIOCount) == 0)
					{
						releaseSession(session);
					}
				}
			}
		}
	}

	void LANServer::recvPost(Session* session)
	{
		int retval;
		WSABUF buffer;

		if (session->bShouldDisconnect)
		{
			return;
		}
		InterlockedIncrement16(&session->Verifier.CurrentAsyncIOCount);

		RingBuffer& recvBuffer = session->ReceiveBuffer;

		ZeroMemory(&session->RecvOverlapped.Overlapped, sizeof(session->RecvOverlapped.Overlapped));

		buffer.buf = recvBuffer.GetRear();
		buffer.len = recvBuffer.GetDirectEnqueueableSize();
		DWORD flags = 0;

		retval = WSARecv(session->Socket, &buffer, 1, nullptr, &flags, &session->RecvOverlapped.Overlapped, nullptr);

		if (retval == SOCKET_ERROR)
		{
			int errorCode = WSAGetLastError();
			if (errorCode != ERROR_IO_PENDING)
			{
				if (errorCode != WSAECONNRESET)
				{
					mListener->OnError(errorCode, L"WASRecv Error");
				}
				if (InterlockedDecrement16(&session->Verifier.CurrentAsyncIOCount) == 0)
				{
					releaseSession(session);
				}
			}
		}
	}

	void LANServer::releaseSession(Session* session)
	{
		if (InterlockedCompareExchange((long*)&session->Verifier, 1, 0) != 0)
		{
			return;
		}
		sessionID_t id = session->Id;

		closesocket(session->Socket);

		session->Id = INVALID_SESSION_ID;
		session->Socket = INVALID_SOCKET;

		Message* sendMessage;

		while (session->SendQueue.Dequeue(&sendMessage))
		{
			Message::Release(sendMessage);
		}
		for (DWORD i = 0; i < session->NumSent; ++i)
		{
			Message::Release(session->SentMessages[i]);
		}
		session->bShouldDisconnect = false;
		session->bIsSending = false;

		session->NumSent = 0;
		session->SendQueue.Clear();
		session->ReceiveBuffer.Clear();

		InterlockedIncrement16(&session->Verifier.CurrentAsyncIOCount);

		mListener->OnClientLeave(id);

		mUseableIndexes->Push(EXTRACT_INDEX_FROM_ID(id));

		InterlockedDecrement(&mCurrentSessionCount);
	}

	Session* LANServer::acquireSessionOrNull(const sessionID_t ID)
	{
		uint64_t index = EXTRACT_INDEX_FROM_ID(ID);

		if (mMaximumSessionCount <= index)
		{
			return nullptr;
		}
		Session* session = &mSessions[index];

		InterlockedIncrement16(&session->Verifier.CurrentAsyncIOCount);

		if (session->Verifier.ReleaseFlag)
		{
			if (InterlockedDecrement16(&session->Verifier.CurrentAsyncIOCount) == 0)
			{
				releaseSession(session);
			}
			return nullptr;
		}
		if (session->Id != ID)
		{
			if (InterlockedDecrement16(&session->Verifier.CurrentAsyncIOCount) == 0)
			{
				releaseSession(session);
			}
			return nullptr;
		}

		return session;
	}
}