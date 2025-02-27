#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include "library.h"

static pthread_mutex_t lock;


int mysem_init(mysem_t *s, int n){
    if(n!=0 && n!=1){
        return 0;
    }
    if(s->initialize ==1){
        return -1;
    }

    if((s->sem_id = semget(IPC_PRIVATE,1,IPC_CREAT|0666))==-1){
        perror("Semget failed\n");
        exit(1);
    }

    s->arg.val = n;

    if((semctl(s->sem_id,0,SETVAL,s->arg))==-1){
        perror("Semctl failed\n");
        exit(1);
    }

    pthread_mutex_init(&lock, NULL);
    s->initialize = 1;

    return 1;
}

int mysem_down(mysem_t *s){
    if(s->initialize == 0){
        return -1;
    }


    int sem_value = semctl(s->sem_id, 0, GETVAL);
    if (sem_value == -1) {
         perror("semctl GETVAL failed");
         exit(1);
    }
  
    struct sembuf s1;

    pthread_mutex_lock(&lock);
    s1.sem_num = 0;  // Semaphore number in the set
    s1.sem_op = -1;   // Down operation (increment)
    s1.sem_flg = 0;  // Default flag

    pthread_mutex_unlock(&lock);

    if (semop(s->sem_id, &s1, 1) == -1) {
        perror("semop up operation failed");
        exit(1);
    }

  
    return 1;

}

int mysem_up(mysem_t *s){
    if(s->initialize == 0){
        return -1;
    }

    

    int sem_value = semctl(s->sem_id, 0, GETVAL);
    if (sem_value == -1) {
        perror("semctl GETVAL failed");
         exit(1);
    }

    struct sembuf s2;

    pthread_mutex_lock(&lock);
    s2.sem_num = 0;  // Semaphore number in the set
    s2.sem_op = 1;   // Up operation (increment)
    s2.sem_flg = 0;  // Default flag
    
   
   if(sem_value==1){
        pthread_mutex_unlock(&lock);
        fprintf(stderr,"\33[31m*****************Lost up********************\33[37m\n");
        return 0;
    }
    else{
        pthread_mutex_unlock(&lock);
        if (semop(s->sem_id, &s2, 1) == -1) {
            perror("semop up operation failed");
            exit(1);
        }
    }

    return 1;

}

int mysem_destroy(mysem_t *s){
    if(s->initialize == 0 || s->initialize == -1 ){
        return -1;
    }
    
    pthread_mutex_lock(&lock);
    
    if((semctl(s->sem_id,0,IPC_RMID,s->arg))==-1){
        perror("Semctl failed\n");
        exit(1);
    }
    
    s->initialize = -1; 
    
    pthread_mutex_unlock(&lock);

    return 1;

}
