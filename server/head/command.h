#ifndef __COMMAND_H__
#define __COMMAND_H__
#include "func.h"

//获取path路径目录下的所有文件/文件夹的名称，以空格间隔存入buf里
int getls(MYSQL* conn,int user_id,char* path,char* buf);

//cp 强


//tree 返回直接打印的内容存在buf里
int tree(MYSQL* conn,int user_id,char* path,char* buf);
//buf接收要打印的tree,width用于设置宽度
int treePrint(MYSQL* conn,int user_id,int id,char* buf,int width);


//在给定的path路径下创建目录（目录名为path中最后一个字符串）
int makedir(MYSQL* conn,int user_id,char* path);

//给出path路径,然后删除该路径下的文件/文件夹（文件/文件夹 名为path中最后一个字符串）
int rm(MYSQL* conn,int user_id,char* path);
//删除文件夹处理函数，id为文件夹id
int rmDirFunc(MYSQL* conn,int user_id,int id);
//删除文件处理函数，id为文件id
int rmFileFunc(MYSQL* conn,int user_id,int id);

//重命名/剪切移动  src:源路径，dest:目的路径
int mv(MYSQL* conn, int user_id, char* src, char* dest);

//获取当前工作目录，返回存到buf里
int getpwd(MYSQL* conn,int user_id, char* buf);

//cd命令的实现，targetDir为要改变到的工作目录
int changeDir(MYSQL* conn, int user_id, char* targetDir);

//exit





/*以下接口是上面是这些命令接口中需要调用*/

//给出当前id,给出一个子目录名，获取该子目录名的id
int getNextFileId(MYSQL* conn, int user_id, const int cur_fileId, char* next);

//给一个路径，获取路径的id
int getFileId(MYSQL* conn, int user_id, char* path);

//给一个文件/文件夹id,获取类型
int getFileTypeFromId(MYSQL* conn,int file_id);

//获取绝对路径，用buf返回
int getPath(MYSQL* conn, int user_id,const char* str, char* buf);

//用于分割路径path,得到的每个名称，存入二维数组save_ptr里，和总长度*length
int myStrTok(char* path,char save_ptr[][20],int *length);

//根据目录id获取该目录下所有文件/文件夹id，并存入temp_id数组里
int getls_id(MYSQL* conn,int user_id,int id,int temp_id[]);

//根据id获取file_name
int getFileName(MYSQL* conn,int id,char* file_name);



//夏坚强的接口函数（私用）
int digui(MYSQL *conn, char *src, int srcid, int destid, int user_id,int flag,char *newdirname);
int getmd5fromid(MYSQL *conn, char *query, int qlen, int id, char *file_md5);
int gettypefromid(MYSQL *conn, char *query, int qlen, int id, char *type);
int getnamefromid(MYSQL *conn, char *query, int qlen, int id, char *file_name);
int getsizefromid(MYSQL *conn, char *query, int qlen, int id, char *file_size);
int addcountfrommad5(MYSQL *conn, char *query, int qlen, char *file_md5);
int delcountfrommd5(MYSQL *conn, char *query, int qlen, char *file_md5);
int detectsame(MYSQL *conn, char *query, int qlen, char *srcname, int dirid);
int cp(MYSQL *conn, int user_id, char *src, char *dest);


#endif