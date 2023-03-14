#pragma once

namespace NetworkLib
{
	struct Session
	{
		enum { MAX_ASYNC_SENDS = 256 };

		struct OverlappedExtension
		{
			OVERLAPPED	Overlapped;
			EIOType		Type;
		};
		static_assert(offsetof(OverlappedExtension, Overlapped) == 0, "Offset of Overlapped is not Valid");

		union ReleaseVerifier
		{
			struct
			{
				INT16 ReleaseFlag;
				INT16 CurrentAsyncIOCount;
			};
			INT32 Verifier;
		};

		Session()
			: NumSent(0),
			bShouldDisconnect(false),
			bIsSending(false),
			SendQueue()
		{
			Verifier.ReleaseFlag = 0;
			Verifier.CurrentAsyncIOCount = 1;

			RecvOverlapped.Type = EIOType::Recv;
			SendOverlapped.Type = EIOType::Send;
			AsyncSendOverlapped.Type = EIOType::AsyncSend;
		}

		sessionID_t				Id;
		SOCKADDR_IN				Addr;
		SOCKET					Socket;
		BOOL					bShouldDisconnect;
		RingBuffer				ReceiveBuffer;

		DWORD					NumSent;
		BOOL					bIsSending;
		LockFreeQueue<Message*> SendQueue;
		Message*				SentMessages[MAX_ASYNC_SENDS];

		OverlappedExtension		RecvOverlapped;
		OverlappedExtension		SendOverlapped;
		OverlappedExtension		AsyncSendOverlapped;

		ReleaseVerifier			Verifier;
	};
}