#include "../head/func.h"

void sig_func(int signum){
    if(28 == signum || SIGPIPE == signum){
        printf("\n%d is coming!\n", signum);
    }
    else{
        printf("\n%d is coming!\n", signum);
        exit(0);
    }
}

/**
 * 用户功能
*/
int UserFunc(pTask_node_t pTask){

    //信号捕捉函数
    for(int i = 1; i < 64; ++i){
        signal(i, sig_func);
    }

    //创建数据库连接
    database_t db;
    memset(&db, 0, sizeof(database_t));
    FILE *fp = fopen("./config/db.ini", "r");
    fscanf(fp, "db_ip = %s db_user = %s db_passwd = %s database_name = %s", db.db_ip, db.db_user, db.db_passwd, db.db_name);
    fclose(fp);
    MYSQL *conn = database_connect(db.db_ip, db.db_user, db.db_passwd, db.db_name);
    if((MYSQL *)-1 == conn){
        return 0;
    }
    pTask->user_conn = conn;

    int ret = 0;
    char timestamp[20] = { 0 };
    char log_name[30] = { 0 };
    char log_content[300] = { 0 };
    
    //根据时间创建新的log文件
    GetTimeStamp(timestamp, 0);
    sprintf(log_name, "./log/%s.log", timestamp);
    pTask->user_lfd = open(log_name, O_WRONLY | O_CREAT | O_APPEND, 0666);

    WriteLog(pTask->user_lfd, pTask->user_ip, "begin the connection.");

    //用栈空间创建用户信息结构体
    account_t acc;

    while(1){

        memset(&acc, 0, sizeof(account_t));

        WriteLog(pTask->user_lfd, pTask->user_ip, "waiting for choice.");

        //1.接收用户发送过来的用户名，用户密码，功能选择
        ret = recv(pTask->user_cfd, &acc, sizeof(account_t), 0);

        //客户端关闭连接
        if(0 == ret){
            WriteLog(pTask->user_lfd, pTask->user_ip, "close the connection, task over.");
            break;
        }

        //2.登录或者注册
        ret = LoginOrSignin(&acc, pTask);

        //3.客户端关闭，直接退出
        if(EXIT_FLAG == ret){
            WriteLog(pTask->user_lfd, pTask->user_ip, "close the connection, task over.");
            break;
        }
        //4.登录或者注册失败，继续等待接收客户端请求
        else if(RELOGIN == ret){
            continue;
        }
        //4.登录、注册成功
        else{

            //保存用户id
            pTask->user_id = ret;

            //进入程序前，修改pwd
            char query[200] = { 0 };
            sprintf(query, "update user set pwd = '/%s' where id = %d", acc.acc_name, pTask->user_id);
            database_operate(pTask->user_conn, query, NULL);

            //进入命令交互界面，REPASSWD重新登录
            ret = CmdAnalyse(pTask, timestamp);
            if(RELOGIN == ret){
                WriteLog(pTask->user_lfd, pTask->user_ip, "relogin.");
                continue;
            }
            WriteLog(pTask->user_lfd, pTask->user_ip, "UserFunc Over.");
            break;

        }

    }

    //关闭文件描述符和数据库连接
    close(pTask->user_cfd);
    close(pTask->user_lfd);
    database_close(conn);

    return 0;
}

/*
 * 用户登录或者注册函数
 * 返回值：成功返回用户id
 * 参数1：从客户端接收到的登录信息
 * 参数2：任务节点
 */
int LoginOrSignin(pAccount_t pAcc, pTask_node_t pTask){
    
    int ret = 0;
    char query[300] = { 0 };
    char **res = NULL;
    char new_password[20] = { 0 };
    char salt[STR_LEN + 1] = { 0 };

    //查询数据库中是否存在该用户名
    memset(query, 0, sizeof(query));
    sprintf(query, "select id from user where username = '%s'", pAcc->acc_name);
    ret = database_operate(pTask->user_conn, query, NULL);
    
    if(SIGNIN == pAcc->opt_flag){

        WriteLog(pTask->user_lfd, pTask->user_ip, "begin login.");

        //注册操作没查询到用户名
        if(0 == ret){

            WriteLog(pTask->user_lfd, pTask->user_ip, "username is ok.");
            //生成salt值
            GetRandomStr(salt);

            //数据库中添加用户名和salt值和当前目录
            strcpy(pAcc->acc_passwd, salt);

            //发送盐值
            send(pTask->user_cfd, &ret, 4, 0);
            send(pTask->user_cfd, salt, STR_LEN + 1, 0);

            //接收密文
            ret = recv(pTask->user_cfd, new_password, 13, 0);

            //对端关闭连接，退出UserFunc
            if(0 == ret){
                WriteLog(pTask->user_lfd, pTask->user_ip, "close the connection, task over.");
                return EXIT_FLAG;
            }
            
            //插入新用户
            memset(query, 0, sizeof(query));
            sprintf(query, "insert into user ( username , salt , password, pwd ) values ( '%s' , '%s' , '%s' , '/%s' )", pAcc->acc_name, salt, new_password, pAcc->acc_name);
            database_operate(pTask->user_conn, query, NULL);

            //查询id
            memset(query, 0, sizeof(query));
            sprintf(query, "select id from user where username = '%s' and password = '%s'", pAcc->acc_name, new_password);
            database_operate(pTask->user_conn, query, &res);

            //在虚拟文件表创建用户目录
            memset(query, 0, sizeof(query));
            sprintf(query, "insert into file ( father_id, file_name , type , user_id , file_md5 , file_size ) values ( 0 , '%s' , 'd' , %d , 0 , 0)", pAcc->acc_name, atoi(res[0]));
            database_operate(pTask->user_conn, query, NULL);

            WriteLog(pTask->user_lfd, pTask->user_ip, "signin success.");

            //查到返回用户id

            ret = atoi(res[0]);
            send(pTask->user_cfd, &ret, 4, 0);

            return ret;
            
        }

        //用户名已经存在，返回-1，重新注册
        else{

            WriteLog(pTask->user_lfd, pTask->user_ip, "signin error, user exist.");
            ret = -1;
            send(pTask->user_cfd, &ret, 4, 0);
            return RELOGIN;
        }
    }
    else if(LOGIN == pAcc->opt_flag){

        //用户名存在
        if(0 != ret){

            WriteLog(pTask->user_lfd, pTask->user_ip, "begin login.");
            //查询salt
            memset(query, 0, sizeof(query));
            sprintf(query, "select salt from user where username = '%s'", pAcc->acc_name);
            database_operate(pTask->user_conn, query, &res);
            strcpy(salt, res[0]);

            //发送盐值
            send(pTask->user_cfd, &ret, 4, 0);
            send(pTask->user_cfd, salt, STR_LEN + 1, 0);

            //接收密文
            ret = recv(pTask->user_cfd, new_password, 13, 0);

            //对端关闭连接，退出UserFunc
            if(0 == ret){
                return EXIT_FLAG;
            }

            //根据用户名，密码查询id
            memset(query, 0, sizeof(query));
            sprintf(query, "select id from user where username = '%s' and password = '%s'", pAcc->acc_name, new_password);
            ret = database_operate(pTask->user_conn, query, &res);

            //查不到用户，返回-1，重新登录
            if(0 == ret){
                WriteLog(pTask->user_lfd, pTask->user_ip, "password error.");
                ret = -1;
                send(pTask->user_cfd, &ret, 4, 0);
                return RELOGIN;
            }

            //查到返回用户id
            else{
                WriteLog(pTask->user_lfd, pTask->user_ip, "login success.");
                ret = atoi(res[0]);
                send(pTask->user_cfd, &ret, 4, 0);
                return ret;
            }

        }

        //用户名不存在，重新登录
        else{
            WriteLog(pTask->user_lfd, pTask->user_ip, "username eerror.");
            ret = -1;
            send(pTask->user_cfd, &ret, 4, 0);
            return RELOGIN;
        }
        
    }
    
}

/**
 * 参数1：任务节点
 * 参数2：时间戳字符串
*/
int CmdAnalyse(pTask_node_t pTask, char *timestamp){

    // printf("begin CmdAnalyse\n");

    int ret;
    train_t tf;
    command_t cmd;
    char log_content[300] = { 0 };
    char query[300] = { 0 };

    //发送pwd
    int father_id = getpwd(pTask->user_conn, pTask->user_id, tf.data_buf);
    tf.data_len = strlen(tf.data_buf);
    send(pTask->user_cfd, &tf.data_len, 4, MSG_WAITALL);
    send(pTask->user_cfd, tf.data_buf, tf.data_len, 0);
    
    while(1){

        memset(&tf, 0, sizeof(train_t));
        memset(&cmd, 0, sizeof(command_t));
        memset(log_content, 0, sizeof(log_content));

        ret = recv(pTask->user_cfd, &cmd, sizeof(command_t), 0);

        //客户端关闭连接
        if(0 == ret){
            memset(log_content, 0, sizeof(log_content));
            sprintf(log_content, "user_id = %d close the connection, task over.", pTask->user_id);
            WriteLog(pTask->user_lfd, pTask->user_ip, log_content);
            return EXIT_FLAG;
        }

        sprintf(log_content, "user_id = %d send cmd, args = %d, cmd = %s %s %s %s", pTask->user_id, cmd.cmd_args, cmd.cmd_content, cmd.cmd_arg1, cmd.cmd_arg2, cmd.cmd_arg3);
        WriteLog(pTask->user_lfd, pTask->user_ip, log_content);

        int success_flag = NO_ERROR;

        //接收命令
        if(0 == strcmp(cmd.cmd_content, "cp")){
            success_flag = FindCmdError(&cmd, 2, pTask->user_cfd);
            CMD_ERROR_CHECK(success_flag, NO_ERROR);
            ret = cp(pTask->user_conn, pTask->user_id, cmd.cmd_arg1, cmd.cmd_arg2);
            if(0 == ret){
                success_flag = NO_ERROR;
                WriteLog(pTask->user_lfd, pTask->user_ip, "cp success");
            }
            else{
                success_flag = RUN_ERROR;
            }
            send(pTask->user_cfd, &success_flag, 4, 0);
            
            continue;
        }
        else if(0 == strcmp(cmd.cmd_content, "mkdir")){
            success_flag = FindCmdError(&cmd, 1, pTask->user_cfd);
            CMD_ERROR_CHECK(success_flag, NO_ERROR);
            ret = makedir(pTask->user_conn, pTask->user_id, cmd.cmd_arg1);
            if(0 == ret){
                success_flag = NO_ERROR;
                WriteLog(pTask->user_lfd, pTask->user_ip, "mkdir success");
            }
            else{
                success_flag = RUN_ERROR;
            }
            send(pTask->user_cfd, &success_flag, 4, 0);
            continue;
        }
        else if(0 == strcmp(cmd.cmd_content, "rm")){
            success_flag = FindCmdError(&cmd, 1, pTask->user_cfd);
            CMD_ERROR_CHECK(success_flag, NO_ERROR);
            ret = rm(pTask->user_conn, pTask->user_id, cmd.cmd_arg1);
            if(0 == ret){
                success_flag = NO_ERROR;
                WriteLog(pTask->user_lfd, pTask->user_ip, "rm success");
            }
            else{
                success_flag = RUN_ERROR;
            }
            send(pTask->user_cfd, &success_flag, 4, 0);
            continue;
        }
        else if(0 == strcmp(cmd.cmd_content, "mv")){
            success_flag = FindCmdError(&cmd, 2, pTask->user_cfd);
            CMD_ERROR_CHECK(success_flag, NO_ERROR);
            ret = mv(pTask->user_conn, pTask->user_id, cmd.cmd_arg1, cmd.cmd_arg2);
            if(0 == ret){
                success_flag = NO_ERROR;
                WriteLog(pTask->user_lfd, pTask->user_ip, "mv success");
            }
            else{
                success_flag = RUN_ERROR;
            }
            send(pTask->user_cfd, &success_flag, 4, 0);
            continue;
        }
        else if(0 == strcmp(cmd.cmd_content, "download")){
            success_flag = FindCmdError(&cmd, 2, pTask->user_cfd);
            CMD_ERROR_CHECK(success_flag, NO_ERROR);
            ret = user_download(pTask->user_conn, pTask->user_cfd, pTask->user_id, cmd.cmd_arg1, atoi(cmd.cmd_arg2));
            if(CLIENT_DISCONNECTION == ret){
                return CLIENT_DISCONNECTION;
            }
            else if(0 != ret){
                WriteLog(pTask->user_lfd, pTask->user_ip, "user download failure.");
                continue;
            }
            WriteLog(pTask->user_lfd, pTask->user_ip, "user download success.");
            ret = recv(pTask->user_cfd, &ret, 4, MSG_WAITALL);
            if(0 == ret){
                return EXIT_FLAG;
            }
            continue;
        }
        else if(0 == strcmp(cmd.cmd_content, "upload")){
            success_flag = FindCmdError(&cmd, 3, pTask->user_cfd);
            CMD_ERROR_CHECK(success_flag, NO_ERROR);
            ret = user_upload(pTask->user_conn, pTask->user_cfd, father_id, cmd.cmd_arg1, cmd.cmd_arg3, atoi(cmd.cmd_arg2), pTask->user_id);
            if(CLIENT_DISCONNECTION == ret){
                return CLIENT_DISCONNECTION;
            }
            else if(0 != ret){
                WriteLog(pTask->user_lfd, pTask->user_ip, "user upload failure.");
                continue;
            }
            else if(0 == ret){
                success_flag = NO_ERROR;
            }
            send(pTask->user_cfd, &success_flag, 4, 0);
            if(1 == ret){
                WriteLog(pTask->user_lfd, pTask->user_ip, "user upload success.");
            }
            continue;
        }
        else if(0 == strcmp(cmd.cmd_content, "exit") || 0 == strcmp(cmd.cmd_content, "quit") || 0 == strcmp(cmd.cmd_content, "q")){
            send(pTask->user_cfd, &success_flag, 4, 0);
            return EXIT_FLAG;
        }
        else if(0 == strcmp(cmd.cmd_content, "tree")){
            send(pTask->user_cfd, &success_flag, 4, 0);
            ret = tree(pTask->user_conn, pTask->user_id, cmd.cmd_arg1, tf.data_buf);
            if(0 == ret){
                success_flag = NO_ERROR;
            }
            else{
                success_flag = RUN_ERROR;
            }
            send(pTask->user_cfd, &success_flag, 4, 0);
        }
        else if(0 == strcmp(cmd.cmd_content, "pwd")){
            send(pTask->user_cfd, &success_flag, 4, 0);
            ret = getpwd(pTask->user_conn, pTask->user_id, tf.data_buf);
            if(-1 != ret){
                father_id = ret;
                success_flag = NO_ERROR;
            }
            else{
                success_flag = RUN_ERROR;
            }
            send(pTask->user_cfd, &success_flag, 4, 0);
        }
        else if(0 == strcmp(cmd.cmd_content, "cd")){
            success_flag = FindCmdError(&cmd, 1, pTask->user_cfd);
            CMD_ERROR_CHECK(success_flag, NO_ERROR);
            ret = changeDir(pTask->user_conn, pTask->user_id, cmd.cmd_arg1);
            if(-1 == ret){
                success_flag = RUN_ERROR;
                send(pTask->user_cfd, &success_flag, 4, 0);
                continue;
            }
            else{
                send(pTask->user_cfd, &success_flag, 4, 0);
            }
            ret = getpwd(pTask->user_conn, pTask->user_id, tf.data_buf);
            if(-1 != ret){
                father_id = ret;
            }
        }
        else if(0 == strcmp(cmd.cmd_content, "ls")){
            send(pTask->user_cfd, &success_flag, 4, 0);
            ret = getls(pTask->user_conn, pTask->user_id, cmd.cmd_arg1, tf.data_buf);
            if(0 == ret){
                success_flag = NO_ERROR;
            }
            else{
                success_flag = RUN_ERROR;
            }
            send(pTask->user_cfd, &success_flag, 4, 0);
        }   
        else if(0 == strcmp(cmd.cmd_content, "rp") || 0 == strcmp(cmd.cmd_content, "repasswd")){
            send(pTask->user_cfd, &success_flag, 4, 0);
            ret = RePassWd(pTask->user_conn, pTask->user_id, pTask->user_cfd);
            if(RELOGIN == ret){
                WriteLog(pTask->user_lfd, pTask->user_ip, "repasswd success.");
                return RELOGIN;
            }
            else if(EXIT_FLAG == ret){
                WriteLog(pTask->user_lfd, pTask->user_ip, "close the connection, task over.");
                return EXIT_FLAG;
            }
        }
        //错误命令
        else{
            success_flag = ERROR_CMD;
            send(pTask->user_cfd, &success_flag, 4, 0);
            WriteLog(pTask->user_lfd, pTask->user_ip, "error cmd.");
            continue;
        }

        //检测是否执行成功
        CMD_ERROR_CHECK(success_flag, NO_ERROR);

        //打印要发送的东西
        // printf("++++++++++++++++++++++++++++++++\nsend:\n%s\n++++++++++++++++++++++++++++++++\n", tf.data_buf);

        //发送要发送的东西
        tf.data_len = strlen(tf.data_buf);
        send(pTask->user_cfd, &tf.data_len, 4, 0);
        send(pTask->user_cfd, tf.data_buf, tf.data_len, 0);
    }

    return 0;
}

/**
 * 修改密码函数
 * 参数1：数据库连接
 * 参数2：用户id
 * 参数3：与客户端通信的文件描述符
*/
int RePassWd(MYSQL *conn, int user_id, client_fd fd){
    
    int ret = 0;
    char salt[STR_LEN + 1] = { 0 };
    char new_password[20] = { 0 };
    char query[200] = { 0 };
    
    //生成新salt值
    GetRandomStr(salt);
    
    //发送新salt值
    send(fd, salt, STR_LEN + 1, 0);

    //接收密文
    ret = recv(fd, new_password, 13, 0);

    //对端关闭连接，退出UserFunc
    if(0 == ret){
        return EXIT_FLAG;
    }

    //更新表
    sprintf(query, "update user set salt = '%s' , password = '%s' where id = %d", salt, new_password, user_id);

    ret = database_operate(conn, query, NULL);
    if(0 == ret){
        ret = RUN_ERROR;
    }
    else{
        ret = NO_ERROR;
    }
    send(fd, &ret, 4, 0);

    return RELOGIN;
    
}

/**
 * 找命令错误函数
 * 参数1：命令
 * 参数2：应该有的参数数量
 * 参数3：与客户端通信的文件描述符
*/
int FindCmdError(pCommand_t pCmd, int args, client_fd fd){
    int success_flag = NO_ERROR;
    ARGS_CHECK(pCmd->cmd_args, args, success_flag);
    int arg_len[3] = { 0, 0, 0 };
    arg_len[0] = strlen(pCmd->cmd_arg1);
    arg_len[1] = strlen(pCmd->cmd_arg2);
    arg_len[2] = strlen(pCmd->cmd_arg3);
    for(int i = 0; i < args; ++i){
        if(0 == arg_len[i]){
            success_flag = ARGS_CON_ERROR;
        }
    }
    send(fd, &success_flag, 4, 0);
    return success_flag;
}