#include "Head.h"

int DBConn(PGconn **conn, char *ConnStr, FILE *fp) 
{
	*conn = PQconnectdb(ConnStr);
	//-------------------------------------------- Check --------------------------------------------
	if (PQstatus(*conn) != CONNECTION_OK) {
		fprintf(fp, "Connection failed: %s", PQerrorMessage(*conn));
		return FAIL;
	}
	//-----------------------------------------------------------------------------------------------
	return SUCCESS;
}

int DBExec(PGconn *conn, PGresult **res, char *sql, FILE *fp)
{
	*res = PQexec(conn, sql);
	//-------------------------------------------- Check --------------------------------------------
	if (PQresultStatus(*res) != PGRES_COMMAND_OK && PQresultStatus(*res) != PGRES_TUPLES_OK)
	{
		fprintf(fp, "BEGIN command failed: %s", PQerrorMessage(conn));
		return FAIL;
	}
	//-----------------------------------------------------------------------------------------------
	return SUCCESS;
}