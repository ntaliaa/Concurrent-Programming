#ifndef MYSEM_H
#define MYSEM_H

#include <sys/types.h>

union semun {
    int val;
    struct semid_ds *info;
    unsigned short *vals;
};

typedef struct {
    unsigned short sem_num;  // semaphore number 
    short sem_op;   //semaphore operation 
    short sem_flg;  //operation flags 
    
} sembuf;

typedef struct{
    int sem_id;  // System V semaphore id
    int initialize;  // Flag to indicate if a semaphore is initialized
    union semun arg;
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