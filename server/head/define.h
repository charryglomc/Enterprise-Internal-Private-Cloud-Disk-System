#ifndef __DEFINE_H__
#define __DEFINE_H__

//检查运行参数数量
#define ARGS_CHECK(argc, num, flag) \
    do{\
        if(argc != num){\
            printf("ARGS ERROR!\n");\
            flag = ARGS_NUM_ERROR;\
        }\
    }while(0)

//检查错误信息
#define ERROR_CHECK(ret, num, err_msg) \
    do{\
        if(ret == num){\
            perror(err_msg);\
            return -1;\
        }\
    }while(0)

//检查cmd错误信息
#define CMD_ERROR_CHECK(ret, num) {\
    if(ret != num){\
        continue;\
    }\
}

//检查线程错误信息
#define THREAD_ERROR_CHECK(ret, name)\
    do {\
        if(ret != 0){\
            printf("%s : %s\n", name, strerror(ret));\
        }\
    }while(0)

//线程池的数量
#define PTHREAD_NUM 10

//退出程序
#define EXIT_FLAG -100

//错误宏
#define ERROR_CMD -99
#define ARGS_NUM_ERROR -98
#define ARGS_CON_ERROR -97
#define RUN_ERROR -96

//成功宏
#define NO_ERROR 99

//重新登录
#define RELOGIN -10

//salt值字符串长度
#define STR_LEN 10

//注册操作
#define SIGNIN 0

//登录操作
#define LOGIN 1

#endif