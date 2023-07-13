#include "../head/func.h"

/**
 * tcp连接初始化函数
 * 参数1：ip地址
 * 参数2：端口号
 * 返回值：成功返回一个socket文件描述符，用于接收客户端连接，失败返回-1
*/
socket_fd TcpInit(char *ip, int port){

    int ret = 0;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    /*分别设置协议、IP地址和端口号*/
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port = htons(port);

    socket_fd sfd = socket(AF_INET, SOCK_STREAM, 0);
    ERROR_CHECK(sfd, -1, "socket");

    //设置地址可重用
    int reuse = 1;
    ret = setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));
    ERROR_CHECK(ret, -1, "setsockopt");
    
    //绑定ip地址和端口号
    ret = bind(sfd, (struct sockaddr*)&addr, sizeof(addr));
    ERROR_CHECK(ret, -1, "bind");

    //监听描述符，最大连接数设置为10
    ret = listen(sfd, 10);
    ERROR_CHECK(ret, -1, "ret");

    return sfd;
}

/**
 * 返回值：成功返回0，失败返回-1
 * 参数1：epoll的文件描述符
 * 参数2：需要监听的文件描述符
*/
int EpollAddFd(int epoll_fd, int fd){

    int ret = 0;
    struct epoll_event events;
    memset(&events, 0, sizeof(events));

    events.events = EPOLLIN;
    events.data.fd = fd;

    ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &events);
    ERROR_CHECK(ret, -1, "epoll_ctl");

    return 0;
}

/**
 * 参数：传入传出参数，获得一个salt值
*/
void GetRandomStr(char *str){
    int flag;
    srand(time(NULL));
    for(int i = 0; i < STR_LEN ; ++i){
        flag = rand()%3;
        switch(flag){
            case 0: 
                str[i] = rand()%26 + 'a';
                break;
            case 1:
                str[i] = rand()%26 + 'A';
                break;
            case 2:
                str[i] = rand()%10 + '0';
                break;
            default:
                str[i] = rand()%4 + '#';
                break;
        }
    }
}

/**
 * 参数1：传入传出参数，获得当前时间戳
 * 参数2：1表示日期时分秒，0表示只有日期
*/
void GetTimeStamp(char *str, int flag){

    time_t t;
    time(&t);
    struct tm *tmp_time = localtime(&t);
    char buf[20] = { 0 };
    if(1 == flag){
        strftime(buf, sizeof(buf), "%04Y%02m%02d %H:%M:%S", tmp_time);
    }
    else{
        strftime(buf, sizeof(buf), "%04Y%02m%02d", tmp_time);
    }
    strcpy(str, buf);
    
}

/**
 * 返回值：成功返回0，失败返回-1
 * 参数1：用于写log文件的文件描述符
 * 参数2：ip
 * 参数3：记录语句
*/
int WriteLog(log_fd fd, char *ip, char *buf){
    
    char timestamp[20] = { 0 };
    char log_content[4096] = { 0 };

    GetTimeStamp(timestamp, 1);
    sprintf(log_content, "[%s %s]%s\n", timestamp, ip, buf);
    int ret = write(fd, log_content, strlen(log_content));
    ERROR_CHECK(ret, -1, "write");
    return 0;

}