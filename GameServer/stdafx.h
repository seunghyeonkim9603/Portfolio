#pragma once

#pragma comment(lib, "NetworkLib.lib")
#pragma comment(lib, "pdh.lib")
#pragma comment (lib, "DbgHelp.Lib")
#pragma comment (lib, "cpp_redis.lib")
#pragma comment (lib, "tacopie.lib")

#include "NetworkLib/NetworkLib.h"
#include <cpp_redis/cpp_redis>
#include <codecvt>

#include <DbgHelp.h>
#include <crtdbg.h>
#include <Psapi.h>
#include <fstream>
#include <strsafe.h>
#include <stdio.h>
#include <stdarg.h>

#include "mysql.h"
#include "errmsg.h"

#include <Pdh.h>
#include <Psapi.h>

#include "Logger.h"
#include "CCrashDump.h"

#include "MonitoringClient.h"
#include "ProcessDataHelper.h"
#include "ProcessTaskManager.h"

#include "DBConnector.h"
#include "DBWriterThread.h"

#include "CommonProtocol.h"

#include "ECharacterType.h"
#include "EContentType.h"
#include "EObjectType.h"
#include "EObjectStatus.h"

#include "GameServer.h"
#include "ClientIDGenerator.h"

#include "Point.h"
#include "Object.h"
#include "Player.h"
#include "Monster.h"
#include "Cristal.h"
#include "Sector.h"

#include "LoginThread.h"
#include "SelectCharacterThread.h"
#include "GameThread.h"