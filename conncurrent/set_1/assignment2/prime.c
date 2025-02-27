#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

//struct to save data usefull for the worker
typedef struct {
    int given_number; //number given from user
    int is_available; //variable to check if the worker is available
    int worker_id; //for debugging
    int terminate; //flag that indicates if the worker should terminate or not
    int notify; //flag that indicates if the worker is notified from the main thread
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
//If all workers are available, then return 1, else return 0
int check_workers_availability(worker* worker_args, int num_of_workers) {
    int i;
    for(i = 0; i < num_of_workers; i++) {
        if(worker_args[i].is_available == 0) {
            return 0;
        }
    }
    return 1;
}
//Spinning function in main. Go through every worker thread in a while loop
//When one of them is available, return its id
int find_available_worker(worker* worker_args, int num_of_workers) {
    int i;
    while(1) {
        for(i = 0; i < num_of_workers; i++) {
            if(worker_args[i].is_available == 1) {
                return i;
            }
        }
    }
}

//The function given to every worker thread
void *worker_op(void *arg) {
    worker *worker_args = (worker *)arg; //Get arguements needed for proper work
    int result;
    worker_args->is_available = 1;


    //Constant loop
    while(1) {
        //If the thread is notified by the master, check if the notice is for termination
        //or for checking a new number to see if it is prime
        if(worker_args->notify == 1) {
            //If you are not asked to terminate, get number from arguements struct and check if it is prime
            if(worker_args->terminate != 1) {
                //Set availability to 0 and change the notification variable back to 0 in order for the master
                //to understand you have received its order and number
                worker_args->is_available = 0;
                worker_args->notify = 0;
                printf("\033[32mWorker with id: %d, checking the number %d: \033[32m", worker_args->worker_id, worker_args->given_number);
                result = is_prime(worker_args->given_number);
                //Print the result taken
                if(result) {
                    printf("\033[31mPrime\033[31m\n");
                }
                else
                    printf("\033[31mNot Prime\033[31m\n");
               // sleep(0.1);
                
                //Change availability back to 1, so the master can see you're availabel
                worker_args->is_available = 1;    
            }
            //If asked to terminate, change your termination variable to 2 in order to notify the master you have terminated
            //and return
            else{
                printf("\033[37mTerminating thread %d\033[37m\n", worker_args->worker_id);
                worker_args->terminate = 2;
                return NULL;
            }
        }
    }
    worker_args->terminate = 2;
    return NULL;
}

//Main function of the program. Here
int main(int argc, char* argv[]) {

    //clock_t start_time = clock();
    int num_of_workers;
    int *number_array;
    int numbers_amount;
    int i;
    int next_num, available_worker, finished_threads;
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

    //Initialize the workers' arguements
    for(i = 0; i < num_of_workers; i++) {
        worker_args[i].given_number = 0;
        worker_args[i].is_available = 0;
        worker_args[i].worker_id = i;
        worker_args[i].terminate = 0;
        worker_args[i].notify = 0;
    }
    //Create worker threads
    for(i = 0; i < num_of_workers; i++) {
        if (pthread_create(&worker_threads[i], NULL, worker_op, &worker_args[i]) != 0) {
        fprintf(stderr, "Error creating thread %d.\n", i);
        return 0;;
        }
    }

    i = 0;
    //While we have not checked all the given numbers, wait for a worker to become available
    while(i < numbers_amount) {
        //Set next number to be checked from the array they are saved in
        next_num = number_array[i];
        // while(!check_workers_availability(worker_args,num_of_workers));
        //Wait until you get the id of an available worker
        available_worker = find_available_worker(worker_args, num_of_workers);
        //If the id is -1, then you have an error
        if(available_worker == -1) {
            printf("\033[37mError, available worker not found\033[37m\n");
            return 0;
        }
        //Pass the number to be checked to the available worker adn notify it in order to test it
        printf("\033[35mworker %d is available, passing to it number %d\033[35m\n", available_worker, number_array[i]);
        worker_args[available_worker].given_number = next_num;
        worker_args[available_worker].notify = 1;
        
        //Wait while the worker is becoming unavailable, so the master will not pass many integers to the same
        //worker because the thread did not become unavailable quicker than the availability check was done again
        while(worker_args[available_worker].notify);
        
        i++;
    }
    //Wait / loop untill every thread has stopped working
    printf("\033[33mMaster waiting for workers to stop\033[33m\n");
    while(!check_workers_availability(worker_args, num_of_workers));

    //Notify all threads to shut down
    printf("\033[36mMaster shutting down workers\033[36m\n");
    for(i = 0; i < num_of_workers; i++) {
        worker_args[i].terminate = 1;
        worker_args[i].notify = 1;
        printf("\033[36mShut down worker %d\033[36m\n", i);
    }


    //Wait for all the workers to terminate
    while(1){
        finished_threads = 0;
        for(i= 0; i < num_of_workers; i++) {
            if(worker_args[i].terminate == 2) {
                finished_threads++;
            } 
        }
        if(finished_threads == num_of_workers) {
            break;
        }
    }

    //Finish program
    printf("\033[30mDone, exiting program\033[30m\n");

    free(number_array);
    free(worker_args);
    free(worker_threads);

    /*clock_t end_time = clock();

    double final_time = ((double)(end_time-start_time)) / CLOCKS_PER_SEC;
    printf("\033[33mExecution took %f seconds\033[33m\n", final_time);*/

    return 0;
}