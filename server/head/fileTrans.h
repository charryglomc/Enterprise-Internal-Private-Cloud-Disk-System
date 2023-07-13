#ifndef __FILETRANS_H__
#define __FILETRANS_H__

#include "database.h"
#include "head.h"

#define DOWNLOAD_NOT_EXIST_FILE -2
#define CLIENT_DISCONNECTION -3
#define UPLOAD_EXISTING_FILE -4

int split_path_name(char *path,int user_id,MYSQL *conn,char *filename);
int user_download(MYSQL *db_connect, int clientfd, int user_id, char *filePath, off_t file_offset);
int user_upload(MYSQL *db_connect, int clientfd, int father_id, const char *filename, const char *md5, off_t filesize, int user_id);
int get_file_size(const char *fileName, off_t fileOffset, int *clientFd, struct stat *fileStat);

#endif