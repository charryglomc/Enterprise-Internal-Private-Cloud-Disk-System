#ifndef __FUNC_H__
#define __FUNC_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <signal.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include<sys/sendfile.h>
//输入密码，显示 * 要用的头文件
#include<termios.h>
#include<assert.h>

#define ARGS_CHECK(argc, num) \
        {if(argc != num) {fprintf(stderr, "ARGS ERROR!\n"); return -1;}}

#define ERROR_CHECK(ret, num, msg) \
        {if(ret == num) {perror(msg); return -1;}}


#define THREAD_ERROR_CHECK(ret, funcName) \
    do{ \
        if(ret != 0) \
        { \
            printf("%s:%s\n", funcName, strerror(ret)); \
        } \
    }while(0) 

#endif

