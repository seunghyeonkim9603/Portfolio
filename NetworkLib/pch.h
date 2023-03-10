// pch.h: 미리 컴파일된 헤더 파일입니다.
// 아래 나열된 파일은 한 번만 컴파일되었으며, 향후 빌드에 대한 빌드 성능을 향상합니다.
// 코드 컴파일 및 여러 코드 검색 기능을 포함하여 IntelliSense 성능에도 영향을 미칩니다.
// 그러나 여기에 나열된 파일은 빌드 간 업데이트되는 경우 모두 다시 컴파일됩니다.
// 여기에 자주 업데이트할 파일을 추가하지 마세요. 그러면 성능이 저하됩니다.

#ifndef PCH_H
#define PCH_H

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

typedef uint64_t sessionID_t;

#define MAKE_ID(unique, index) (((unique) << 16) | (index))
#define EXTRACT_INDEX_FROM_ID(id) ((id) & 0x000000000000FFFF)
#define CACHE_ALIGN __declspec(align(64))

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

#endif //PCH_H
