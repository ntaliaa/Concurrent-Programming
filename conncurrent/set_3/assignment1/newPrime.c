#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "library.h"

static mysem_t number_taken;            //semaphore for worker to signal the number is assigned
static mysem_t notification_main;       //semaphore for main to give notification (job or termination) to wroker
static mysem_t available_thread;        //semaphore for main to wait for available worker
static mysem_t mtx1;                    //semaphore for mutual exclusion
static mysem_t numbers_checked;         //semaphore for main to wait until all numbers are checked

static int wait_for_available = 0;      //flag for main to signal it's waiting for available worker
static int number;                      //next number to check 
static int terminate;                   //flag to terminate
static int available_exists;            //counter for available threads
static int remaining_numbers;           //counter for remaining numbers to check

//worker struct
typedef struct {
    int worker_id;
    int terminate;
}worker;

// Function to check if a number is prime , returns 1 if it is or else 0
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

//The function given to every worker thread
void *worker_op(void *arg) {
    worker *worker_args = (worker *)arg;    //worker arguments
    int result, num;

    while(1) {
        
        mysem_down(&mtx1);

        //raise semaphore if main is waiting, since the worker is available
        if (wait_for_available == 1) {
            mysem_up(&available_thread);
            wait_for_available = 0;
        }
        available_exists++;

        mysem_up(&mtx1);
        
        mysem_down(&notification_main);     //wait until main notifies for job or termination

        mysem_down(&mtx1);
        available_exists--;


        num = number;                       //get number

        mysem_up(&number_taken);            //raise semaphore to tell main the number is taken
        
            //If you are not asked to terminate, get number from arguments struct and check if it is prime
            if (terminate == 1) {
                mysem_up(&mtx1);
                
                return NULL;
            }
            mysem_up(&mtx1);

            fprintf(stderr,"\033[32mWorker with id: %d, checking the number %d\n\033[32m", worker_args->worker_id, num);

            result = is_prime(num);     //call function to check if number is prime

            //Print the result received
            if(result) {
                printf("\033[31m%d Prime\033[31m\n", num);
            }
            else {
                printf("\033[31m%d Not Prime\033[31m\n", num);
            }

            mysem_down(&mtx1);

            //raise semaphore to tell main all numbers have been checked
            remaining_numbers--;
            if (remaining_numbers == 0) {
                mysem_up(&mtx1);
                mysem_up(&numbers_checked);
            }
            else mysem_up(&mtx1);
        }    
    return NULL;
}


int main(int argc, char* argv[]) {

    mysem_init(&number_taken, 0);
    mysem_init(&notification_main, 0);
    mysem_init(&mtx1, 1);
    mysem_init(&numbers_checked, 0);
    mysem_init(&available_thread, 0);
    
    int num_of_workers;
    int *number_array;
    int numbers_amount;
    int i;
    pthread_t *worker_threads;
    worker *worker_args;
    available_exists = 0;

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
    remaining_numbers = numbers_amount;

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

    //assign each number to a thread to check prime/not prime
    for (i=0; i<numbers_amount; i++) {

        number = number_array[i];       //new number given

        mysem_down(&mtx1);

        //wait if no available threads
        if (available_exists == 0) {

            wait_for_available = 1;             //flag because main is waiting
            mysem_up(&mtx1);
            mysem_down(&available_thread);      //drop semaphore to wait until worker available
        }
        else mysem_up(&mtx1);

        mysem_up(&notification_main);           //raise semaphore for next worker to receive number
        mysem_down(&number_taken);              //wait until worker gets number before giving the next
    }
    

    mysem_down(&numbers_checked);       //wait until all numbers have been checked

    terminate = 1;
    
    //terminate each thread
    for (i = 0; i < num_of_workers; i++)
    {

        mysem_down(&mtx1);

        //wait if no available threads
        if (available_exists == 0) {

            fprintf(stderr, "Terminate\n");

            wait_for_available = 1;            //flag because main is waiting
            mysem_up(&mtx1);
            mysem_down(&available_thread);      //drop semaphore to wait until worker available
        }
        else mysem_up(&mtx1);
        

        mysem_up(&notification_main);           //raise semaphore for next worker to receive number
        mysem_down(&number_taken);              //wait until worker gets termination signal before the next
    }
    
    for(i = 0 ; i < num_of_workers; i++) {
        pthread_join(worker_threads[i], NULL);
    }

    //Finish program
    printf("\033[37mDone, exiting program\033[37m\n");

    return 0;
}
