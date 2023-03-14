#pragma once

#include "framework.h"

#pragma comment(lib, "ws2_32")

#include <iostream>
#include <WinSock2.h>
#include <tchar.h>
#include <float.h>
#include <stdio.h>
#include <string.h>
#include <Windows.h>
#include <process.h>
#include <unordered_set>
#include <unordered_map>
#include <memory>

#define CACHE_ALIGN __declspec(align(64))

typedef uint64_t sessionID_t;

#include "EIOType.h"

#include "Chunk.h"
#include "ObjectPool.h"
#include "TLSObjectPoolMiddleware.h"
#include "TLSObjectPool.h"

#include "LockFreeStack.h"
#include "LockFreeQueue.h"

#include "Message.h"
#include "RingBuffer.h"
#include "INetworkServerEventListener.h"
#include "INetworkClientEventListener.h"
#include "Session.h"
#include "PacketHeader.h"
#include "Crypto.h"
#include "LANServer.h"
#include "WANServer.h"
#include "LANClient.h"
#include "WANClient.h"