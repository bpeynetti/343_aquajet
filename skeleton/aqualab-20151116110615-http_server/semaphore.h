#ifndef _SEMAPHORE_DEF_H_
#define _SEMAPHORE_DEF_H_

//semaphore is shared by all
typedef struct m_sem_t {
    int value;
    pthread_cond_t notify;
    pthread_mutex_t lock;
} m_sem_t;

void sem_initialize(m_sem_t *s, int value);
int sem_wait(m_sem_t *s);
int sem_post(m_sem_t *s);
void sem_kill(m_sem_t *s);

 


#endif