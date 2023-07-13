#include "../head/func.h"

//void型的线程清理函数!
void clean_func(void *p){
    pTask_queue_t pQue = (pTask_queue_t)p;
    pthread_mutex_unlock(&pQue->queue_mutex);
}

/**
 * 线程处理函数，代表一个线程
 * 一个线程对应一个用户
*/
void *thread_func(void *p){

    //将传入进来的队列进行类型转换
    pTask_queue_t pQue = (pTask_queue_t)p;

    //创建任务节点用取任务
    pTask_node_t pTask = NULL;
    
    //取任务函数的返回值，0为失败，1为成功
    int get_success = 0;
    
    while(1){

        //上锁互斥访问队列，如果之前上锁，则会阻塞
        pthread_mutex_lock(&pQue->queue_mutex);

        //清理函数紧贴要清理的资源，前面上锁了所以要在清理函数里面解锁
        pthread_cleanup_push(clean_func, pQue);
        
        //如果队列中没有节点
        //函数上半部做的是：1.条件变量上排队2.解锁3.睡眠等待激发
        //函数下半部做的是：1.被唤醒（激发）2.加锁，如果已经上锁，则阻塞至锁解锁，然后加锁3.pthread_cond_wait函数返回
        if(0 == pQue->queue_len){
            pthread_cond_wait(&pQue->queue_cond, &pQue->queue_mutex);
        }

        //GetTaskNode取节点
        get_success = GetTaskNode(pQue, &pTask);
        
        //解锁
        pthread_mutex_unlock(&pQue->queue_mutex);
        pthread_cleanup_pop(0);

        //取节点成功，用文件描述符发送文件，任务完成释放节点资源，置为空指针
        if(1 == get_success){
            printf("----------------Client connection----------------\n");
            UserFunc(pTask);
            printf("------------------UserFunc Over------------------\n");
            free(pTask);
            pTask = NULL;
        }

        if(1 == pQue->exit_flag){
            printf("-------------------Thread Over-------------------\n");
            //线程退出
            pthread_exit(NULL);
        }

    }
}

/**
 * 线程池初始化函数
 * 参数1：线程池结构体地址
 * 参数2：线程数量
 * 返回值：成功返回0
*/
int ThreadPoolInit(pThread_pool_t pPool, int thread_num){

    int ret = 0;
    
    //线程数量
    pPool->thread_num = thread_num;
    
    //为线程申请堆空间
    pPool->pThread_id = (pthread_t *)calloc(thread_num, sizeof(pthread_t));

    //任务队列初始化
    ret = TaskQueueInit(&pPool->task_queue);
    THREAD_ERROR_CHECK(ret, "TaskQueInit");

    return 0;
}

/**
 * 线程池创建函数
 * 参数：线程池地址
 * 返回值：成功返回0
*/
int ThreadPoolCreate(pThread_pool_t pPool){
    
    int ret = 0;
    
    //循环创建线程
    for(int i = 0; i < pPool->thread_num; ++i){
        //两种方式创建线程
        /* ret = pthread_create(pPool->pThread_id + i, NULL, thread_func, &pPool->task_que); */
        ret = pthread_create(&pPool->pThread_id[i], NULL, thread_func, &pPool->task_queue);
        THREAD_ERROR_CHECK(ret, "pthread_create");
    }

    return 0;
}
