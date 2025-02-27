#ifndef MYSEM_H
#define MYSEM_H

#include <pthread.h>

typedef struct{
    pthread_cond_t queue; //queue of the semaphore
    pthread_mutex_t lock; // mutex of the monitor
}monitors;

typedef struct{
    monitors monitor; //monitor of the semaphore
    int initialize;  // Flag to indicate if a semaphore is initialized
    int val; //value of semaphore
    int q_cnt; //queue counter
}mysem_t;


//Initializing the semaphore
int mysem_init(mysem_t *s, int n);

//Decrease by 1 the value
int mysem_down(mysem_t *s);

//Increase by 1 the value 
int mysem_up(mysem_t *s);

//Destroy the semaphore
int mysem_destroy(mysem_t *s);

#endif