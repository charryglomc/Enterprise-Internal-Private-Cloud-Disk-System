#include "database.h"
/**
 * 参数1：MySQL登录密码
 * 参数2：需要操作的数据库名
 * 返回值：MySQL连接
*/

const int KEY_LEN = 64;

MYSQL *database_connect(char *server, char *user, char *password, char *database)
{
	MYSQL *db_connect = NULL;

	//初始化
	db_connect = mysql_init(NULL);
	if (db_connect == NULL)
	{
		printf("MySQL init failed\n");
	}

	//连接数据库，看连接是否成功，只有成功才能进行后面的操作
	if (!mysql_real_connect(db_connect, server, user, password, database, 0, NULL, 0))
	{
		// printf("Error connecting to database : %s\n", mysql_error(db_connect));
		return (MYSQL *)-1;
	}
	else
	{
		printf("MySQL Connected...\n");
	}
	return db_connect;
}

void database_close(MYSQL *db_connect)
{
	mysql_close(db_connect);
}

/*
参数1：database_connect函数返回的 MySQL连接
参数2：MySQL操作的字符串，如：select * from tb_name。不加分号。
参数3：查询得到的字符串数组（传二级指针的地址，传出参数，调用者不用申请空间）
返回值：成功返回查询结果数量，失败返回0
*/
int database_operate(MYSQL *db_connect, char *query, char ***result)
{
	
	MYSQL_RES *res; //该结构代表返回行的查询结果。
	MYSQL_ROW row;

	long ret = 0;
	unsigned int rowNum = 1, i;
	//把SQL语句传递给MySQL
	unsigned int queryRet = mysql_query(db_connect, query);
	// unsigned int queryRet = mysql_real_query(db_connect, query, strlen(query));
	if (queryRet != 0)
	{
		// printf("Error making query : %s\n", mysql_error(db_connect));
		// printf("sssssssss%d\n", queryRet);
		return 0;
	}
	else
	{
		switch (query[0])
		{
		case 's': //select

			res = mysql_store_result(db_connect);

			rowNum = mysql_num_rows(res); //用mysql_num_rows可以得到查询的结果集有几行,要配合mysql_store_result使用
			if (result != NULL)
			{
				*result = (char **)calloc(rowNum, sizeof(char *));
				for (i = 0; i < rowNum; i++)
				{
					(*result)[i] = (char *)calloc(KEY_LEN, sizeof(char));
				}
				row = mysql_fetch_row(res);

				i = 0;
				if (row == NULL)
				{
					printf("Don't find any data.\n");
				}
				else
				{
					do
					{
						// printf("num=%d\n",mysql_num_fields(res));//列数
						//每次循环打印一整行的内容
						for (queryRet = 0; queryRet < mysql_num_fields(res); queryRet++)
						{
							strcat((*result)[i], row[queryRet]);
							if (queryRet < mysql_num_fields(res) - 1)
								strcat((*result)[i], " ");
						}
						//printf("uuuu %s\n",(*result)[i] );
						i++;
					} while ((row = mysql_fetch_row(res)) != NULL);
				}
				mysql_free_result(res);
			}
			break;

		case 'i':
		case 'd':
		case 'u':
			ret = mysql_affected_rows(db_connect);
			if (ret)
			{
				printf("operate success, mysql affected row : %lu\n", ret);
			}
			else
			{
				printf("operate failed, mysql affected rows : %lu\n", ret);
			}
			break;

		default:
			break;
		}
	}
	return rowNum;
}
