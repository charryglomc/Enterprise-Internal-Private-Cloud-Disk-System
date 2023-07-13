#define _GNU_SOURCE
#include "../head/func.h"
#include "../head/md5.h"
#include "../head/cilent.h"

//将命令和参数赋值给command结构体变量
int GetCommand(pcommand_t pcommand, char *buf)
{
	char *word[10] = {0};			//用来保存字符串转化的结果
	int i = TransToWord(buf, word); //i代表这条命令的参数有几个

	//如果用户输入的全是空格，那么word[0]中就只有一个'\0'，长度就是1
	if (strlen(word[0]) == 0)
	{
		//这里就什么也不做，直接返回
		return COMMAND_ERROR;
	}

	//用户输入的不是空格
	pcommand->c_argsnum = i;	//这个 是 bug；	
	strcpy(pcommand->c_content, word[0]); //拆分出来的第一个单词就是命令
	if (i == 1)							  //有一个参数
	{
		strcpy(pcommand->c_args1, word[1]);
	}
	else if (i == 2) //有两个参数
	{
		strcpy(pcommand->c_args1, word[1]);
		strcpy(pcommand->c_args2, word[2]);
	}

	return NO_ERROR; //正常赋值
}

//把指令字符串按照空格分割为单词存储
int TransToWord(char *str, char **word)
{ //word是一个指针数组，在传递参数时会退化为二级指针
	int i = 0;
	char *p1 = str, *p2 = str; //p1和p2分别指向每一个单词开头和单词结尾
	while (1)
	{
		while (*p1 == ' ') //p1指向的内容 是 空格时，p1，p2都像后移动，直到 其指向内容不是空格
		{
			p1++;
			p2++;
		}
		while (*p2 != ' ' && *p2 != '\0' && *p2 != '\n') //p1不动,p2向后移动，直到指向的内容是 空格 或者 \0
		{
			p2++;
		}
		if (*p2 == '\0' || *p2 == '\n') //判断p2是不是指向字符串的末尾了
		{
			break;
		}
		*p2 = '\0';
		p2++;
		word[i++] = p1; //得到了一个完整的单词，将他的地址给word[]
		p1 = p2;		//让p1 和 p2 指向相同位置，开始下一次循环
	}
	*p2 = '\0';
	word[i] = p1; //保存最后一个单词的地址

	return i;
}

//下载命令
int DownloadCommand(pcommand_t pcommand, int sfd)
{
	off_t recvsize = 0;	  //偏移量，用于断点续传
	char buf[4096] = {0}; //接受文件内容
	off_t filesize = 0;	  //文件大小
	int ret = 0, fd = 0;

	//分析文件路径，获得该文件名,根据 文件名，创建或者打开该文件
	char *p = strrchr(pcommand->c_args1, '/');
	if(p != NULL)
	{
		fd = open(++p, O_RDWR | O_CREAT, 0766);
		ERROR_CHECK(fd, -1, "open");
	}
	else
	{
		fd = open(pcommand->c_args1, O_RDWR | O_CREAT, 0766);
		ERROR_CHECK(fd, -1, "open");	
	}
	
	//获得文件大小，设置偏移量，并将文件大小赋值给command结构体变量
	struct stat fileinfo;
	memset(&fileinfo, 0, sizeof(fileinfo));
	ret = fstat(fd, &fileinfo);
	ERROR_CHECK(ret, -1, "fstat");
	recvsize = fileinfo.st_size; //用于断点续传，recvsize 表示 偏移量
	ret = lseek(fd, recvsize, SEEK_SET);
	ERROR_CHECK(ret, -1, "lseek"); //改变当前文件大小偏移ptr，使其能够指向正确的位置
	sprintf(pcommand->c_args2, "%ld", fileinfo.st_size);
	pcommand->c_argsnum += 1;

	//发送命令，并判断命令发送过程是否出错
	ret = SendCommand(pcommand, sfd);
	if(COMMAND_ERROR == ret || ARGS_NUM_ERROR == ret || ARGS_CON_ERROR == ret)
	{
		return FUNC_ERROR;
	}

	// 先接收off_t个字节，文件的大小
	ret = recv(sfd, &filesize, sizeof(off_t), MSG_WAITALL);
	ERROR_CHECK(ret, -1, "recv_filesize");

	// 如果收到的是-2，说明该文件不存在，回到主函数
	if (filesize == NO_SUCH_FILE)
	{
		printf("no such file\n");
		return NO_SUCH_FILE;
	}

	// 打印文件的相关信息
	printf("file size = %ld\n", filesize + recvsize);
	printf("need transport size = %ld\n", filesize);
	filesize += recvsize;

	// 循环接收文件内容
	while (recvsize < filesize)
	{ 	
		memset(buf, 0, sizeof(buf));
		ret = recv(sfd, buf, 4095, 0);				
		if (0 == ret)		//服务器中断，程序退出
		{			
			printf("connection break\n");
			exit(1);
		}
		ret = write(fd, buf, ret); 		// ret 本次接收到的字节，将接收到的数据写入文件当中
		ERROR_CHECK(ret, -1, "write");
		recvsize += ret;				// 写入到本地的字节数
		if (recvsize % 10240 == 0) 		// 每写入10 就打印一次进度
		{
			float rate = (float)recvsize / filesize * 100;
			printf("rate = %5.2f%%\r", rate);
			fflush(stdout);
		}
	}

	// 判断，如果退出接收时，已经写入的大小 = 文件大小，说明下载完成
	if (recvsize == filesize)
	{
		printf("download complished\n");
	}
	else
	{
		printf("someting wrong\n");
	}

	close(fd);
	return NO_ERROR;
}

//上传命令
int UploadCommand(pcommand_t pcommand, int sfd)
{
	off_t sendsize = 0; //偏移量
	int fd = 0;
	int ret = 0;
	char buf[4096] = {0}; //接收文件内容

	//获得要上传文件的md5码，并赋值给command结构体变量
	char file_path[128];
	sprintf(file_path, "%s", pcommand->c_args1);
	char md5_str[MD5_STR_LEN + 1];
	ret = Compute_file_md5(file_path, md5_str);
	strcpy(pcommand->c_args3, md5_str);
	pcommand->c_argsnum += 1;

	//根据输入的 文件名，打开该文件,
	fd = open(pcommand->c_args1, O_RDONLY, 0666); //文件不存在，返回-1
	if (fd == -1)
	{
		return NO_SUCH_FILE;
	}

	//获得文件大小, 把文件大小，放在command结构体中
	struct stat fileinfo;
	memset(&fileinfo, 0, sizeof(fileinfo));
	ret = fstat(fd, &fileinfo);
	ERROR_CHECK(ret, -1, "fstat");
	sprintf(pcommand->c_args2, "%ld", fileinfo.st_size);
	pcommand->c_argsnum += 1;

	//发送命令，并判断命令发送过程是否出错，出错回主函数
	ret = SendCommand(pcommand, sfd);
	if(COMMAND_ERROR == ret || ARGS_NUM_ERROR == ret || ARGS_CON_ERROR == ret)
	{
		return FUNC_ERROR;
	}	

	//接收 服务器 发给我的 偏移量
	ret = recv(sfd, &sendsize, sizeof(off_t), MSG_WAITALL);
	if (sendsize == -4)
	{
		printf("file exist\n");
		return NO_SUCH_FILE;
	}
	ERROR_CHECK(ret, -1, "recv_offset");

	//文件偏移
	ret = lseek(fd, sendsize, SEEK_SET);
	ERROR_CHECK(ret, -1, "lseek"); //改变当前文件大小偏移ptr，使其能够指向正确的位置

	//打印文件相关信息
	printf("[file - %s] md5 value: ", file_path);
	printf("%s\n", md5_str);
	printf("file size = %ld\n", fileinfo.st_size);
	printf("need transport size = %ld\n", fileinfo.st_size - sendsize);

	//循环发送文件内容
	while (sendsize < fileinfo.st_size)
	{
		memset(buf, 0, sizeof(buf));
		ret = read(fd, buf, sizeof(buf)); // ret 是本次读到的字节数
		ERROR_CHECK(ret, -1, "read");
		ret = send(sfd, buf, ret, 0); //将读到的字节数发送过去
		if (ret == -1)
		{
			printf("connection break\n");
			exit(1);
		}
		sendsize += ret; // 已经发送到缓冲区的字节数
		if (sendsize % 10240 == 0) // 每发送10字节 就打印一次进度
		{
			float rate = (float)sendsize / fileinfo.st_size * 100;
			printf("rate = %5.2f%%\r", rate);
			fflush(stdout);
		}
	}

	if (sendsize == fileinfo.st_size)
	{
		printf("upload complished\n");
	}
	else
	{
		printf("something wrong\n");
	}

	close(fd);
	return NO_ERROR;
}

//更改密码
int RepasswdCommand(pcommand_t pcommand, int sfd)
{
	int ret = 0;
	char buf[32] = {0};	 //存储第一次输入的密码
	char buf1[32] = {0}; //存储第二次输入的密码
	char salt[20] = {0}; //存储盐值

	ret = SendCommand(pcommand, sfd);
	if(COMMAND_ERROR == ret || ARGS_NUM_ERROR == ret || ARGS_CON_ERROR == ret)
	{
		return FUNC_ERROR;
	}
	
	while (1)
	{
		printf("enter new passwd:\n");
		// getchar();
		HidePasswd(buf);

		printf("\n");		//再次输入密码
		printf("configue new passwd:\n");
		char buf1[32] = {0};
		// getchar();
		HidePasswd(buf1);
		printf("\n");
		if (strcmp(buf, buf1) == 0)
		{
			break;
		}
		// 如果两次输入的密码不相同
		printf("passwd wrong, please reset passwd\n\n");
		continue;
	}

	//接受salt
	ret = recv(sfd, salt, 11, MSG_WAITALL);
	ERROR_CHECK(ret, -1, "recv_salt");

	//根据salt 和 密码加密 账户
	char *passwd = crypt(buf, salt);
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%s", passwd);
	send(sfd, buf, strlen(buf), MSG_WAITALL);

	recv(sfd, &ret, sizeof(int), 0);

	printf("reset password success\n\n");
	printf("please Re-Login \n\n");

	printf("enter any key to continue\n\n");
	getchar();
	system("clear");
	return NO_ERROR;
}

int SendCommand(pcommand_t pcommand, int sfd)
{
	int ret = send(sfd, pcommand, sizeof(command_t), 0);
	ERROR_CHECK(ret, -1, "send_command");

	//接收服务器发给我确认信息，如果出现命令在传输过程中丢失，则重新发送
	recv(sfd, &ret, 4, MSG_WAITALL);
	if (ARGS_NUM_ERROR == ret)
	{
		printf("args num wrong\n");
		return ARGS_NUM_ERROR;
	}
	else if (ARGS_CON_ERROR == ret || RUN_ERROR == ret)
	{
		printf("command run error\n");
		return ARGS_CON_ERROR;
	}
	else if(NO_ERROR == ret)
	{
		return NO_ERROR;
	}
	else
	{
		printf("no such command\n");
		return COMMAND_ERROR;
	}
}
