#include "../head/func.h"
#include "../head/cilent.h"
#include "../head/md5.h"
void sigfunc(int signum)
{
    printf("connection break\n");
    exit(1);
}

int main(int argc, char *argv[])
{

    signal(SIGPIPE, sigfunc);

    ARGS_CHECK(argc, 3);

    //与服务器进行连接
    int sfd = socket(AF_INET, SOCK_STREAM, 0);  //创建一个socket标识
    ERROR_CHECK(sfd, -1, "socket");
    struct sockaddr_in sockaddr;                //创建一个结构体存储服务器的IP和端口号
    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = inet_addr(argv[1]);
    sockaddr.sin_port = htons(atoi(argv[2]));
    int ret = connect(sfd, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
    ERROR_CHECK(ret, -1, "connect");               //向服务端发起连接

    //创建必要的变量
    char buf[4096] = {0};       
    char username[4096] = {0};      // 保存当前用户名
    char userdir[8192] = {0};       // 保存当前用户目录
    result_t result;                // 传输服务器在第一次登陆时发给我的 用户目录
    memset(&result, 0, sizeof(result));
    int recvflag = 0;               // 命令执行结束，接收服务器发给我的参数
    int relogin = 1;                // 用于修改密码，重新登陆
    

    //循环接收 用户 输入的命令
    while (1)
    {
        if (relogin == 1)           // 该变量为1，则进入登陆界面，用于首次登陆 和 修改密码的情况
        {
            UserLogin(sfd);
            // fflush(stdout);

            relogin = 0;
            memset(userdir, 0, sizeof(userdir));

            //每次登录，接收一次 用户的当前目录
            ret = recv(sfd, &result.len, 4, 0);        
            ERROR_CHECK(ret, -1, "recv_len");
            ret = recv(sfd, result.buf, result.len, 0);
            ERROR_CHECK(ret, -1, "recv_buf");

            //将用户当前目录拷贝到 userdir 数组里
            strcpy(username, result.buf);
            sprintf(userdir, "%s%s%s", username, ":", "$ ");
            memset(username, 0, sizeof(username));
        }

        puts("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
        printf("%s", userdir);              //打印 用户当前目录
        fflush(stdout);

        //读取用户输入的命令
        memset(buf, 0, sizeof(buf));
        ret = read(STDIN_FILENO, buf, sizeof(buf) - 1);
        ERROR_CHECK(ret, -1, "read");

        //获得用户输入的命令和参数
        command_t command;
        memset(&command, 0, sizeof(command));

        // 如果用户输入的是空格，那么就什么也不做，直接开始下一次的命令输入
        if (GetCommand(&command, buf) == COMMAND_ERROR)
        {           
            continue;
        }

        //分析用户输入的命令的类型，并分情况处理
        if (strcmp(command.c_content, "download") == 0)
        {
            //如果用户输入的是 下载命令
            ret = DownloadCommand(&command, sfd);
            IS_RIGHT(ret);

            //下载完成，给对方发送一个整数
            send(sfd, &ret, 4, 0);
        }
        else if (strcmp(command.c_content, "upload") == 0)
        {
            //如果用户输入的是 上传命令
            ret = UploadCommand(&command, sfd);
            IS_RIGHT(ret);

            //上传完成，接收对方发给我的一个整数
            ret = recv(sfd, &recvflag, 4, MSG_WAITALL);
            if(ret == 0)
            {
                printf("connection break\n");
            }
        }
        else if ((strcmp(command.c_content, "ls") == 0) || (strcmp(command.c_content, "tree") == 0) || (strcmp(command.c_content, "pwd") == 0))
        {
            //执行ls,tree,pwd命令
            ret = SendCommand(&command, sfd);
            IS_RIGHT(ret);

            recv(sfd, &ret, 4, 0);
            if(ret != NO_ERROR)
            {
                printf("%s failed\n", command.c_content);
                continue;
            }
            

            //接收命令返回的字符串
            memset(&result, 0, sizeof(result));
            ret = recv(sfd, &result.len, sizeof(result.len), MSG_WAITALL);
            ERROR_CHECK(ret, -1, "recv_len");
            if (result.len == 0)
            {
                printf("%s", userdir);
                printf("\b \b");
                printf("\b \b");
                printf("\b \b\n");
                continue;
            }
            recv(sfd, result.buf, result.len, MSG_WAITALL);
            ERROR_CHECK(ret, -1, "recv_buf");
            puts(result.buf);
        }
        else if (strcmp(command.c_content, "cd") == 0)
        {
            //执行cd命令
            ret = SendCommand(&command, sfd);
            IS_RIGHT(ret);

            //收到-1，表示没有目录
            recv(sfd, &ret, 4, MSG_WAITALL);
            if (ret == RUN_ERROR)
            {
                printf("no such dir\n");
                continue;
            }
            printf("cd success\n");

            //用result结构体接收，执行正确的情况
            memset(&result, 0, sizeof(result));
            ret = recv(sfd, &result.len, sizeof(result.len), MSG_WAITALL);
            ERROR_CHECK(ret, -1, "recv_len");
            recv(sfd, result.buf, result.len, MSG_WAITALL);
            ERROR_CHECK(ret, -1, "recv_buf");

            //将结果复制到 userdir 变量中
            strcpy(username, result.buf);
            memset(userdir, 0, sizeof(userdir));
            sprintf(userdir, "%s%s%s", username, ":", "$ ");
            memset(username, 0, sizeof(username));
        }
        else if (strcmp(command.c_content, "clear") == 0)
        {
            system("clear");
        }
        else if (strcmp(command.c_content, "exit") == 0 || strcmp(command.c_content, "quit") == 0 || strcmp(command.c_content, "q") == 0)
        {
            //退出命令
            ret = SendCommand(&command, sfd);
            IS_RIGHT(ret);
            printf("bye bye\n");
            break;
        }
        else if (strcmp(command.c_content, "rp") == 0 || strcmp(command.c_content, "repasswd") == 0)
        {
            //重置密码
            ret = RepasswdCommand(&command, sfd);
            IS_RIGHT(ret);

            relogin = 1;
            fflush(stdout);
        }
        else
        {   
            //其他命令
            ret = SendCommand(&command, sfd);
            IS_RIGHT(ret);

            recv(sfd, &recvflag, 4, MSG_WAITALL);
            if (recvflag == NO_ERROR)
            {
                printf("%s success\n", command.c_content);
            }
            else
            {
                printf("%s failed\n", command.c_content);
            }
        }
    }

    return 0;
}
