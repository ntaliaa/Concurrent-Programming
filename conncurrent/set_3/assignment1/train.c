#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "library.h"

static mysem_t start_train;             //semaphore for train to wait until all passengers enter
static mysem_t start_exit;              //semaphore for passengers to wait until the train ride ends to start exiting the train
static mysem_t ready_to_board;          //semaphore for passengers to wait before entering the train until all previous passengers exit
static mysem_t mtx;                     //semaphore for mutual exclusion
static int capacity;                    //train capacity
static double interval_passengers;      //time interval between passenger arrival (user input)
static double ride_duration;            //ride duration (user input)
static int passenger_counter;           //number of passengers on the train
static int queue_to_board=0;            //number of passengers waiting in queue to board the train
static int queue_to_exit=0;             //number of passengers waiting to exit the train
static int exit_in_progress=0;          //flag (1 when exit is in progress) for passengers block 
static int num_of_passengers;           //total number of passengers (user input)
static int remaining_passengers;        //number of remaining passenger to enter the train       
static int last_ride=0;                 //flag for last ride (1 when not enough remaining passengers for a new ride)


typedef struct{
    int id;              //passenger id
    int no_entry;        //flag for cars that should not enter or exit the train (not enough for a new ride)
}passenger_args;

//sleep function for double values
void sleep_double(double seconds) {
    int microseconds = (int)(seconds * 1e6); // Convert seconds to microseconds
    usleep(microseconds);
}

//entry function for passenger
//enters only if a new ride is possible, waits in queue until the train is waiting and empty
void entry_op(int id, passenger_args* passenger_data) {

    mysem_down(&mtx);
    
    //don't enter if not enough remaining passengers for a new ride
    if (last_ride == 1) {
        passenger_data->no_entry = 1;
        mysem_up(&mtx);
        return;
    }

    //wait if the train is full or other passengers are already waiting in queue or passengers are still exiting
    if(passenger_counter >= capacity || queue_to_board > 0 || exit_in_progress == 1){

        queue_to_board++;                   //increase queue counter

        fprintf(stderr, "\33[35mPassenger %d entered queue of %d\33[37m\n",id, queue_to_board);
        mysem_up(&mtx);
        mysem_down(&ready_to_board);        //drop semaphore for passenger to wait in queue
        mysem_down(&mtx);
        queue_to_board--;                   //passenger exits queue and boards
    }
    
    //passenger boards train
    passenger_counter++;
    remaining_passengers--;
    
    fprintf(stderr,"\33[32mPassenger %d entered train with %d passengers on board and %d remaining passengers\33[37m\n", id, passenger_counter, remaining_passengers);

    //if train is full start the train
    if(passenger_counter==capacity) { 

        fprintf(stderr,"\33[37m%d passengers entered, entry is completed\33[37m\n",passenger_counter);
        
        //if less passenger will arrive than the train's capacity set last ride flag to 1 (this is the last ride)
        if (remaining_passengers < capacity) {
            last_ride=1;
            fprintf(stderr,"\33[33mNot enough passengers for new ride, train should exit\33[37m\n");
        }
        mysem_up(&start_train);     //raise semaphore to start ttrain ride
        
    }
    //if the train is not full and there are passengers in queue raise semaphore for entry
    else if(queue_to_board>0){
        mysem_up(&ready_to_board);
    }
    mysem_up(&mtx);
}

//exit function for passenger
//exits train if the ride has finished 
void exit_op(int id, passenger_args* passeger_data) {

    mysem_down(&mtx);

    //if this is the first passenger exiting set flag to 1
    if (passenger_counter == capacity) {
        exit_in_progress = 1;
    }

    //terminate passengers if they didn't enter the train
    if (passeger_data->no_entry == 1) {
        fprintf(stderr, "\33[36mPassenger %d didn't enter the train (not enough passengers for full ride). Immediate return.\33[37m\n", id);
        mysem_up(&mtx);
        return;
    }
   
    //passenger in queue to exit
    queue_to_exit++;
    mysem_up(&mtx);
    mysem_down(&start_exit);        //raise semaphore for passenger to wait for train to finish
    mysem_down(&mtx);

    //passengers exit
    if(passenger_counter>0){
        passenger_counter--;
        queue_to_exit--;
        fprintf(stderr,"\33[31mPassenger %d exits the train, remaining passengers to exit are %d\33[37m\n",id, passenger_counter);
    }

    //last passenger rasies semaphore for new passengers to enter train
    if(passenger_counter==0 && last_ride == 0){
        passenger_counter=0;
        exit_in_progress=0;
        fprintf(stderr,"\33[37mLast passenger with id %d exited the train with %d remaining passengers on board. New passengers can enter.\33[37m\n", id, passenger_counter);
        mysem_up(&ready_to_board);
    }
    //if last ride raises semaphore to start train that was waiting for exit to end
    else if (passenger_counter == 0 && last_ride==1) {
        fprintf(stderr,"\33[37mLast passenger with id %d from the final full ride has exited the train\33[37m\n", id);
        mysem_up(&start_train);  
    }
    //raise semaphore for next passenger to exit
    else if(queue_to_exit>0){
        mysem_up(&start_exit);
    }

    mysem_up(&mtx);  
    return;
}


//passeger function to enter and exit train
void *passenger_op(void *arg){
    passenger_args *passenger_data = (passenger_args*)arg;

    entry_op(passenger_data->id, passenger_data);
    
    exit_op(passenger_data->id, passenger_data);
    
return NULL;
}

//train function
//waits for boarding to complete, repeats until no remaining passengers
void *train_op(void *arg){

    while(1){
        
        //wait until train is full
        mysem_down(&start_train);

        mysem_down(&mtx);
        fprintf(stderr,"\33[34mThe train started with %d passengers\33[37m\n", passenger_counter);
        mysem_up(&mtx);

        //sleep for duration of ride
        sleep_double(ride_duration);

        mysem_down(&mtx);
        exit_in_progress=1;
        mysem_up(&mtx);

        //raise semaphore for passengers to exit
        mysem_up(&start_exit); 

        fprintf(stderr,"\33[34mThe train completed the ride\33[37m\n");

        //wait for passengers to exit and exit loop (train stops) if last ride
        mysem_down(&mtx);
        if(last_ride==1) {

            fprintf(stderr,"\33[34mTrain completed last ride, waits for passengers to exit\33[37m\n");
            mysem_up(&mtx);
        
            mysem_down(&start_train);
            fprintf(stderr,"\33[37mAll passengers exited after the last ride. Train stops.\33[37m\n");
            break;
        }
        else {
            mysem_up(&mtx);
        }
        
    }

return NULL;
}



int main(int argc,char* argv[]){

    pthread_t train;
    pthread_t* passengers;
    passenger_args *passenger_data;

    int i=0;

    //initialize semaphores
    mysem_init(&start_train,0);
    mysem_init(&start_exit,0);
    mysem_init(&ready_to_board,0);
    mysem_init(&mtx,1);

    //get values from user input
    capacity = atoi(argv[1]);
    num_of_passengers = atoi(argv[2]);
    ride_duration = atof(argv[3]);
    interval_passengers = atof(argv[4]);
    remaining_passengers = num_of_passengers;

    //allocate memory for threads
    passengers = (pthread_t*)malloc(sizeof(pthread_t)*num_of_passengers);
    passenger_data = (passenger_args*)malloc(sizeof(passenger_args)*num_of_passengers);
    
    //if not enough passengers for full train return
    if (capacity > num_of_passengers) {
        fprintf(stderr, "\33[34mThe train will not start because there are not enough passengers for a full ride\33[37m\n");
    }
    else {
        //create train thread
        if (pthread_create(&train, NULL, train_op, NULL) != 0) {
            fprintf(stderr, "Error creating train thread %d.\n", i);
            return 0;
        }

        //create passenger threads
        for(i = 0; i < num_of_passengers; i++) {
            sleep_double(interval_passengers);

            passenger_data[i].id = i;
            passenger_data[i].no_entry =0;
            
            if (pthread_create(&passengers[i], NULL, passenger_op, &passenger_data[i]) != 0) {
            fprintf(stderr, "Error creating passenger thread %d.\n", i);
            return 0;
            }
        }

        for (int i = 0; i < num_of_passengers; i++) {
            pthread_join(passengers[i], NULL);
        }

        pthread_join(train, NULL);
    }

   

    return 0;
}

