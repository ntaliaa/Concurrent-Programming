#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

static int number;     //next number to check 
static int terminate;  //flag to terminate
static int worker_q=0; //counter for workers in queue
static int num_of_workers;

typedef struct{
    pthread_cond_t main_wait; // main is waiting for available thread
    pthread_cond_t wait_for_num; // main waits for thread to get number
    pthread_cond_t queue; // queue of available threads
    pthread_mutex_t lock;
}monitor;

static monitor m;

typedef struct {
    int worker_id;
}worker;

int is_prime(int number){

    int i;
    if (number <= 1){
        return 0;
    }
    if (number == 2){
        return 1;
    }    
    if (number % 2 == 0){
        return 0;
    }
    for (i = 3; i <= sqrt(number); i += 2) {
        if (number % i == 0){
            return 0;
        }    
    }
    return 1;
}

void *worker_op(void *arg) {
    worker *worker_args = (worker *)arg;    //worker arguments
    int result, num;

    while(1){

        pthread_mutex_lock(&m.lock);
        if (terminate == 1) {
            fprintf(stderr, "\33[33mThread %d terminated\33[37m\n", worker_args -> worker_id);
            break;
        }
        
        if(worker_q==0){
          
            pthread_cond_signal(&m.main_wait);

            worker_q++;
            
            pthread_cond_wait(&m.queue, &m.lock);

            if (terminate == 1) {
                fprintf(stderr, "\33[33mThread %d terminated\33[37m\n", worker_args -> worker_id);
                break;
            }

            num = number;
            pthread_cond_signal(&m.wait_for_num);
            pthread_mutex_unlock(&m.lock);

            fprintf(stderr,"\033[32mWorker with id: %d, checking the number %d\n\033[37m", worker_args->worker_id, num);

            result = is_prime(num);     //call function to check if number is prime

            //Print the result received
            if(result) {
                printf("\033[31m%d Prime\033[37m\n", num);
            }
            else {
                printf("\033[31m%d Not Prime\033[37m\n", num);
            }
        }
        else {
            worker_q++;
            
            pthread_cond_wait(&m.queue, &m.lock);
            if (terminate == 1) {
                fprintf(stderr, "\33[33mThread %d terminated\33[37m\n", worker_args -> worker_id);
                break;
            }
            

            num = number;
            pthread_cond_signal(&m.wait_for_num);
            pthread_mutex_unlock(&m.lock);
            fprintf(stderr,"\033[32mWorker with id: %d, checking the number %d\n\033[37m", worker_args->worker_id, num);
            result = is_prime(num);     //call function to check if number is prime

            //Print the result received
            if(result) {
                printf("\033[31m%d Prime\033[37m\n", num);
            }
            else {
                printf("\033[31m%d Not Prime\033[37m\n", num);
            }
        }
      
    }
     pthread_mutex_unlock(&m.lock);
    
    return NULL;
}

int main(int argc,char* argv[]){

    pthread_mutex_init(&m.lock,NULL);
    pthread_cond_init(&m.main_wait,NULL);
    pthread_cond_init(&m.queue,NULL);

    // static int num_of_workers;
    int *number_array;
    int numbers_amount;
    int i;
    pthread_t *worker_threads;
    worker *worker_args;

    //Exit if the user did not input arguments to the command line
    if(argc == 1) {
        printf("\033[37mNo commands given!\033[37m\n");
        return 0;
    }
    //Exit if the user did not give any numbers fot th
    if(argc == 2) {
        printf("\033[37mNo numbers to check given!\033[37m\n");
        return 0;
    }

    //Get from the user's input the number of worker threads and the amount of given integers to be checked
    num_of_workers = atoi(argv[1]);
    numbers_amount = argc - 2;

    //Allocate memory for the array that contains the numbers to be checked
    number_array = (int*)malloc(sizeof(int) * (numbers_amount));

    //Store the numbers to the array
    for(i = 2; i < argc; i++) {
        number_array[i-2] = atoi(argv[i]);
    }

    //Allocate memory for the worker threads and their arguement structs
    worker_args = (worker*)malloc(sizeof(worker) *num_of_workers);
    worker_threads = (pthread_t*)malloc(sizeof(pthread_t) * num_of_workers);

    //Create worker threads
    for(i = 0; i < num_of_workers; i++) {

        worker_args[i].worker_id = i;
        if (pthread_create(&worker_threads[i], NULL, worker_op, &worker_args[i]) != 0) {
        fprintf(stderr, "Error creating thread %d.\n", i);
        return 0;
        }
    }

    for (i=0; i<numbers_amount; i++) {
        
        pthread_mutex_lock(&m.lock);
        if (worker_q == 0){

            pthread_cond_wait(&m.main_wait,&m.lock);
            number = number_array[i];
            worker_q--;
            pthread_cond_signal(&m.queue);
            pthread_cond_wait(&m.wait_for_num, &m.lock);
            pthread_mutex_unlock(&m.lock);

        }
        else {
        number = number_array[i];
        worker_q--;
        pthread_cond_signal(&m.queue);
        pthread_cond_wait(&m.wait_for_num, &m.lock);
        pthread_mutex_unlock(&m.lock);
        }
        
        
    }

    pthread_mutex_lock(&m.lock);
    fprintf(stderr, "Main terminating workers\n");
    terminate = 1;

    while (worker_q > 0 ) {
        pthread_cond_signal(&m.queue);
        worker_q--;
    }
    pthread_mutex_unlock(&m.lock);


    for(i = 0; i < num_of_workers; i++) {
        pthread_join(worker_threads[i], NULL);
    }

    fprintf(stderr,"Done,exiting program\n");
    
    return 0;
}

