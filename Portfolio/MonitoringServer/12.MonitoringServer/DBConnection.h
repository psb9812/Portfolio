#pragma once
#include <Windows.h>
#include "C:\\Program Files\\MySQL\\MySQL Server 8.0\\include\\mysql.h"

class DBConnection
{
private:
	MYSQL	_conn;
	MYSQL* _pConnection = nullptr;
	SRWLOCK _lock;

public:
	DBConnection(const char* ip, const char* userName, const char* password, const char* schemaName, int port);
	~DBConnection();

	inline MYSQL* GetConnection() { return _pConnection; }
};