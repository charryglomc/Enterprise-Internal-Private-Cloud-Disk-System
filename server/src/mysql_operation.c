#include "../head/func.h"

int detectsame(MYSQL *conn,char *query,int qlen,char *srcname,int dirid)
{
	char **result=NULL;
	int row=0;
	memset(query,0,qlen);
	sprintf(query,"select id from file where father_id=%d;",dirid);
	puts(query);
	row=database_operate(conn,query,&result);
	//获取所有目录文件下的id
	char allchildid[128][20];
	memset(allchildid,0,sizeof(allchildid));
	char file_name[21];
	for(int i=0;i<row;i++)
	{
		strcpy(allchildid[i],result[i]);
		memset(query,0,qlen);
		getnamefromid(conn,query,qlen,atoi(allchildid[i]),file_name);
		printf("file_name of allchildid[%d] =%s;",i,file_name);
		if(!strcmp(file_name,srcname))
		{
			printf("there are same filename\n");
			return 0;//成功返回0
		}

	}
	
	//无重复文件
	return -1;
}
int addcountfrommad5(MYSQL *conn,char *query,int qlen,char *file_md5)
{
	char **result=NULL;
	int row=0;
	memset(query,0,qlen);
	strcpy(query,"update file_real set file_count=file_count+1 where md5='");
	sprintf(query,"%s%s%s",query,file_md5,"';");
	puts(query);
	row=database_operate(conn,query,&result);
	printf("addcountfrommad5 row=%d\n",row);
	return 0;

}
int delcountfrommd5(MYSQL *conn,char *query,int qlen,char *file_md5)
{
	char **result=NULL;
	int row=0;
	char count[20];
	memset(count,0,sizeof(count));
	memset(query,0,qlen);
	strcpy(query,"update file_real set file_count=file_count-1 where md5='");
	sprintf(query,"%s%s%s",query,file_md5,"';");
	puts(query);
	row=database_operate(conn,query,&result);
	printf("delcountfrommad5 row=%d\n",row);

	memset(query,0,qlen);
	strcpy(query,"select file_count from file_real where md5='");
	sprintf(query,"%s%s%s",query,file_md5,"';");
	puts(query);
	row=database_operate(conn,query,&result);
	printf("delcountfrommad5 row=%d\n",row);
	strcpy(count,*result);
	printf("count from %s =%d\n",file_md5,atoi(count));
	
	if(atoi(count)==0)
	{
		printf("call rmfilefunc\n");
		memset(query,0,qlen);
		sprintf(query,"delete from file_real where md5='%s';",file_md5);
		row=database_operate(conn,query,&result);
	}


	return 0;
	
}
int getnamefromid(MYSQL *conn,char *query,int qlen,int id,char *file_name)
{
	char **result=NULL;
	int row=0;
	memset(query,0,qlen);
	memset(file_name,0,21);
	strcpy(query,"select file_name from file where id=");
	sprintf(query,"%s%d%s",query,id,";");
	puts(query);
	row=database_operate(conn,query,&result);
	strcpy(file_name,*result);
	printf("file_name=%s\n",file_name);
	printf("getnamefromid row=%d\n",row);
	if(row==0)
	{
		printf("file where id=id not exist\n");
		return -1;
	}

	return 0;
}
int getsizefromid(MYSQL *conn,char *query,int qlen,int id,char *file_size)
{

	char **result=NULL;
	int row=0;
	memset(query,0,qlen);
	memset(file_size,0,21);
	strcpy(query,"select file_size from file where id=");
	sprintf(query,"%s%d%s",query,id,";");
	puts(query);
	row=database_operate(conn,query,&result);
	printf("getsizefromid row=%d\n",row);
	strcpy(file_size,*result);
	printf("file_size=%s\n",file_size);
	if(row==0)
	{
		printf("file where id=id not exist\n");
		return -1;
	}

	return 0;

}
int gettypefromid(MYSQL *conn,char *query,int qlen,int id,char *type)
{
	char **result=NULL;
	int row=0;
	memset(type,0,1);
	memset(query,0,qlen);
	strcpy(query,"select type from file where id=");
	sprintf(query,"%s%d%s",query,id,";");
	puts(query);
	row=database_operate(conn,query,&result);
	printf("gettypefromid row=%d\n",row);
	strcpy(type,*result);
	printf("type=%c\n",*type);
	if(row==0)
	{
		printf("file where id=id not exist\n");
		return -1;
	}

	return 0;


}
int getmd5fromid(MYSQL *conn,char *query,int qlen,int id,char *file_md5)
{
	char **result=NULL;
	int row=0;
	memset(file_md5,0,33);
	memset(query,0,qlen);
	strcpy(query,"select file_md5 from file where id=");
	sprintf(query,"%s%d%s",query,id,";");
	puts(query);
	row=database_operate(conn,query,&result);
	printf("getmd5fromid row=%d\n",row);
	strcpy(file_md5,*result);
	printf("file_md5=%s\n",file_md5);
	if(row==0)
	{
		printf("file where id=id not exist\n");
		return -1;
	}

	return 0;
}
int digui(MYSQL *conn,char *src,int srcid,int destid,int user_id,int flag,char *newdirname)
{
	//变量准备	
	int qlen=4096;
	char query[4096];
	memset(query,0,sizeof(query));
	int row=0;
	char **result;
	memset(query,0,sizeof(query));

	//在数据库，新建目录文件数据项
	//获得源目录文件md5
	char file_md5[33];
	getmd5fromid(conn,query,qlen,srcid,file_md5);
	//获得源目录文件size			
	char file_size[20];
	getsizefromid(conn,query,qlen,srcid,file_size);
	//新建数据库，目录文件数据项
	if(flag==0)
	{
		strcpy(query,"insert into file (father_id,file_name,file_md5,file_size,type,user_id) values(");
		sprintf(query,"%s%d%s%s%s%s%s%s%s%d%s",query,destid,",'",src,"','",file_md5,"',",file_size,",'d',",user_id,");");
		puts(query);
		row=database_operate(conn,query,&result);
	}
	else
	{	
		strcpy(query,"insert into file (father_id,file_name,file_md5,file_size,type,user_id) values(");
		sprintf(query,"%s%d%s%s%s%s%s%s%s%d%s",query,destid,",'",newdirname,"','",file_md5,"',",file_size,",'d',",user_id,");");
		puts(query);
		row=database_operate(conn,query,&result);
	}
	//获取数据库新建数据项的id值,通过newdestid保存
	char newdestid[20];
	memset(newdestid,0,sizeof(newdestid));	
	memset(query,0,sizeof(query));
	strcpy(query,"select max(id) from file;");
	row=database_operate(conn,query,&result);
	strcpy(newdestid,*result);
	//寻找源目录文件的所有文件，row为源目录文件所包含的文件数
	memset(query,0,sizeof(query));
	strcpy(query,"select id from file where father_id=");
	sprintf(query,"%s%d%s",query,srcid,";");
	puts(query);
	row=database_operate(conn,query,&result);
	printf("child of src num(row)=%d\n",row);
	char allchildid[16][16];
	int idnum=row;
	memset(allchildid,0,sizeof(allchildid));
	//获取源目录文件中所有的文件id;
	for(int i=0;i<row;i++)
	{
		strcpy(allchildid[i],result[i]);
		printf("id=%s\n",allchildid[i]);
	}

	if(!row)//源文件目录为空文件
	{
		return 0;
	}
	else
	{
		char type;
		char file_name[21];
		for(int i=0;i<idnum;i++)
		{
			//获取目录中第i个目录文件的类型
			gettypefromid(conn,query,qlen,atoi(allchildid[i]),&type);
			//获取file_name
			getnamefromid(conn,query,qlen,atoi(allchildid[i]),file_name);


			if(type=='d')
			{

				int newsrcid=atoi(allchildid[i]);
				int ndest=atoi(newdestid);
				digui(conn,file_name,newsrcid,ndest,user_id,0,NULL);

			}
			else
			{
				//获取file_name
				getnamefromid(conn,query,qlen,atoi(allchildid[i]),file_name);

				//获取该文件md5
				getmd5fromid(conn,query,qlen,atoi(allchildid[i]),file_md5);

				//获取file_size
				getsizefromid(conn,query,qlen,atoi(allchildid[i]),file_size);

				//新建源目录文件数据库目录项即为复制的parent_id;
				int father_id=atoi(newdestid);
				memset(query,0,sizeof(query));
				strcpy(query,"insert into file (father_id,file_name,file_md5,file_size,type,user_id) values(");  
				sprintf(query,"%s%d%s%s%s%s%s%s%s%d%s",query,father_id,",'",file_name,"','",file_md5,"',",file_size,",'f',",user_id,");");
				puts(query);
				row=database_operate(conn,query,&result); 
				//被复制文件的count++；
				addcountfrommad5(conn,query,32,file_md5);


			}

		}
	}

	return 0;


}


