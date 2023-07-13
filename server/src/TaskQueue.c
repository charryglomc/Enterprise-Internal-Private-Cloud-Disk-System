#include "../head/func.h"

/**
 * 功能：初始化任务队列
 * 参数：任务队列
 * 返回值：返回0，不会失败
 */
int TaskQueueInit(pTask_queue_t pQue){

    int ret = 0;
    pQue->queue_len = 0;
    pQue->pHead = NULL;
    pQue->pTail = NULL;
    ret = pthread_cond_init(&pQue->queue_cond, NULL);
    THREAD_ERROR_CHECK(ret, "pthread_cond_init");
    ret = pthread_mutex_init(&pQue->queue_mutex, NULL);
    THREAD_ERROR_CHECK(ret, "pthread_mutex_init");

    return 0;
}

/**
 * 功能：将任务插入任务队列
 * 参数1：任务队列
 * 参数2：任务节点
 * 返回值：返回0，不会失败
 */
int InsertTaskQueue(pTask_queue_t pQue, pTask_node_t pTaskNode){
    
    //任务队列中没有任务节点时，头尾指针指向新加入的任务节点
    if(0 == pQue->queue_len){
        pQue->pHead = pTaskNode;
        pQue->pTail = pTaskNode;
    }

    //有节点，尾插法
    else{
        pQue->pTail->pNext = pTaskNode;
        pQue->pTail = pTaskNode;
    }

    //入队后队列长度+1
    ++pQue->queue_len;
    
    return 0;
}

/**
 * 功能：取出任务队列中的任务
 * 参数1：任务队列
 * 参数2：出队节点（任务）
 * 返回值：取节点成功返回1，失败返回0
 */
int GetTaskNode(pTask_queue_t pQue, pTask_node_t *pGetTaskNode){

    //有节点，头部删除法
    if(0 != pQue->queue_len){
        
        //待取出的节点指向头指针
        *pGetTaskNode = pQue->pHead;
        pQue->pHead = pQue->pHead->pNext;
        (*pGetTaskNode)->pNext = NULL;
        
        //出队后队列长度-1
        --pQue->queue_len;
        
        //取完后队列长度为0，把尾指针置空
        if(0 == pQue->queue_len){
            pQue->pTail = NULL;
        }
        return 1;
    }

    //没节点，返回0
    else{
        return 0;
    }
}
