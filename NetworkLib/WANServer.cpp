#include "pch.h"

namespace NetworkLib
{
	WANServer::WANServer()
		: mIP(0),
		mPort(0),
		mFIxedKey(0),
		mListenSocket(INVALID_SOCKET),
		mSessions(nullptr),
		mListener(nullptr),
		mUseableIndexes(nullptr),
		mhCompletionPort(INVALID_HANDLE_VALUE),
		mhAcceptThread(INVALID_HANDLE_VALUE),
		mNumWorkingThreads(0),
		mNumWorkerThreads(0),
		mNumAccept(0),
		mNumRecv(0),
		mNumSend(0),
		mMaximumSessionCount(0),
		mCurrentSessionCount(0),
		mNumSendQueueFullDisconnected(0),
		mNumInvalidSessionDisconnected(0),
		mNumLimitSessionCountDisconnected(0)
	{
	}

	WANServer::~WANServer()
	{
		Terminate();
	}

	bool WANServer::TryRun(const unsigned long IP, const unsigned short port
		, const unsigned int numWorkerThread, const unsigned int numRunningThread
		, const unsigned int maxSessionCount, const bool bSupportsNagle
		, const unsigned char fixedKey, INetworkServerEventListener* listener)
	{
		int retval;

		mIP = IP;
		mPort = port;
		mListener = listener;
		mFIxedKey = fixedKey;
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

	void WANServer::Terminate()
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

	bool WANServer::TrySendMessage(const sessionID_t ID, Message* messagePtr)
	{
		Session* target = acquireSessionOrNull(ID);

		if (target == nullptr)
		{
			return false;
		}
		if (DISCONNECTION_SEND_QUEUE_SIZE < target->SendQueue.GetSize())
		{
			disconnectSession(target);

			InterlockedIncrement(&mNumSendQueueFullDisconnected);

			if (InterlockedDecrement16(&target->Verifier.CurrentAsyncIOCount) == 0)
			{
				releaseSession(target);
			}
			return false;
		}
		messagePtr->AddReferenceCount();

		WANPacketHeader* header = (WANPacketHeader*)messagePtr->GetBuffer();

		if (messagePtr->isPending())
		{
			header->Code = PACKET_CODE;
			header->Length = messagePtr->GetSize();
			header->RandKey = (BYTE)std::rand();
			header->CheckSum = Crypto::CheckSum(messagePtr->GetFront(), header->Length);

			Crypto::Encode(header, messagePtr->GetFront(), mFIxedKey);

			messagePtr->dispend();
		}
		target->SendQueue.Enqueue(messagePtr);

		//ZeroMemory(&target->AsyncSendOverlapped.Overlapped, sizeof(target->AsyncSendOverlapped.Overlapped));
		//PostQueuedCompletionStatus(mhCompletionPort, 1, (ULONG_PTR)target, &target->AsyncSendOverlapped.Overlapped);
		sendPost(target);

		if (InterlockedDecrement16(&target->Verifier.CurrentAsyncIOCount) == 0)
		{
			releaseSession(target);
		}
		return true;
	}

	bool WANServer::TryDisconnect(const sessionID_t ID)
	{
		Session* target = acquireSessionOrNull(ID);

		if (target == nullptr)
		{
			return false;
		}
		InterlockedIncrement(&mNumRequestDisconnected);

		disconnectSession(target);

		if (InterlockedDecrement16(&target->Verifier.CurrentAsyncIOCount) == 0)
		{
			releaseSession(target);
		}
		return true;
	}

	unsigned long WANServer::GetIP() const
	{
		return mIP;
	}

	unsigned short WANServer::GetPort() const
	{
		return mPort;
	}

	unsigned int WANServer::GetMaximumSessionCount() const
	{
		return mMaximumSessionCount;
	}

	unsigned int WANServer::GetCurrentSessionCount() const
	{
		return mCurrentSessionCount;
	}

	unsigned int WANServer::GetMessagePoolActiveCount() const
	{
		return Chunk<Message>::GetActiveCount();
	}

	unsigned int WANServer::GetNumRequestDisconnected() const
	{
		return mNumRequestDisconnected;
	}

	unsigned int WANServer::GetNumSendQueueFullDisconnected() const
	{
		return mNumSendQueueFullDisconnected;
	}

	unsigned int WANServer::GetNumInvalidSessionDisconnected() const
	{
		return mNumInvalidSessionDisconnected;
	}

	unsigned int WANServer::GetNumLimitSessionDisconnected() const
	{
		return mNumLimitSessionCountDisconnected;
	}

	unsigned int WANServer::GetNumAccept() const
	{
		return mNumAccept;
	}

	unsigned int WANServer::GetNumRecv() const
	{
		return mNumRecv;
	}

	unsigned int WANServer::GetNumSend() const
	{
		return mNumSend;
	}

	unsigned int WANServer::GetNumWorkingThread() const
	{
		return mNumWorkingThreads;
	}

	Message* WANServer::CreateMessage()
	{
		Message* msg = Message::Create();

		msg->MoveReadPos(sizeof(WANPacketHeader));
		msg->MoveWritePos(sizeof(WANPacketHeader));

		return msg;
	}

	void WANServer::ReleaseMessage(Message* message)
	{
		Message::Release(message);
	}

	unsigned int __stdcall WANServer::acceptThread(void* param)
	{
		WANServer* server = static_cast<WANServer*>(param);

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
				++server->mNumLimitSessionCountDisconnected;

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


	unsigned int __stdcall WANServer::workerThread(void* param)
	{
		WANServer* server = (WANServer*)param;

		const unsigned char fixedKey = server->mFIxedKey;

		InterlockedIncrement(&server->mNumWorkingThreads);
		while (true)
		{
			DWORD cbTransferred = 0;
			Session* session = nullptr;
			Session::OverlappedExtension* overlapped;

			bool bSucceded = GetQueuedCompletionStatus(server->mhCompletionPort
				, &cbTransferred, (PULONG_PTR)&session, (LPOVERLAPPED*)&overlapped, INFINITE);

			if (bSucceded == true)
			{
				if (overlapped == nullptr)
				{
					break;
				}
			}
			else
			{
				int errorCode = GetLastError();

				if (overlapped == nullptr)
				{
					server->mListener->OnError(errorCode, L"Failed GQCS and Overlapped is null");
					break;
				}
				else if (errorCode != ERROR_NETNAME_DELETED && errorCode != WSAECONNRESET && errorCode != ERROR_OPERATION_ABORTED && errorCode != 0)
				{
					server->mListener->OnError(errorCode, L"Failed I/O Operation");
				}
				server->disconnectSession(session);

				goto RELEASE_SESSION;
			}

			//Overlapped I/O Completion Processing
			if (cbTransferred != 0 && overlapped != nullptr && !session->bShouldDisconnect)
			{
				switch (overlapped->Type)
				{
				case EIOType::Recv:
				{
					RingBuffer& recvBuffer = session->ReceiveBuffer;
					unsigned int numRecv = 0;

					recvBuffer.MoveRear(cbTransferred);

					WANPacketHeader header;
					while (!recvBuffer.IsEmpty())
					{
						if (!recvBuffer.TryPeek((char*)&header, sizeof(header)))
						{
							break;
						}
						if (header.Code != PACKET_CODE || recvBuffer.GetCapacity() - sizeof(header) < header.Length)
						{
							InterlockedIncrement(&server->mNumInvalidSessionDisconnected);

							goto RELEASE_SESSION;
						}
						if (recvBuffer.GetSize() < header.Length + sizeof(header))
						{
							break;
						}
						++numRecv;

						Message* message = Message::Create();
						{
							recvBuffer.MoveFront(sizeof(header));
							recvBuffer.TryDequeue(message->GetRear(), header.Length);

							message->MoveWritePos(header.Length);

							Crypto::Decode(&header, message->GetFront(), fixedKey);

							if (header.CheckSum != Crypto::CheckSum(message->GetFront(), header.Length))
							{
								Message::Release(message);

								InterlockedIncrement(&server->mNumInvalidSessionDisconnected);

								goto RELEASE_SESSION;
							}
							server->mListener->OnRecv(session->Id, message);
						}
						Message::Release(message);
					}
					InterlockedAdd((LONG*)&server->mNumRecv, numRecv);
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
				case EIOType::AsyncSend:
				{
					server->sendPost(session);
				}
				break;
				default:
					std::cout << "Unhandled IO Type Error" << std::endl;
					break;
				}
			}

		RELEASE_SESSION:
			if (InterlockedDecrement16(&session->Verifier.CurrentAsyncIOCount) == 0)
			{
				server->releaseSession(session);
			}
		}
		InterlockedDecrement(&server->mNumWorkingThreads);
		return 0;
	}

	void WANServer::sendPost(Session* session)
	{
		int		retval;
		WSABUF	buffers[Session::MAX_ASYNC_SENDS];

		if (session->bShouldDisconnect)
		{
			return;
		}
		if (!session->SendQueue.IsEmpty() && InterlockedExchange8(reinterpret_cast<char*>(&session->bIsSending), true) == false)
		{
			if (session->SendQueue.IsEmpty())
			{
				session->bIsSending = false;
				return;
			}
			InterlockedIncrement16(&session->Verifier.CurrentAsyncIOCount);

			ZeroMemory(&session->SendOverlapped.Overlapped, sizeof(session->SendOverlapped.Overlapped));

			Message* sendMessage;
			DWORD numSend = 0;

			while (session->SendQueue.Dequeue(&sendMessage) && numSend <= Session::MAX_ASYNC_SENDS)
			{
				WANPacketHeader* header = (WANPacketHeader*)sendMessage->GetBuffer();

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
					disconnectSession(session);

					if (errorCode != WSAECONNRESET)
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

	void WANServer::recvPost(Session* session)
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
				disconnectSession(session);

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

	void WANServer::disconnectSession(Session* session)
	{
		session->bShouldDisconnect = true;
		CancelIoEx((HANDLE)session->Socket, nullptr);
	}

	void WANServer::releaseSession(Session* session)
	{
		if (InterlockedCompareExchange((long*)&session->Verifier.Verifier, 1, 0) != 0)
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

	Session* WANServer::acquireSessionOrNull(const sessionID_t ID)
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