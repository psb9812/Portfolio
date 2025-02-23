#pragma once
#include "LanServer.h"
#include <unordered_map>
//SQL
#include "C:\\Program Files\\MySQL\\MySQL Server 8.0\\include\\mysql.h"
#include "C:\\Program Files\\MySQL\\MySQL Server 8.0\\include\\errmsg.h"

#pragma comment(lib, "C:\\Program Files\\MySQL\\MySQL Server 8.0\\lib\\mysqlclient.lib")
////////////////////////////////////////
//# MonitoringLanServer
// ������� �� ����ϴ� ����͸� �� ����
// 
//# �ϴ� �� : 
// 1. �������� �α��� ��Ŷ�� ����͸� ������ ����
// 2. �� �����͵��� �ִ�/���/�ּ� �� ���ϱ�
// 3. 5�п� �� ���� DB�� �����ϱ�
// 4. MonitoringNetServer���� ���� ������ �����ϱ�
// 
////////////////////////////////////////

#define DB_CONFIG_LEN 32

class MonitoringNetServer;
class DBConnection;

class MonitoringLanServer :
    public LanServer
{
	friend MonitoringNetServer;
public:
	MonitoringLanServer(MonitoringNetServer* pNetServer);
	virtual ~MonitoringLanServer();

	bool Start(const WCHAR* configFileName) override;
	void Stop() override;
private:

	bool OnConnectionRequest(const WCHAR* connectIP, INT32 connectPort) override;
	void OnAccept(INT64 sessionID) override;
	void OnRelease(INT64 sessionID) override;
	void OnRecv(INT64 sessionID, Message* pMessage) override;
	void OnSend(INT64 sessionID, INT32 sendSize) override;
	void OnTime() override;
	void OnError(INT32 errorCode, WCHAR* comment) override;

	IUser* GetUser(INT64 sessionID) override;

	void MakeMessage_MonitorToolDataUpdate(Message* pMessage, BYTE serverID, BYTE dataType, int dataValue, int timeStamp);
	void HandleMonitorLoginMessage(INT64 sessionID, Message* pMessage);
	void HandleMonitorDataUpdateMessage(INT64 sessionID, Message* pMessage);

	void SaveData(int serverID, int dataType, int dataValue);

	bool LoadLoginServerConfig();

	void ConvertWcharToChar(const WCHAR* wideStr, char* charStr, size_t charStrSize);
	void ConvertCharToWchar(char* charStr, WCHAR* wideStr, size_t wideStrSize);
	void InsertDBQuery(const std::string& tableName,
		int serverID, int type, int avg, int min, int max);

private:
	static constexpr int DB_SAVE_INTERVAL = 5 * 60;		//5�п� �� �� DB�� ����
	int tickCount = 0;

	static constexpr int SERVER_NUM = 3;
	static constexpr int LOGIN_DATA_ARR_START = 1;
	static constexpr int LOGIN_DATA_ARR_END = 6;
	static constexpr int GAME_DATA_ARR_START = 10;
	static constexpr int GAME_DATA_ARR_END = 23;
	static constexpr int CHAT_DATA_ARR_START = 30;
	static constexpr int CHAT_DATA_ARR_END = 37;
	static constexpr int COMMON_DATA_ARR_START = 40;
	static constexpr int COMMON_DATA_ARR_END = 44;

	std::unordered_map<INT64, int> _servers;

	MonitoringNetServer* _pNetServer = nullptr;
	SRWLOCK _lock;

	struct DataStatistics
	{
		int _sum;
		int _count;
		int _max;
		int _min;

		void clear()
		{
			_sum = 0;
			_count = 0;
			_max = 0;
			_min = 0;
		}
	};

	bool _isConnectServers[4] = { 0, };

	//�α��� ������ �迭 (1~6)
	DataStatistics _loginServerDatas[LOGIN_DATA_ARR_END];
	//���� ������ �迭(10 ~ 23)
	DataStatistics _gameServerDatas[GAME_DATA_ARR_END - GAME_DATA_ARR_START + 1];
	//ä�� ������ �迭(30 ~ 37)
	DataStatistics _chatServerDatas[CHAT_DATA_ARR_END - CHAT_DATA_ARR_START + 1];
	//���� ������ ������ �迭 (40 ~ 44)
	DataStatistics _commonServerDatas[SERVER_NUM + 1][COMMON_DATA_ARR_END - COMMON_DATA_ARR_START + 1];

	//DB
	inline static thread_local DBConnection* _pDBConnection = nullptr;
	std::vector<DBConnection*> _DBConnections;

	WCHAR	_DBServerIP[IP_LEN] = { 0, };
	USHORT	_DBServerPort;
	WCHAR	_DBUserName[DB_CONFIG_LEN] = { 0, };
	WCHAR	_DBPassword[DB_CONFIG_LEN] = { 0, };
	WCHAR	_DBSchemaName[DB_CONFIG_LEN] = { 0, };

};

