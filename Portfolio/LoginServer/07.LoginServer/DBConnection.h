#pragma once
#include "pch.h"

class DBConnection
{
private:
	MYSQL	_conn;
	MYSQL*  _pConnection = nullptr;
	SRWLOCK _lock;

public:
	DBConnection(const char* ip, const char* userName, const char* password, const char* schemaName, int port);
	~DBConnection();

	inline MYSQL* GetConnection() { return _pConnection; }
};