#include "MonitoringLanServer.h"
#include "MonitoringNetServer.h"
#include "Tools/Message.h"
#include "../../CommonProtocol.h"
#include "Tools/Parser.h"
#include "wchar.h"
#include "DBConnection.h"
#include <string>
#include <iomanip>
#include <sstream>

MonitoringLanServer::MonitoringLanServer(MonitoringNetServer* pNetServer)
	:_pNetServer(pNetServer)
{
	InitializeSRWLock(&_lock);
}

MonitoringLanServer::~MonitoringLanServer()
{
	for (auto element : _DBConnections)
	{
		delete element;
	}
}

bool MonitoringLanServer::Start(const WCHAR* configFileName)
{
	//네트워크 라이브러리의 Start 호출
	LanServer::Start(configFileName);

	if (!LoadLoginServerConfig())
	{
		wprintf(L"[ERROR] 모니터링 랜 서버 Config Fail\n");
		return false;
	}

	_DBConnections.reserve(_IOCP_WorkerThreadNum);
	wprintf(L"DB IP : %s / DB Port : %d\n",_DBServerIP, _DBServerPort);

	wprintf(L"# 모니터링 랜 서버 스타트.\n");
	return true;
}

void MonitoringLanServer::Stop()
{
	LanServer::Stop();

	wprintf(L"# 모니터링 랜 서버 스탑.\n");
}

bool MonitoringLanServer::OnConnectionRequest(const WCHAR* connectIP, INT32 connectPort)
{
	return true;
}

void MonitoringLanServer::OnAccept(INT64 sessionID)
{

}

void MonitoringLanServer::OnRelease(INT64 sessionID)
{
	AcquireSRWLockExclusive(&_lock);
	//끊어진 서버를 판단한다.
	int serverID = _servers[sessionID];
	_servers.erase(sessionID);
	_isConnectServers[serverID] = false;
	ReleaseSRWLockExclusive(&_lock);
}

void MonitoringLanServer::OnRecv(INT64 sessionID, Message* pMessage)
{
	WORD messageType = -1;
	(*pMessage) >> messageType;

	switch (messageType)
	{
	case en_PACKET_SS_MONITOR_LOGIN:
		HandleMonitorLoginMessage(sessionID, pMessage);
		break;
	case en_PACKET_SS_MONITOR_DATA_UPDATE:
		HandleMonitorDataUpdateMessage(sessionID, pMessage);
		break;
	default:
		Disconnect(sessionID);
		SystemLog::GetInstance()->WriteLog(L"ERROR", SystemLog::LOG_LEVEL_ERROR,
			L"알 수 없는 타입의 패킷");
		break;
	}
	pMessage->SubRefCount();
}

void MonitoringLanServer::OnSend(INT64 sessionID, INT32 sendSize)
{
}

void MonitoringLanServer::OnTime()
{
	tickCount++;
	if (tickCount < DB_SAVE_INTERVAL)
		return;

	tickCount = 0;

	AcquireSRWLockExclusive(&_lock);
	//TLS에 있는 DBConnection 초기화
	if (_pDBConnection == nullptr)
	{
		//WCHAR -> CHAR 변환
		char DBServerIP[IP_LEN] = { 0, };
		char DBUserName[DB_CONFIG_LEN] = { 0, };
		char DBPassword[DB_CONFIG_LEN] = { 0, };
		char DBSchemaName[DB_CONFIG_LEN] = { 0, };
		ConvertWcharToChar(_DBServerIP, DBServerIP, sizeof(DBServerIP));
		ConvertWcharToChar(_DBUserName, DBUserName, sizeof(DBUserName));
		ConvertWcharToChar(_DBPassword, DBPassword, sizeof(DBPassword));
		ConvertWcharToChar(_DBSchemaName, DBSchemaName, sizeof(DBSchemaName));

		_pDBConnection = new DBConnection(DBServerIP, DBUserName, DBPassword, DBSchemaName, _DBServerPort);
		_DBConnections.push_back(_pDBConnection);
	}
	//모든 항목 DB 저장

	// DB 테이블 이름 얻기
	SYSTEMTIME systime;
	GetSystemTime(&systime);
	std::ostringstream tableNameStream;
	//setw는 일회용으로 자릿수 맞춰주는 함수, setfill은 그 빈 자리수를 채워주는 문자를 지정함
	tableNameStream << "monitorlog_" << systime.wYear << std::setw(2) << std::setfill('0') << systime.wMonth;
	std::string tableName = tableNameStream.str();

	//트랜잭션 시작 ===============================================
	std::string beginTransaction = "START TRANSACTION;";
	int query_stat = mysql_query(_pDBConnection->GetConnection(), beginTransaction.c_str());
	if (query_stat != 0)
	{
		//전송 실패 시 연결 끊기
		return;
	}

	if (_isConnectServers[static_cast<int>(SERVER_ID::LOGIN)])
	{
		for (int type = LOGIN_DATA_ARR_START; type <= LOGIN_DATA_ARR_END; type++)
		{
			DataStatistics& data = _loginServerDatas[type - LOGIN_DATA_ARR_START];

			int avg = 0;
			if (data._count > 0)
			{
				avg = data._sum / data._count;
			}
			InsertDBQuery(tableName, static_cast<int>(SERVER_ID::LOGIN), type, avg, data._min, data._max);
			data.clear();
		}
	}

	if (_isConnectServers[static_cast<int>(SERVER_ID::GAME)])
	{
		for (int type = GAME_DATA_ARR_START; type <= GAME_DATA_ARR_END; type++)
		{
			DataStatistics& data = _gameServerDatas[type - GAME_DATA_ARR_START];
			int avg = 0;
			if (data._count > 0)
			{
				avg = data._sum / data._count;
			}
			InsertDBQuery(tableName, static_cast<int>(SERVER_ID::GAME), type, avg, data._min, data._max);
			data.clear();
		}
	}

	if (_isConnectServers[static_cast<int>(SERVER_ID::CHAT)])
	{
		for (int type = CHAT_DATA_ARR_START; type < CHAT_DATA_ARR_END; type++)
		{
			DataStatistics& data = _chatServerDatas[type - CHAT_DATA_ARR_START];
			int avg = 0;
			if (data._count > 0)
			{
				avg = data._sum / data._count;
			}
			InsertDBQuery(tableName, static_cast<int>(SERVER_ID::CHAT), type, avg, data._min, data._max);
			data.clear();
		}
	}

	for (int serverID = 1; serverID <= SERVER_NUM; serverID++)
	{
		if (_isConnectServers[serverID])
		{
			for (int type = COMMON_DATA_ARR_START; type < COMMON_DATA_ARR_END; type++)
			{
				DataStatistics& data = _commonServerDatas[serverID][type - COMMON_DATA_ARR_START];
				int avg = 0;
				if (data._count > 0)
				{
					avg = data._sum / data._count;
				}
				InsertDBQuery(tableName, serverID, type, avg, data._min, data._max);
				data.clear();
			}
		}
	}

	std::string endTransaction = "COMMIT;";
	query_stat = mysql_query(_pDBConnection->GetConnection(), endTransaction.c_str());
	if (query_stat != 0)
	{
		//전송 실패 시 연결 끊기
		return;
	}
	//트랜젝션 끝 ===================================================
	ReleaseSRWLockExclusive(&_lock);
}

void MonitoringLanServer::OnError(INT32 errorCode, WCHAR* comment)
{
}

IUser* MonitoringLanServer::GetUser(INT64 sessionID)
{
	return nullptr;
}

void MonitoringLanServer::MakeMessage_MonitorToolDataUpdate(Message* pMessage, BYTE serverID, BYTE dataType, int dataValue, int timeStamp)
{
	*pMessage << static_cast<WORD>(en_PACKET_CS_MONITOR_TOOL_DATA_UPDATE)
		<< serverID << dataType << dataValue << timeStamp;
}

void MonitoringLanServer::HandleMonitorLoginMessage(INT64 sessionID, Message* pMessage)
{
	int serverID;
	if (pMessage->GetDataSize() != sizeof(serverID))
	{
		Disconnect(sessionID);
		__debugbreak();
		return;
	}

	*pMessage >> serverID;
	_isConnectServers[serverID] = true;

	AcquireSRWLockExclusive(&_lock);
	_servers.insert({ sessionID, serverID });
	ReleaseSRWLockExclusive(&_lock);
}

void MonitoringLanServer::HandleMonitorDataUpdateMessage(INT64 sessionID, Message* pMessage)
{
	BYTE dataType; 
	int dataValue; 
	int timeStamp;

	//패킷 유효성 검사
	if (pMessage->GetDataSize() != 
		sizeof(dataType) + sizeof(dataValue) + sizeof(timeStamp))
	{
		Disconnect(sessionID);
		__debugbreak();
		return;
	}
	//데이터 추출
	*pMessage >> dataType >> dataValue >> timeStamp;

	AcquireSRWLockExclusive(&_lock);
	//어떤 서버의 데이터인지 판별
	int serverID = _servers[sessionID];

	//수신 데이터를 내부 자료구조에 저장한다.
	SaveData(serverID, dataType, dataValue);

	ReleaseSRWLockExclusive(&_lock);

	//접속한 모니터 클라이언트가 있다면 수신한 데이터를 그대로 송신한다.
	if (_pNetServer->GetSessionCount() > 0)
	{
		Message* pMonitorToolDataUpdateMessage = Message::Alloc();
		MakeMessage_MonitorToolDataUpdate(pMonitorToolDataUpdateMessage, 
			serverID, dataType, dataValue, timeStamp);
		_pNetServer->SendPacket_AllSession(pMonitorToolDataUpdateMessage);
	}
}

void MonitoringLanServer::SaveData(int serverID, int dataType, int dataValue)
{
	//데이터 저장
	//동일 항목이 1초에 한 번씩 오기 때문에 인터락을 하지 않았다.
	if (COMMON_DATA_ARR_START <= dataType && dataType <= COMMON_DATA_ARR_END)
	{
		_commonServerDatas[serverID][dataType - COMMON_DATA_ARR_START]._count++;
		_commonServerDatas[serverID][dataType - COMMON_DATA_ARR_START]._sum += dataValue;

		if (_commonServerDatas[serverID][dataType - COMMON_DATA_ARR_START]._max < dataValue)
			_commonServerDatas[serverID][dataType - COMMON_DATA_ARR_START]._max = dataValue;
		if (_commonServerDatas[serverID][dataType - COMMON_DATA_ARR_START]._min > dataValue)
			_commonServerDatas[serverID][dataType - COMMON_DATA_ARR_START]._min = dataValue;
	}
	else if (LOGIN_DATA_ARR_START <= dataType && dataType <= LOGIN_DATA_ARR_END)
	{
		if (serverID != static_cast<int>(SERVER_ID::LOGIN))
		{
			__debugbreak();
		}

		_loginServerDatas[dataType - LOGIN_DATA_ARR_START]._count++;
		_loginServerDatas[dataType - LOGIN_DATA_ARR_START]._sum += dataValue;

		if (_loginServerDatas[dataType - LOGIN_DATA_ARR_START]._max < dataValue)
			_loginServerDatas[dataType - LOGIN_DATA_ARR_START]._max = dataValue;
		if (_loginServerDatas[dataType - LOGIN_DATA_ARR_START]._min > dataValue)
			_loginServerDatas[dataType - LOGIN_DATA_ARR_START]._min = dataValue;
	}
	else if (GAME_DATA_ARR_START <= dataType && dataType <= GAME_DATA_ARR_END)
	{
		if (serverID != static_cast<int>(SERVER_ID::GAME))
		{
			__debugbreak();
		}

		_gameServerDatas[dataType - GAME_DATA_ARR_START]._count++;
		_gameServerDatas[dataType - GAME_DATA_ARR_START]._sum += dataValue;

		if (_gameServerDatas[dataType - GAME_DATA_ARR_START]._max < dataValue)
			_gameServerDatas[dataType - GAME_DATA_ARR_START]._max = dataValue;
		if (_gameServerDatas[dataType - GAME_DATA_ARR_START]._min > dataValue)
			_gameServerDatas[dataType - GAME_DATA_ARR_START]._min = dataValue;
	}
	else if (CHAT_DATA_ARR_START <= dataType && dataType <= CHAT_DATA_ARR_END)
	{
		if (serverID != static_cast<int>(SERVER_ID::CHAT))
		{
			__debugbreak();
		}

		_chatServerDatas[dataType - CHAT_DATA_ARR_START]._count++;
		_chatServerDatas[dataType - CHAT_DATA_ARR_START]._sum += dataValue;

		if (_chatServerDatas[dataType - CHAT_DATA_ARR_START]._max < dataValue)
			_chatServerDatas[dataType - CHAT_DATA_ARR_START]._max = dataValue;
		if (_chatServerDatas[dataType - CHAT_DATA_ARR_START]._min > dataValue)
			_chatServerDatas[dataType - CHAT_DATA_ARR_START]._min = dataValue;
	}

}

bool MonitoringLanServer::LoadLoginServerConfig()
{
	Parser parser;

	if (!parser.LoadFile(L"MonitoringLanServer.cnf"))
	{
		return false;
	}
	parser.GetValue(L"DB_SERVER_IP", _DBServerIP, IP_LEN);
	parser.GetValue(L"DB_SERVER_PORT", (int*)&_DBServerPort);
	parser.GetValue(L"DB_USER_NAME", _DBUserName, DB_CONFIG_LEN);
	parser.GetValue(L"DB_PASSWORD", _DBPassword, DB_CONFIG_LEN);
	parser.GetValue(L"DB_SCHEMA_NAME", _DBSchemaName, DB_CONFIG_LEN);

	return true;
}

void MonitoringLanServer::ConvertWcharToChar(const WCHAR* wideStr, char* charStr, size_t charStrSize)
{
	int bytesWritten = WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, charStr, static_cast<int>(charStrSize), nullptr, nullptr);

	if (bytesWritten == 0)
	{
		wprintf(L"Conversion failed with error: %d\n", GetLastError());
	}
}

void MonitoringLanServer::ConvertCharToWchar(char* charStr, WCHAR* wideStr, size_t wideStrSize)
{
	int bytesWritten = MultiByteToWideChar(CP_UTF8, 0, charStr, -1, wideStr, static_cast<int>(wideStrSize));

	if (bytesWritten == 0)
	{
		wprintf(L"Conversion failed with error: %d\n", GetLastError());
	}
}

void MonitoringLanServer::InsertDBQuery(const std::string& tableName,
	int serverID, int type, int avg, int min, int max)
{
	// DB에 insert 쿼리 쏘기
	std::string query = "INSERT INTO `" + tableName + "` VALUES(NULL, NOW(), "
		+ std::to_string(serverID) + ", " + std::to_string(type) + ", " 
		+ std::to_string(avg) + ", " + std::to_string(min) + ", " + std::to_string(max) + ");";
	int result = mysql_query(_pDBConnection->GetConnection(), query.c_str());

	// 테이블 없을시 생성 후 입력
	if (result && mysql_errno(_pDBConnection->GetConnection()) == 1146)
	{
		std::string createQuery = "CREATE TABLE " + tableName + " LIKE monitorlog_template;";
		mysql_query(_pDBConnection->GetConnection(), createQuery.c_str());
		mysql_query(_pDBConnection->GetConnection(), query.c_str());
	}
}
