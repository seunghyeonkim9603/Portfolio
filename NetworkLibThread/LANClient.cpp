#include "pch.h"

#define MAKE_ID(unique, index) (((unique) << 16) | (index))
#define EXTRACT_INDEX_FROM_ID(id) ((id) & 0x000000000000FFFF)

namespace NetworkLib
{
	LANClient::LANClient()
		: mIP(0),
		mPort(0),
		mListener(nullptr),
		mhCompletionPort(INVALID_HANDLE_VALUE),
		mNumRecv(0),
		mNumSend(0)
	{
	}

	LANClient::~LANClient()
	{
		Terminate();
	}

	bool LANClient::TryRun(const unsigned long IP, const unsigned short port
		, const unsigned int numWorkerThread, const unsigned int numRunningThread
		, const bool bSupportsNagle, INetworkClientEventListener* listener)
	{
		int retval;

		mIP = IP;
		mPort = port;
		mListener = listener;

		mNumWorkerThreads = numWorkerThread;

		mClient.Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (mClient.Socket == INVALID_SOCKET)
		{
			return false;
		}

		LINGER lingerOptval;
		{
			lingerOptval.l_onoff = 1;
			lingerOptval.l_linger = 0;
		}
		retval = setsockopt(mClient.Socket, SOL_SOCKET, SO_LINGER, (char*)&lingerOptval, sizeof(lingerOptval));

		if (retval == SOCKET_ERROR)
		{
			return false;
		}
		if (!bSupportsNagle)
		{
			bool flag = true;
			retval = setsockopt(mClient.Socket, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));
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

		SOCKADDR_IN serverAddr;
		{
			ZeroMemory(&serverAddr, sizeof(serverAddr));
			serverAddr.sin_family = AF_INET;
			serverAddr.sin_addr.s_addr = htonl(IP);
			serverAddr.sin_port = htons(port);
		}
		retval = connect(mClient.Socket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));

		if (retval == SOCKET_ERROR)
		{
			return false;
		}
		CreateIoCompletionPort((HANDLE)mClient.Socket, mhCompletionPort, (ULONG_PTR)&mClient, 0);

		for (unsigned int i = 0; i < numWorkerThread; ++i)
		{
			mhWorkerThreads[i] = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, &workerThread, this, 0, nullptr));
		}

		mListener->OnConnect();

		return true;
	}

	void LANClient::Terminate()
	{
		closesocket(mClient.Socket);

		mClient.Socket = INVALID_SOCKET;

		for (unsigned int i = 0; i < mNumWorkerThreads; ++i)
		{
			PostQueuedCompletionStatus(mhCompletionPort, 0, 0, nullptr);
		}
		WaitForMultipleObjects(static_cast<DWORD>(mNumWorkerThreads), mhWorkerThreads, true, INFINITE);

		for (unsigned int i = 0; i < mNumWorkerThreads; ++i)
		{
			CloseHandle(mhWorkerThreads[i]);

			mhWorkerThreads[i] = INVALID_HANDLE_VALUE;
		}
		CloseHandle(mhCompletionPort);

		mhCompletionPort = INVALID_HANDLE_VALUE;
	}

	bool LANClient::TrySendMessage(Message* messagePtr)
	{
		if (!tryAcquireClient())
		{
			return false;
		}
		if (DISCONNECTION_SEND_QUEUE_SIZE < mClient.SendQueue.GetSize())
		{
			CancelIoEx((HANDLE)mClient.Socket, nullptr);

			if (InterlockedDecrement16(&mClient.Verifier.CurrentAsyncIOCount) == 0)
			{
				releaseClient();
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
		mClient.SendQueue.Enqueue(messagePtr);

		PostQueuedCompletionStatus(mhCompletionPort, 1, (ULONG_PTR)&mClient, &mClient.AsyncSendOverlapped.Overlapped);
		/*sendPost(target);

		if (InterlockedDecrement16(&target->Verifier.CurrentAsyncIOCount) == 0)
		{
			releaseSession(target);
		}*/
		return true;
	}

	bool LANClient::TryDisconnect()
	{
		if (!tryAcquireClient())
		{
			return false;
		}
		CancelIoEx((HANDLE)mClient.Socket, nullptr);

		if (InterlockedDecrement16(&mClient.Verifier.CurrentAsyncIOCount) == 0)
		{
			releaseClient();
		}
		return true;
	}

	unsigned long LANClient::GetIP() const
	{
		return mIP;
	}

	unsigned short LANClient::GetPort() const
	{
		return mPort;
	}

	unsigned int LANClient::GetMessagePoolActiveCount() const
	{
		return Chunk<Message>::GetActiveCount();
	}

	unsigned int LANClient::GetNumRecv() const
	{
		return mNumRecv;
	}

	unsigned int LANClient::GetNumSend() const
	{
		return mNumSend;
	}

	Message* LANClient::CreateMessage()
	{
		Message* msg = Message::Create();

		msg->MoveReadPos(sizeof(LANPacketHeader));
		msg->MoveWritePos(sizeof(LANPacketHeader));

		return msg;
	}

	void LANClient::ReleaseMessage(Message* message)
	{
		Message::Release(message);
	}


	unsigned int __stdcall LANClient::workerThread(void* param)
	{
		LANClient* client = (LANClient*)param;

		while (true)
		{
			DWORD cbTransferred = 0;
			Session* session = nullptr;
			Session::OverlappedExtension* overlapped;

			bool bSucceded = GetQueuedCompletionStatus(client->mhCompletionPort
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
					client->mListener->OnError(errorCode, L"Failed GQCS and Overlapped is null");
					break;
				}
				else if (errorCode != ERROR_NETNAME_DELETED && errorCode != ERROR_IO_PENDING
					&& errorCode != WSAECONNRESET && errorCode != ERROR_OPERATION_ABORTED && errorCode != 0)
				{
					client->mListener->OnError(errorCode, L"Failed I/O Operation");
				}
			}

			//Overlapped I/O Completion Processing
			if (cbTransferred != 0 && overlapped != nullptr)
			{
				switch (overlapped->Type)
				{
				case EIOType::Recv:
				{
					RingBuffer& recvBuffer = session->ReceiveBuffer;

					recvBuffer.MoveRear(cbTransferred);

					LANPacketHeader header;
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
						InterlockedIncrement(&client->mNumRecv);

						Message* message = Message::Create();
						{
							recvBuffer.MoveFront(sizeof(header));
							recvBuffer.TryDequeue(message->GetRear(), header.Length);

							message->MoveWritePos(header.Length);

							client->mListener->OnRecv(message);
						}
						Message::Release(message);
					}
					client->recvPost();
				}
				break;
				case EIOType::Send:
				{
					int numSent = session->NumSent;

					InterlockedAdd((LONG*)&client->mNumSend, numSent);

					for (int i = 0; i < numSent; ++i)
					{
						Message::Release(session->SentMessages[i]);
					}
					session->NumSent = 0;
					session->bIsSending = false;

					client->sendPost();
				}
				break;
				case EIOType::AsyncSend:
				{
					client->sendPost();
				}
				break;
				default:
					std::cout << "Unhandled IO Type Error" << std::endl;
					break;
				}
			}
			if (InterlockedDecrement16(&session->Verifier.CurrentAsyncIOCount) == 0)
			{
				client->releaseClient();
			}
		}
		return 0;
	}

	void LANClient::sendPost()
	{
		int		retval;
		WSABUF	buffers[Session::MAX_ASYNC_SENDS];

		if (!mClient.SendQueue.IsEmpty() && InterlockedExchange8(reinterpret_cast<char*>(&mClient.bIsSending), true) == false)
		{
			InterlockedIncrement16(&mClient.Verifier.CurrentAsyncIOCount);

			ZeroMemory(&mClient.SendOverlapped.Overlapped, sizeof(mClient.SendOverlapped.Overlapped));

			Message* sendMessage;
			DWORD numSend = 0;

			while (mClient.SendQueue.Dequeue(&sendMessage))
			{
				LANPacketHeader* header = (LANPacketHeader*)sendMessage->GetBuffer();

				buffers[numSend].buf = (char*)header;
				buffers[numSend].len = sizeof(*header) + header->Length;

				mClient.SentMessages[numSend] = sendMessage;
				++numSend;
			}
			mClient.NumSent = numSend;

			retval = WSASend(mClient.Socket, buffers, numSend, nullptr, 0, &mClient.SendOverlapped.Overlapped, nullptr);

			if (retval == SOCKET_ERROR)
			{
				int errorCode = WSAGetLastError();
				if (errorCode != ERROR_IO_PENDING)
				{
					for (DWORD i = 0; i < mClient.NumSent; ++i)
					{
						Message::Release(mClient.SentMessages[i]);
					}
					mClient.NumSent = 0;
					mClient.bIsSending = false;

					if (errorCode != WSAECONNRESET && errorCode != WSAEINVAL)
					{
						mListener->OnError(errorCode, L"WASSend Error");
					}
					if (InterlockedDecrement16(&mClient.Verifier.CurrentAsyncIOCount) == 0)
					{
						releaseClient();
					}
				}
			}
		}
	}

	void LANClient::recvPost()
	{
		int retval;
		WSABUF buffer;

		InterlockedIncrement16(&mClient.Verifier.CurrentAsyncIOCount);

		RingBuffer& recvBuffer = mClient.ReceiveBuffer;

		ZeroMemory(&mClient.RecvOverlapped.Overlapped, sizeof(mClient.RecvOverlapped.Overlapped));

		buffer.buf = recvBuffer.GetRear();
		buffer.len = recvBuffer.GetDirectEnqueueableSize();
		DWORD flags = 0;

		retval = WSARecv(mClient.Socket, &buffer, 1, nullptr, &flags, &mClient.RecvOverlapped.Overlapped, nullptr);

		if (retval == SOCKET_ERROR)
		{
			int errorCode = WSAGetLastError();
			if (errorCode != ERROR_IO_PENDING)
			{
				if (errorCode != WSAECONNRESET)
				{
					mListener->OnError(errorCode, L"WASRecv Error");
				}
				if (InterlockedDecrement16(&mClient.Verifier.CurrentAsyncIOCount) == 0)
				{
					releaseClient();
				}
			}
		}
	}

	void LANClient::releaseClient()
	{
		if (InterlockedCompareExchange((long*)&mClient.Verifier.Verifier, 1, 0) != 0)
		{
			return;
		}
		closesocket(mClient.Socket);

		mClient.Socket = INVALID_SOCKET;

		Message* sendMessage;

		while (mClient.SendQueue.Dequeue(&sendMessage))
		{
			Message::Release(sendMessage);
		}
		for (DWORD i = 0; i < mClient.NumSent; ++i)
		{
			Message::Release(mClient.SentMessages[i]);
		}
		mClient.bIsSending = false;

		mClient.NumSent = 0;
		mClient.SendQueue.Clear();
		mClient.ReceiveBuffer.Clear();

		InterlockedIncrement16(&mClient.Verifier.CurrentAsyncIOCount);

		mListener->OnDisconnect();
	}

	bool LANClient::tryAcquireClient()
	{
		InterlockedIncrement16(&mClient.Verifier.CurrentAsyncIOCount);

		if (mClient.Verifier.ReleaseFlag)
		{
			if (InterlockedDecrement16(&mClient.Verifier.CurrentAsyncIOCount) == 0)
			{
				releaseClient();
			}
			return false;
		}
		return true;
	}
}