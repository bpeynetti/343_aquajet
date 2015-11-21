#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include "util.h"
#include "thread_pool.h"
#include "seats.h"

/**
 *  @struct threadpool_task
 *  @brief the work struct
 *
 *  Feel free to make any modifications you want to the function prototypes and structs
 *
 *  @var function Pointer to the function that will perform the task.
 *  @var argument Argument to be passed to the function.
 */

#define MAX_THREADS 20

// enum task_t{
//     PARSE,
//     PROCESS
// }
#define PARSE 1
#define PROCESS 2
#define KILL_THREAD 3



typedef struct {
    void (*function)(void *);
    void *argument;
    void* next;
    void* previous;
    int  taskType;
    struct request req;
    int connfd;
    // enum priority_t priority;
    float arrival_time;
    
} pool_task_t;


struct pool_t {
  pthread_mutex_t lock; //this is used to lock the pool data structure
  pthread_cond_t notify; //this signals threads to wake up
  pthread_t *threads; //this is an array with each thread
  pool_task_t *queue; //this is the queue (priority queue) for the requests
  int thread_count; //this counts the number of threads available
  int task_queue_size_limit; //a limit to requests
  void* queue_head;
  void* queue_tail;
  int current_queue_size;
  int total_requests;
  float total_time_elapsed;
};

static void *thread_do_work(pool_t* pool);
pool_task_t* get_next_task(pool_t* pool);
void queueDelete(pool_task_t* queue);


/*
 * Create a threadpool, initialize variables, etc
 *
 */
pool_t *pool_create(int queue_size, int num_threads)
{
    printf("Creating pool \n");
    pool_t* pool = (pool_t*) malloc(sizeof(pool_t));
    
    //initialize mutex for the pool
    pthread_mutex_init(&(pool->lock),NULL);
    pthread_mutex_lock(&(pool->lock));
    //initialize pthread condition 
    pthread_cond_init(&(pool->notify),NULL);
    //initialize thread count
    pool->thread_count = num_threads;
    //initialize limit of tasks
    pool->task_queue_size_limit = queue_size;
    pool->current_queue_size = 0;
    //initialize a priority queue 
    //for now, empty queue
    pool->queue = NULL;
    //pool->queue = initializePriorityQueue();
    //create the threads
        //add a loop to create all threads (thread_count)
    pool->threads = (pthread_t*) malloc(sizeof(pthread_t)*pool->thread_count);
    int t=0;
    for (t=0;t<pool->thread_count;t++)
    {
        printf("creating thread %d \n",t+1);
        //assign the function and the arguments that each thread will take
        pthread_create(&(pool->threads[t]),NULL,(void*) &thread_do_work,(void*) pool);
    }
    //return the pool that you created
    pool->queue_head=NULL;
    pool->queue_tail=NULL;
    
    pool->total_requests = 0;
    pool->total_time_elapsed = 0.00;
    
    pthread_mutex_unlock(&(pool->lock));
    printf("Created the pool and released lock\n");
    return pool;
}


/*
 * Add a task to the threadpool
 *
 */
int pool_add_task(pool_t* pool,int taskType, void (*function)(void *), void *argument,int connfd,struct request req, float arrivalTime)
{
    //printf("Adding task  \n");
    //all we do here is add a task to the queue
    //if we implement priority queue, then add the proper way to a priority queue
    
    //hold on to the queue while I modify it
    //printf("Waiting for pool\n");
    pthread_mutex_lock(&(pool->lock));
    
    if (taskType==PARSE)
    {
        pool->total_requests++;
    }
    
    //printf("Got the lock \n");
    //if size of queue is full, return -1
    if (pool->task_queue_size_limit==pool->current_queue_size){
        //too big, can't do this
        return -1;
    }
    
    //create new node 
    pool_task_t* newRequest = (pool_task_t*)(malloc(sizeof(pool_task_t)));
    //printf("new node goes at %p \n",newRequest);
    newRequest->function = function;
    newRequest->argument = argument;
    newRequest->taskType = taskType;
    newRequest->req = req;
    newRequest->next = NULL;
    newRequest->connfd = connfd;
    newRequest->previous = NULL;
    newRequest->arrival_time=arrivalTime;

    pool_task_t* headNode = pool->queue_head;
    
    if (headNode!=NULL)
    {
        headNode->previous = newRequest;
    }
    
    pool->queue_head = newRequest;
    
    if (pool->current_queue_size==0)
    {
        pool->queue_tail = newRequest;
    }
    
    pool->current_queue_size++;
    //printf("releasing the lock on pool\n");
    pthread_mutex_unlock(&(pool->lock));
    //printf("released lock on pool \n");
    
   // printf("Finished adding task. Currently %d tasks. Located at %p ,broadcasting \n",pool->current_queue_size,pool->queue_head);
    pthread_cond_signal(&(pool->notify));
    return 0;
    
}


pool_task_t* get_next_task(pool_t *pool)
{
    //note -> we need to free the node after we return it and extract the information!
    //this will just return and fix the list
   // printf("getting a task, out of %d available\n",pool->current_queue_size);
    if (pool->queue_head==NULL)
    {
        //printf("no tasks \n");
        return NULL;
    }
    
    //get tail to return
    pool_task_t* temp;
    temp = pool->queue_tail;
    
    //set tail to previous
    pool->queue_tail = temp->previous;
    
    //if it's not the only one, update the next to NULL
    if (pool->queue_tail!=NULL)
    {
        pool_task_t* tailNode = pool->queue_tail;
        tailNode->next = NULL;
    }
    
    //decrease the size
    pool->current_queue_size--;
    
    //if down to 0, set head and tail to null
    if (pool->current_queue_size==0)
    {
        pool->queue_tail=NULL;
        pool->queue_head=NULL;
    }
    return temp;

}

/*
 * Destroy the threadpool, free all memory, destroy treads, etc
 *
 */
int pool_destroy(pool_t *pool)
{
    printf("destroying thread pool\n");
    int err = 0;
    //pthread_mutex_lock(&(pool->lock));
    
    //display stats
   
    
    //send threads # to request
    int i=0;
    for (i=0;i<pool->thread_count;i++)
    {
        struct request req = {0,0,0,NULL};
        pool_add_task(pool,3,NULL,NULL,0,req,0.0);
    }
    
    for (i=0;i<pool->thread_count;i++)
    {
        
        printf("joining thread %d\n",i);
        if(pool->threads[i] != pthread_self())
        {
            pthread_join(pool->threads[i], NULL);
        }
        //printf("joined \n");
    }
    // destroy mutex
    //pthread_mutex_unlock(&(pool->lock));
    pthread_mutex_destroy(&(pool->lock));
    // destroy conditionj
    pthread_cond_destroy(&(pool->notify));
    //destroy queue
    //printf("destroying queue\n"); 
    queueDelete(pool->queue_head);
 
    printf("Total requests: %d \n",pool->total_requests);
    float time_per_request;
    time_per_request = pool->total_time_elapsed / pool->total_requests;
    printf("Avg time per request: %f \n",time_per_request);
    
    //destroy pool
    free((void*)pool->threads);
    free((void*)pool);
    
    return err;
}

void queueDelete(pool_task_t* queue)
{
    //deletes the queue
    if (queue==NULL)
    {
        //printf("nothing left \n");
        return;
    }
    
    //go one by one, deleting the queue
    pool_task_t* next = queue->next;
    while (queue!=NULL)
    {
        next = queue->next;
        free(queue);
        queue = next;
    }
    return;
}



/*
 * Work loop for threads. Should be passed into the pthread_create() method.
 *
 */
static void *thread_do_work(pool_t *pool)
{ 

    //printf("Waiting ... \n");
    //lock it before you go into the loop, for unlocking later
    pthread_mutex_lock(&(pool->lock));
    while(1) {
        
        //lock the pool
        // printf("locking the pool \n");
         //pthread_mutex_lock(&(pool->lock));
        // printf("got the lock on the pool \n");
        //get the function and argument
        pool_task_t* task;
        task = get_next_task(pool);
        pthread_mutex_unlock(&(pool->lock));

        while(task!=NULL)
        {
            //printf("got a task from the pool\n");
            //printf("task type is %d \n",task->taskType);
            //printf("task: %p\n",task);
            //don't need it for now, so unlock it
            //printf("task tail is %p \n",pool->queue_tail);
            
            //if task is parsing, then parse it
            //and add the task to the stuff
            if (task->taskType==PARSE)
            {
                //then ask for the parsing with task req as one of the arguments
                //printf("------- Parsing task\n");
                int error = parse_request(task->connfd,&(task->req));
                if (error==-1)
                {
                    //bad request, return 0
                    //printf("bad request. connection closed \n");
                    //add to pool total time
                    float finish_time = clock();
                    pool->total_time_elapsed += (finish_time - task->arrival_time) / CLOCKS_PER_SEC;
                    
                }
                else
                {
                    // struct request req = task->req;
                    //printf("adding process to task list \n");
                    pool_add_task(pool,PROCESS,NULL,NULL,task->connfd,task->req,task->arrival_time);
                }
            }
            
            if (task->taskType==PROCESS)
            {
                //printf("-------- processing task \n");
                //struct request req = task->req;
                //printf("Task is: %s for user: %d, seat %d, priority: %d  \n",req.resource,req.user_id,req.seat_id,req.customer_priority);
                process_request(task->connfd,&(task->req));
                close(task->connfd);
                //finish so get time to request and add
                float f_time = clock();
                pool->total_time_elapsed += (f_time - task->arrival_time) / CLOCKS_PER_SEC;
            }
            if (task->taskType==KILL_THREAD)
            {
                //free task node
                free(task);
                
                //return thread
                pthread_exit(NULL);
                return(NULL);
                
            }

            
            //free the node
            //printf("freeing the location of task : %p\n",task);
            // task->connfd = 0;
            free(task);
            
            //lock pool and get task
            pthread_mutex_lock(&(pool->lock));
            //get the next task
            task = get_next_task(pool);
            pthread_mutex_unlock(&(pool->lock));
        }
        //lock it so we can wait
        //printf("Nothing, so releasing\n");
        pthread_mutex_lock(&(pool->lock));
        pthread_cond_wait(&(pool->notify),&(pool->lock));
       // printf("Waking up! \n");
    }

    pthread_exit(NULL);
    return(NULL);
}
