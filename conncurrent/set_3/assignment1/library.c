#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "library.h"

//initialize semaphore
int mysem_init(mysem_t *s, int n){
    if(n!=0 && n!=1){
        return 0;
    }
    if(s->initialize ==1){
        return -1;
    }
    s->val = n;

    pthread_cond_init(&s->monitor.queue, NULL); 
    pthread_mutex_init(&s->monitor.lock, NULL);

    s->q_cnt=0;

    s->initialize = 1;

    return 1;
}

//drop semaphore
int mysem_down(mysem_t *s){
    pthread_mutex_lock(&s->monitor.lock);
    if(s->initialize == 0){
        return -1;
    }
    if(s->val == 1){
        s->val--;
    }
    else{
        s->q_cnt++;
        pthread_cond_wait(&s->monitor.queue,&s->monitor.lock);
    }

    pthread_mutex_unlock(&s->monitor.lock);
    return 1;

}

//raise semaphore
int mysem_up(mysem_t *s){
    pthread_mutex_lock(&s->monitor.lock);

    if(s->initialize == 0){
        return -1;
    }

    if(s->val==1){
        fprintf(stderr,"\33[31m*****************Lost up********************\33[37m\n");
        return 0;
    }

    //if queue is empty, set semaphore value to 1
    if(s->q_cnt==0){
        s->val++;
    }
    else{
        s->q_cnt--;
        pthread_cond_signal(&s->monitor.queue);
        
    }

    pthread_mutex_unlock(&s->monitor.lock);
    return 1 ;
}

//destroy semaphore
int mysem_destroy(mysem_t *s){

    if(s->initialize == 0 || s->initialize == -1 ){
        return -1;
    }

    s->initialize = -1; 
    fprintf(stderr,"Signal destroyed!\n");

    return 1;
}