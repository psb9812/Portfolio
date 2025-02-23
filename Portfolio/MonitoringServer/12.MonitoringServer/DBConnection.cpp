#include "DBConnection.h"
#include <iostream>


DBConnection::DBConnection(const char* ip, const char* userName, const char* password, const char* schemaName, int port)
{
	mysql_init(&_conn);

	_pConnection = mysql_real_connect(&_conn, ip, userName, password, schemaName, port, (char*)NULL, 0);
	if (_pConnection == NULL)
	{
		fprintf(stderr, "Mysql connection error: %s", mysql_error(&_conn));
		return;
	}
}

DBConnection::~DBConnection()
{
	mysql_close(_pConnection);
}
