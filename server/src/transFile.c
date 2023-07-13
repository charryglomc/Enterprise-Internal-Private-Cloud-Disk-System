#include "../head/func.h"

#define BIG_SIZE 104857600 //100M，大文件阈
#define QUERY_LEN 256      //SQL语句字符串长度
#define REAL_NAME_LEN 64   //真实文件名字符串长度
#define BUF_LEN 4096       //传输buf大小
#define NET_DISK "./file/" //真实文件存储位置

int split_path_name(char *path, int user_id, MYSQL *conn, char *filename)
{
    printf("file path =%s\n", path);
    int length = 0;
    int length_path = strlen(path);
    char save_ptr[20][20] = {0};
    char temp_path[64] = {0};
    //path是输入的路径参数
    myStrTok(path, save_ptr, &length);

    strcpy(filename, save_ptr[length - 1]);
    printf("filename =%s\n", filename);
    //复制除了名字外其他的字符串
    if (length != 1)
    {
        strncpy(temp_path, path, length_path - 1 - strlen(save_ptr[length - 1]));
        //printf("%s\n",temp_path);
    }
    else
    {
        strcpy(temp_path, ".");
    }
    char buf[128] = {0};
    getPath(conn, user_id, temp_path, buf);
    //printf("buf:%s\n",buf);

    int id = getFileId(conn, user_id, buf);
    //printf("id:%d\n",id);

    return id;
}

int get_file_size(const char *fileName, off_t fileOffset, int *clientFd, struct stat *fileStat)
{
    //组合真实文件路径
    char real_filename[REAL_NAME_LEN] = NET_DISK;
    sprintf(real_filename, "%s%s", real_filename, fileName);
    printf("filename=%s\n", real_filename);

    //打开真实文件
    int fileFd = open(real_filename, O_RDWR | O_CREAT, 0666);
    ERROR_CHECK(fileFd, -1, "open");

    //获取真实文件大小
    fstat(fileFd, fileStat);

    fileStat->st_size -= fileOffset; //文件大小减去偏移量(即客户端已下载的文件大小)

    //向客户端发送待传输的文件大小
    int ret = send(*clientFd, &fileStat->st_size, sizeof(off_t), 0);
    ERROR_CHECK(ret, -1, "recv file size");
    printf("file size = %ld\n", fileStat->st_size);

    return fileFd;
}

/**用户下载
 * 
 * 返回值：正确返回0。错误返回对应错误数值。
*/
//filePath: dir1/dir2/file
int user_download(MYSQL *db_connect, int clientfd, int user_id, char *filePath, off_t file_offset)
{
    char filename[64] = {0};
    int father_id = split_path_name(filePath, user_id, db_connect, filename);
    printf("filename = %s\n", filename);
    printf("father id = %d \n", father_id);

    //SQL查询语句
    char query[QUERY_LEN] = {0};
    sprintf(query, "select file_md5 from file where father_id = %d and file_name = '%s'",
            father_id, filename);

    //查file表获取真实文件名(即md5)
    char **file_md5 = NULL;
    int ret = database_operate(db_connect, query, &file_md5);
    // printf("ret = %d  filename = %s\n",ret, file_md5[0]);
    if (ret == 0) //虚拟文件表中不存在该文件
    {
        printf("file is not exist\n");
        off_t notExist = DOWNLOAD_NOT_EXIST_FILE;
        send(clientfd, &notExist, sizeof(off_t), 0);
        return DOWNLOAD_NOT_EXIST_FILE;
    }

    //组合真实文件路径，打开\创建真实文件，获取真实文件大小，向客户端发送文件大小
    struct stat fileStat;
    memset(&fileStat, 0, sizeof(struct stat));
    int filefd = get_file_size(file_md5[0], file_offset, &clientfd, &fileStat);

    //向客户端传输文件内容
    char trans_buf[BUF_LEN] = {0};

    off_t sendLength = 0; //记录已传送的数据量

    if (fileStat.st_size < BIG_SIZE) //小文件
    {
        lseek(filefd, file_offset, SEEK_SET); //偏移文件读写指针：断点续传

        while (sendLength < fileStat.st_size)
        {
            memset(trans_buf, 0, BUF_LEN);

            ret = read(filefd, trans_buf, BUF_LEN); //先读文件内容到trans_buf
            ERROR_CHECK(ret, -1, "read real file");

            ret = send(clientfd, trans_buf, ret, MSG_WAITALL); //再发送到客户端
            ERROR_CHECK(ret, -1, "send file to client");
            if (ret == -1)
            {
                return CLIENT_DISCONNECTION;
            }

            sendLength += ret;
        }
    }
    else //大文件，零拷贝技术传输。自带偏移量：可实现断点续传
    {
        sendfile(clientfd, filefd, &file_offset, fileStat.st_size);
    }
    close(filefd);
    return 0;
}

//用户上传
int user_upload(MYSQL *db_connect, int clientfd, int father_id, const char *filename, const char *md5, off_t filesize, int user_id)
{

    struct stat *fileStat = (struct stat *)calloc(1, sizeof(struct stat));

    char query[QUERY_LEN] = {0};
    sprintf(query, "select user_id from file where father_id = %d and file_name = '%s'", father_id, filename);
    char **result = NULL;
    int ret = database_operate(db_connect, query, &result);
    if (ret != 0 && (atoi(result[0]) == user_id))
    {
        //组合真实文件路径
        char real_filename[REAL_NAME_LEN] = NET_DISK;
        sprintf(real_filename, "%s%s", real_filename, md5);
        printf("filename=%s\n", real_filename);

        //打开真实文件
        int checkFd = open(real_filename, O_RDWR | O_CREAT, 0666);
        ERROR_CHECK(checkFd, -1, "open");

        //获取真实文件大小
        fstat(checkFd, fileStat);
        printf("size = %ld\n",fileStat->st_size);
        if (fileStat->st_size == filesize)
        {
            printf("client upload existed file\n");
            off_t Existing = UPLOAD_EXISTING_FILE;
            send(clientfd, &Existing, sizeof(off_t), 0);
            close(checkFd);
            return UPLOAD_EXISTING_FILE;
        }
        close(checkFd);
    }

    memset(fileStat,0,sizeof(struct stat));
    //组合真实文件路径，打开\创建真实文件，获取真实文件大小，向客户端发送文件大小
    int filefd = get_file_size(md5, 0, &clientfd, fileStat);

    //SQL查询语句
    sprintf(query, "select * from file_real where md5 = '%s'", md5);

    //查询file_real中是否存在此次上传的MD5码
    ret = database_operate(db_connect, query, NULL);
    // printf("op ret = %d\n", ret);

    if (ret == 0 || fileStat->st_size == filesize) //新文件，即秒传文件需要插入记录到file表
    {
        memset(query, 0, QUERY_LEN);

        //插入新纪录到file表
        sprintf(query, "insert into file (father_id,file_name,file_md5,file_size,type,user_id) values(%d, '%s', '%s', %ld, 'f',%d) ", father_id, filename, md5, filesize, user_id);

        printf("%s\n", query);

        database_operate(db_connect, query, NULL);
    }

    if (ret != 0 && fileStat->st_size == filesize) //MySQL中存在(即上传旧文件)，且大小相等：文件秒传
    {
        memset(query, 0, QUERY_LEN);

        //秒传只要更新file_real表中对应记录的file_count值
        sprintf(query, "update file_real set file_count = file_count + 1 where md5 ='%s'", md5);
        // printf("%s\n",query);

        database_operate(db_connect, query, NULL);
    }
    else //1、服务器中没有，就接收上传新文件。2、服务器中有但文件大小不等，就接收断点续传
    {
        if (ret == 0) //服务器中没有,MySQL中插入
        {
            memset(query, 0, QUERY_LEN);

            //新文件需要插入新记录到file_real表
            sprintf(query, "insert into file_real (md5,file_count) values ('%s',1)", md5);
            printf("%s\n", query);

            database_operate(db_connect, query, NULL);
        }

        //接收客户端上传
        off_t recvLen = 0;
        off_t needLen = filesize - fileStat->st_size;

        lseek(filefd, 0, SEEK_END); //读写指针偏移到文件尾：断点续传

        char gets_buf[BUF_LEN] = {0};
        // printf("before recv\n");
        while (recvLen < needLen)
        {
            memset(gets_buf, 0, BUF_LEN);

            ret = recv(clientfd, gets_buf, BUF_LEN, 0);
            ERROR_CHECK(ret, -1, "server recv");
            if (ret == 0)
            {
                return CLIENT_DISCONNECTION;
            }
            ret = write(filefd, gets_buf, ret);
            ERROR_CHECK(ret, -1, "server write");

            recvLen += ret;
        }
        // printf("after recv\n");
    }
    return 0;
}