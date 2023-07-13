#ifndef __CLIENT_H__
#define __CLIENT_H__

#include "func.h"
#define COMMAND_ERROR -99
#define ARGS_NUM_ERROR -98
#define ARGS_CON_ERROR -97
#define NO_ERROR 99
#define RUN_ERROR -96
#define FUNC_ERROR -95
#define NO_SUCH_FILE -2
#define CONNECTION_BREAK -94

#define IS_RIGHT(ret) \
        {if(FUNC_ERROR == ret || NO_SUCH_FILE == ret || RUN_ERROR == ret || COMMAND_ERROR == ret || ARGS_NUM_ERROR == ret || ARGS_CON_ERROR == ret) \
            { \
                continue; \
            }}       

//实现用户登录与注册所需的结构体以及函数声明

typedef struct account_s //保存账户名，密码
{
    int flag; //0是注册，1是登录
    char a_name[32];
    char a_passwd[32];
} account_t, *paccount_t;

char *crypt(const char *key, const char *salt);     //根据salt加密密码

int UserLogin(int sfd); //用户登录或注册

void ShowManu(); //显示菜单

int UseChoose(paccount_t paccount); //用户选择 功能，并输入用户名密码

void GetAccountPasswd(paccount_t paccount); //用户登录函数，保存用户输入的用户名和密码

void HidePasswd(char pb[]); //实现隐藏密码的功能，将输入的密码显示为 *




//接收用户 输入的命令 并发送给服务器 所需的 结构体 和 函数

typedef struct command_s //保存命令和参数的结构体
{
    int c_argsnum;      //该命令的参数的个数
    char c_content[20]; //命令内容
    char c_args1[100];   //参数1
    char c_args2[100];   //参数2
    char c_args3[36];   //参数3
} command_t, *pcommand_t;

typedef struct result_S
{
    int len;
    char buf[4096];
}result_t;


// int Getbuf(char *buf);
int GetCommand(pcommand_t pcommand, char *buf); //将命令和参数赋值给command结构体变量

int TransToWord(char *str, char **word); //把指令字符串按照空格分割为单词存储

int SendCommand(pcommand_t pcommand, int sfd); //其他命令

int DownloadCommand(pcommand_t pcommand, int sfd); //下载命令 执行的 函数

int UploadCommand(pcommand_t pcommand, int sfd); //上传命令 执行的 函数

int RepasswdCommand(pcommand_t pcommand, int sfd);  //修改密码


#endif
