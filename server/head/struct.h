#ifndef __STRUCT_H__
#define __STRUCT_H__

//服务器用于接收tcp连接的文件描述符
typedef int socket_fd;
//服务器用于和客户端通信的文件描述符
typedef int client_fd;
//服务端打开的用于存储log文件的文件描述符
typedef int log_fd;
//服务端打开的用于存储config文件的文件描述符
typedef int config_fd;

//数据传输结构体
typedef struct{
    //数据长度
    int data_len;
    //数据内容
    char data_buf[4096];
}train_t;

//用户登录、注册结构体
typedef struct account_s{
    //LOGIN是登录，SIGNIN是注册
    int opt_flag;
    //用户名
    char acc_name[32];
    //用户密码
    char acc_passwd[32];
}account_t, *pAccount_t;

//命令结构体
typedef struct command_s{
    //命令参数数量
    int cmd_args;
    //命令
    char cmd_content[20];
    //参数1
    char cmd_arg1[100];
    //参数2
    char cmd_arg2[100];
    //参数3
    char cmd_arg3[36];
}command_t, *pCommand_t;

//任务节点：存储一个用户的各种信息
typedef struct task_node_s{
    //与客户端通信的文件描述符
    client_fd user_cfd;
    //客户端log文件的文件描述符
    log_fd user_lfd;
    //客户端用户的id
    int user_id;
    //客户端的地址信息
    char user_ip[15];
    //数据库连接
    MYSQL *user_conn;
    //下一个任务
    struct task_node_s *pNext;
}task_node_t, *pTask_node_t;

//任务队列
typedef struct task_queue_s{
    //退出信号
    short exit_flag;
    //任务队列长度
    int queue_len;
    //线程的条件变量
    pthread_cond_t queue_cond;
    //线程锁
    pthread_mutex_t queue_mutex;
    //任务队列的首尾指针
    pTask_node_t pHead, pTail;
}task_queue_t, *pTask_queue_t;

//定义线程池结构体，一个线程池里有thread_num个线程，共享一个任务队列
typedef struct {
    //线程个数
    int thread_num;
    //线程id数组，动态创建
    pthread_t *pThread_id;
    //任务队列
    task_queue_t task_queue;
}thread_pool_t, *pThread_pool_t;

//数据库连接信息
typedef struct database_s{
    char db_ip[16];
    char db_user[20];
    char db_passwd[20];
    char db_name[20];
}database_t, pDatabase_t;

#endif