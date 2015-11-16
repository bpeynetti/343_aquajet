#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_



typedef struct pool_t pool_t;


pool_t *pool_create(int thread_count, int queue_size);

int pool_add_task(pool_t* pool,int taskType, void (*function)(void *), void *argument,int connfd,struct request req,float arrivalTime);

int pool_destroy(pool_t *pool);

#endif
