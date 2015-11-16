#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "semaphore.h"

void sem_initialize(m_sem_t *s, int value)
{
    //initialize semaphore value to whatever input is
    s->value = value;
    //initialize mutex elements
    pthread_mutex_init(&(s->lock),NULL);
    pthread_cond_init(&(s->notify),NULL);
    return;
}

int sem_wait(m_sem_t *s)
{
    //TODO
    //wait until i can grab the lock
    pthread_mutex_lock(&(s->lock));
    while (s->value <= 0)
    {
        //wait for the lock to be free
        pthread_cond_wait(&(s->notify),&(s->lock));
        //got the lock, and if it's 1, out of while. else, someone beat me to it
    }
    //set to 0
    s->value--;
    //and unlock the lock (it's already 0 so not a problem for others to look at it. they won't touch it)
    pthread_mutex_unlock(&(s->lock));
    //wait in a quue
    return 0;
}

int sem_post(m_sem_t *s)
{
    //TODO
    //take it, and will update to 1
    pthread_mutex_lock(&(s->lock));
    //increment value of lock to 1
    s->value++;
    //and let someone else have it by signaling everyone
    pthread_cond_signal(&(s->notify));
    //unlock it so someone can take 
    pthread_mutex_unlock(&(s->lock));
    return 0;
}

void sem_kill(m_sem_t *s)
{
    //kills the mutex for semaphores
    pthread_mutex_destroy(&(s->lock));
    pthread_cond_destroy(&(s->notify));
    
}