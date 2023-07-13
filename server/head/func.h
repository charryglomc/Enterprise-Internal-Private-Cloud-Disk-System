#ifndef __FUNC_H__
#define __FUNC_H__

#include "head.h"
#include "define.h"
#include "struct.h"
#include "database.h"
#include "command.h"
#include "fileTrans.h"

socket_fd TcpInit(char *ip, int port);

void GetRandomStr(char *str);
void GetTimeStamp(char *str, int flag);

int UserFunc(pTask_node_t pTask);
int EpollAddFd(int epoll_fd, int fd);
int TaskQueueInit(pTask_queue_t pQueue);
int ThreadPoolCreate(pThread_pool_t pPool);
int WriteLog(log_fd fd, char *ip, char *buf);
int CmdAnalyse(pTask_node_t pTask, char *timestamp);
int RePassWd(MYSQL *conn, int user_id, client_fd fd);
int LoginOrSignin(pAccount_t pAcc, pTask_node_t pTask);
int ThreadPoolInit(pThread_pool_t pPool, int thread_num);
int FindCmdError(pCommand_t pCmd, int args, client_fd fd);
int InsertTaskQueue(pTask_queue_t pQueue, pTask_node_t Node);
int GetTaskNode(pTask_queue_t pQueue, pTask_node_t *pGetNode);



#endif


