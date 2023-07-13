#include "../head/func.h"

int out_pipe[2];

void sigfunc(int signum){
    printf("sig = %d is coming\n", signum);
    write(out_pipe[1], "1", 1);
}

int main()
{
    signal(10, sigfunc);
    pipe(out_pipe);

    if(fork()){
        //主进程关闭读端
        close(out_pipe[0]);

        //等待子进程的退出然后退出
        wait(NULL);
        printf("main out\n");
        exit(0);
    }
    
    //子进程关闭写端
    close(out_pipe[1]);

    int ret = 0;

    //读取ip配置文件
    char ip[16] = { 0 }; 
    int port = 0;
    FILE *fp = fopen("./config/ip.ini", "r");
    fscanf(fp, "ip = %s port = %d", ip, &port);
    fclose(fp);

    //创建线程池结构体，不是指针！
    thread_pool_t pool;
    memset(&pool, 0, sizeof(thread_pool_t));

    //初始化线程池，创建pthread_num个线程空间，创建任务队列
    ret = ThreadPoolInit(&pool, PTHREAD_NUM);
    THREAD_ERROR_CHECK(ret, "ThreadPoolInit");

    //开启线程池，创建线程，接收任务
    ret = ThreadPoolCreate(&pool);
    THREAD_ERROR_CHECK(ret, "ThreadPoolCreate");

    //创建Tcp套接字监听连接
    socket_fd sfd = TcpInit(ip, port);
    ERROR_CHECK(ret, -1, "TcpInit");

    //创建epoll，参数只要大于0就行，并将需要监听的文件描述符添加到epoll中
    int epoll_fd = epoll_create(1);
    ret = EpollAddFd(epoll_fd, sfd);
    ERROR_CHECK(ret, -1, "EpollAddFd");
    ret = EpollAddFd(epoll_fd, out_pipe[0]);
    ERROR_CHECK(ret, -1, "EpollAddFd");

    //epoll结构体，存放就绪文件描述符
    struct epoll_event evs[2];
    memset(evs, 0, sizeof(struct epoll_event));

    while(1){

        //-1表示无条件等待
        int ready_num = epoll_wait(epoll_fd, evs, 2, -1);

        for(int i = 0; i < ready_num; ++i){

            //收到退出信号
            if(evs[i].data.fd == out_pipe[0]){
                
                //修改任务队列中的退出标号为1
                pthread_mutex_lock(&pool.task_queue.queue_mutex);
                pool.task_queue.exit_flag = 1;
                pthread_mutex_unlock(&pool.task_queue.queue_mutex);

                //激发全部条件变量
                ret = pthread_cond_broadcast(&pool.task_queue.queue_cond);
                THREAD_ERROR_CHECK(ret, "pthread_cond_broadcast");

                //等待线程退出
                for(int j = 0; j < PTHREAD_NUM; ++j){
                    pthread_join(pool.pThread_id[j], NULL);
                }

                printf("child process exit\n");
                //子进程退出
                exit(0);
            }

            if(evs[i].data.fd == sfd){

                printf("client coming\n");

                //创建结构体存储对端地址和端口
                struct sockaddr_in addr_client;
                socklen_t len = sizeof(struct sockaddr_in);
                memset(&addr_client, 0, len);

                //接收新连接的文件描述符
                client_fd new_fd = accept(sfd, (struct sockaddr *)&addr_client, &len);
                ERROR_CHECK(new_fd, -1, "accept");

                //在堆空间申请任务
                pTask_node_t pTask = (pTask_node_t)calloc(1, sizeof(task_node_t));

                //添加客户端信息
                pTask->user_cfd = new_fd;
                strcpy(pTask->user_ip, inet_ntoa(addr_client.sin_addr));

                //上锁互斥访问任务队列
                pthread_mutex_lock(&pool.task_queue.queue_mutex);

                //将与客户端通信的文件描述符加入任务队列
                ret = InsertTaskQueue(&pool.task_queue, pTask);
                ERROR_CHECK(ret, -1, "Insert pTask queue");

                //激发所有子线程
                ret = pthread_cond_broadcast(&pool.task_queue.queue_cond);
                THREAD_ERROR_CHECK(ret, "broadcast");

                //解锁
                pthread_mutex_unlock(&pool.task_queue.queue_mutex);
                
            }

        }
    }
}

