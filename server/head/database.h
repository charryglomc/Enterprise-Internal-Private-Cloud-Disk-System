#ifndef __DATABASE_H__
#define __DATABASE_H__

#include <stdio.h>
#include <string.h>
#include <mysql/mysql.h>
MYSQL *database_connect(char *server, char *user, char *password, char *database);
int database_operate(MYSQL *db_connect, char *query, char ***result);
void database_close(MYSQL *db_connect);

#endif