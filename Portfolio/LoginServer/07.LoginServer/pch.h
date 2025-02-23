#pragma once
#pragma comment(lib, "C:\\Program Files\\MySQL\\MySQL Server 8.0\\lib\\mysqlclient.lib")
#pragma comment(lib, "Winmm.lib")

#include "WinSock2.h"
#include "Windows.h"
//SQL
#include "C:\\Program Files\\MySQL\\MySQL Server 8.0\\include\\mysql.h"
#include "C:\\Program Files\\MySQL\\MySQL Server 8.0\\include\\errmsg.h"

//Redis
#include <cpp_redis/cpp_redis>
#pragma comment (lib, "cpp_redis.lib")
#pragma comment (lib, "tacopie.lib")
#pragma comment (lib, "ws2_32.lib")

#include <iostream>
#include <stdio.h>
#include <string>
#include <conio.h>

#include "../../CommonProtocol.h"
#include "CommonDefine.h"

//#define PROFILE
//#include "Profiler.h"