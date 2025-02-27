#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

static int cars_left = 0; //cars waiting on bridge's left
static int cars_right = 0; //cars waiting on bridge's right
static int cars_on_bridge = 0; //cars on bridge currently
static int cars_to_cross = 0; //cars that have at least started crossing the bridge during this direction/round configuration
static int cars_crossed = 0; //cars that have crossed the bridge during this direction/round configuration

static int num_of_cars = 0; //total number of cars that will enter and exit the bridge
static int max_cars = 0; //max cars allowed on bridge at any time
static int bridge_direction = -1; //0 left , 1 right

static int num_for_change_direction = 0; //max number of cars to pass the bridge before the direction changes automatically
static int check =0; //flag used to stop cars from calling in others when the bridge is full
static double bridge_crossing_time = 0; //time it takes a car to cross the bridge


//static mysem_t mtx; //semaphore used as mutex in order to read from and write to every counter in the program
//static mysem_t left_free_space; //binary semaphore used to block and unblock cars trying to enter the bridge from the left
//static mysem_t right_free_space; //same as above but for the cars from the right


typedef struct{
    pthread_mutex_t mtx;
    pthread_cond_t left_free_space;
    pthread_cond_t right_free_space;
}monitor;

static monitor m;

//arguements struct for cars
typedef struct{
    int id;
    int direction;
}car_args;

//if there is a car waiting behind the car currently entering the bridge, check if you can let it enter the bridge too
void check_for_next_car(int direction, int id, car_args* car) {
    
    int cars_queue = 0;

    if (direction == 0)
    {
        cars_queue = cars_left;
    }
    else
    {
        cars_queue = cars_right;
    }
    //i still have cars from my direction and the max number of them t opass before changing sides has not been reached 
    if (cars_queue != 0 && cars_to_cross+1<=num_for_change_direction && cars_on_bridge < max_cars) { 

        if (direction == 0)
        {
            fprintf(stderr,"Car %d opening the way for the next car from \33[31mLEFT\33[37m\n", id);
            //mysem_up(&left_free_space);
            pthread_cond_signal(&m.left_free_space);
        }
        else
        {
            fprintf(stderr,"Car %d opening the way for the next car from \33[36mRIGHT\33[37m\n", id);
            pthread_cond_signal(&m.right_free_space);
        }
        
    } 
    
    if (cars_to_cross < num_for_change_direction && cars_on_bridge < max_cars && check== 1) {
        check= 0;
    }
}

//function of car when it enters the bridge
void enter_bridge(int direction, int id, car_args* car) {
    int cars_queue = 0;

    pthread_mutex_lock(&m.mtx);
    //if bridge and sides are empty, enter bridge and set direction
    if (bridge_direction == -1) {
        bridge_direction=direction;

        if (direction==0) {
            fprintf(stderr, "\33[33mCar %d enters empty bridge from \33[31mLEFT\33[33m and sets the bridge's direction as its own\n\33[37m", id);
            //pthread_cond_signal(&m.left_free_space);
            //pthread_cond_wait(&m.left_free_space,&m.mtx);
            
        }
        else {
            fprintf(stderr, "\33[33mCar %d enters empty bridge from \33[36mRIGHT\33[33m and sets the bridge's direction as its own\n\33[37m", id);
           // pthread_cond_signal(&m.right_free_space);
           // pthread_cond_wait(&m.right_free_space,&m.mtx);
        }
    }
    
    
    //get number of cars on current car's side
    if (direction == 0)
    {
        cars_queue = cars_left;
    }
    else
    {
        cars_queue = cars_right;
    }

    //if the car is going in the same direction as the bridge has been set, enter this block
    if (bridge_direction == direction) {
        //immediately enter the bridge if the if condition is met
        if (cars_queue == 0 && cars_on_bridge < max_cars && cars_to_cross < num_for_change_direction) {
            
            cars_on_bridge++;
            cars_to_cross++;
            if(direction == 0) {
                fprintf(stderr, "\33[31m%d immediately enters bridge from LEFT, %d cars on bridge\n\33[37m", id, cars_on_bridge);
            }
            else {
                fprintf(stderr, "\33[36m%d immediately enters bridge from RIGHT, %d cars on bridge\n\33[37m", id, cars_on_bridge);
            }
            
            check_for_next_car(direction, id,car);
            if (cars_on_bridge >= max_cars) {
                check= 1;
            }
           // mysem_up(&mtx);
        }
        //else enter the queue of the side the car is coming from and wait to be let in the bridge
        else {
            if (direction == 0)
            {
                fprintf(stderr, "\33[31mCar %d waits behind a queue of %d cars on the LEFT side\n\33[37m", id, cars_left);
                cars_left++;
               // mysem_up(&mtx);
                pthread_cond_wait(&m.left_free_space,&m.mtx);
                
            }
            else
            {
                fprintf(stderr, "\33[36mCar %d waits behind a queue of %d cars on the RIGHT side\n\33[37m", id, cars_right);
                cars_right++;
                
                //mysem_up(&mtx);
                pthread_cond_wait(&m.right_free_space,&m.mtx);
            }
            //after being let in change appropriately the variables inside the mutex
           // mysem_down(&mtx);
            
            cars_on_bridge++;
            cars_to_cross++;
            //and enter bridge
            if (direction == 0)
            {
                fprintf(stderr, "\33[31mCar %d enters bridge after waiting with %d cars on bridge from LEFT\n\33[37m", id, cars_on_bridge);
                cars_left--;
                check_for_next_car(direction, id, car);
                    if (cars_on_bridge >= max_cars) {
                        check= 1;
                    }
               // mysem_up(&mtx);
            }
            else
            {
                fprintf(stderr, "\33[36mCar %d enters bridge after waiting with %d cars on bridge from RIGHT\n\33[37m", id, cars_on_bridge);

                cars_right--;
                check_for_next_car(direction, id, car);
                    if (cars_on_bridge >= max_cars) {
                        check= 1;
                    }
               // mysem_up(&mtx);
            }
            
            

        }
    }
    //if the car's direction is the opposite of the current one
    else {
        //enter your correct queue
        if (direction == 0)
        {
            fprintf(stderr, "\33[31mCar %d is opposite of current side passing through the bridge: will wait on the LEFT behind %d others\n\33[37m", id, cars_left);
            cars_left++;
           // mysem_up(&mtx);
            pthread_cond_wait(&m.left_free_space,&m.mtx);
        }
        else
        {
            fprintf(stderr, "\33[36mCar %d is opposite of current side passing through the bridge: will wait from the RIGHT behind %d others\n\33[37m", id, cars_right);
            cars_right++;
           // mysem_up(&mtx);
            pthread_cond_wait(&m.right_free_space,&m.mtx);
        }
        //then enter bridge
       // mysem_down(&mtx);

        cars_on_bridge++;
        cars_to_cross++;

        if (direction == 0)
        {
            fprintf(stderr, "\33[31mCar %d enters bridge after waiting with %d cars on bridge from LEFT\n\33[37m", id, cars_on_bridge);
            cars_left--;
            check_for_next_car(direction, id, car);
                if (cars_on_bridge >= max_cars) {
                    check= 1;
                }
           // mysem_up(&mtx);
        }
        else
        {
            fprintf(stderr, "\33[36mCar %d enters bridge after waiting with %d cars on bridge from RIGHT\n\33[37m", id, cars_on_bridge);
            cars_right--;
            check_for_next_car(direction, id, car);
                if (cars_on_bridge >= max_cars) {
                    check= 1;
                }
           // mysem_up(&mtx);
        }
    }
    pthread_mutex_unlock(&m.mtx);
}

//function when a car exits the bridge
void exit_bridge(int direction, int id, car_args* car){
    int curr_side = 0;
    int other_side = 0;

    //mysem_down(&mtx);
    pthread_mutex_lock(&m.mtx);

    cars_crossed++;
    cars_on_bridge--;

    //get values needed in mtx
    if(direction==0){ 
        curr_side = cars_left;
        other_side = cars_right;
    }
    else{
        curr_side = cars_right;
        other_side = cars_left;
    }
    //cars from my direction still exist in queue and i haven't yet reached the limit for cars before changing direction
    if ((curr_side != 0 && cars_to_cross < num_for_change_direction)) { 
        fprintf(stderr, "\33[37mCar %d exiting bridge without changing direction. %d cars have crossed, %d cars on bridge\n\33[37m", id, cars_crossed, cars_on_bridge);
        if(check== 1) {
            if(direction == 0) {
                fprintf(stderr, "\33[37mNext car from the \33[31mLEFT\33[37m is allowed to enter as the current car number that has at least started crossing is %d\n\33[37m", cars_to_cross);

                pthread_cond_signal(&m.left_free_space);
            }
            else {
                fprintf(stderr, "\33[37mNext car from the \33[36mRIGHT\33[37m is allowed to enter as the curr car number that has at least started crossing is%d\n\33[37m", cars_to_cross);
                pthread_cond_signal(&m.right_free_space);
            }
            check=0;
        }
        
        //mysem_up(&mtx);
    }
    //if bridge is empty and opposite side has at least 1 car, change direction when exiting
    else if ((cars_on_bridge == 0 && other_side > 0))
    {
        fprintf(stderr, "\33[33mCar %d changes direction from ", id);
        if(bridge_direction == 0) {
            fprintf(stderr, "\33[31mLEFT\33[37m \33[33mto \33[36mRIGHT\33[33m ");
        }
        else {
            fprintf(stderr, "\33[36mRIGHT\33[37m \33[33mto \33[31mLEFT\33[33m ");
        }
        fprintf(stderr, "while exiting\n%d remaining cars on its side and %d cars on the opposite\n\33[37m", curr_side, other_side);
  
        bridge_direction = !(direction);

        cars_to_cross = 0;
        cars_crossed = 0;
        

        if (direction == 0)
        {
            // fprintf(stderr, "Car with ID: %d about to raise semaphore for right cars\n", id);
            pthread_cond_signal(&m.right_free_space); // up otan feugei amaksaki apo to bridge
        }
        else
        {
            // fprintf(stderr, "Car with ID: %d about to raise semaphore for left cars\n", id);
            pthread_cond_signal(&m.left_free_space);
        }
        //mysem_up(&mtx);
    }
    //if both sides and the bridge are empty, reset the bridge's direction to nothing so that it can be set by the first car
    else if(cars_on_bridge == 0 && other_side == 0 && curr_side == 0) {
        cars_to_cross = 0;
        cars_crossed = 0;
        bridge_direction = -1;
        fprintf(stderr, "\33[33mCar %d exits, bridge and sides are completely empty, bridge direction is reset to wait for next car\n\33[37m", id);
        //mysem_up(&mtx);
    } 
    //if there are cars only on current side, reset counters and keep going to avoid deadlock
    else if(cars_on_bridge == 0 && other_side == 0 && curr_side != 0) {
        cars_to_cross = 0;
        cars_crossed = 0;
        bridge_direction = direction;
        fprintf(stderr, "\33[33mCar %d exits reaching limit that unblocks opposite side(no starvation)\nNo cars opposite, %d cars on %d's side\n%d resets counters and cars can keep coming from ", id, curr_side, id, id);
        
        if(direction == 0) {
            fprintf(stderr, "\33[31mLEFT\n\33[37m");
            pthread_cond_signal(&m.left_free_space);
        }
        else {
            fprintf(stderr, "\33[36mRIGHT\n\33[37m");
            pthread_cond_signal(&m.right_free_space);
        }
        //mysem_up(&mtx);
    }
    //in every other case, just exit
    else {
        fprintf(stderr, "\33[37mCar %d exiting bridge without changing direction. %d cars have crossed, %d cars on bridge\n\33[37m", id, cars_crossed, cars_on_bridge);

        //mysem_up(&mtx);
    }
    pthread_mutex_unlock(&m.mtx);
}
//make sleep work with decimals(doubles)
void sleep_double(double seconds) {
    int microseconds = (int)(seconds * 1e6); // Convert seconds to microseconds
    usleep(microseconds);
}

//car function, enter bridge, cross it and exit
void *car_op(void *arg){
    car_args *car_data = (car_args*)arg;

    enter_bridge(car_data->direction, car_data->id, car_data);

    sleep_double(bridge_crossing_time);

    exit_bridge(car_data->direction, car_data->id, car_data);

    return NULL;

}




int main(int argc,char* argv[]){

    pthread_t *cars;
    int i;
    double interval_car_arrival_time = 0;

    car_args *car_data;
    //mysem_init(&mtx,1);
    //mysem_init(&left_free_space,0);
    //mysem_init(&right_free_space,0);

    pthread_mutex_init(&m.mtx,NULL);
    pthread_cond_init(&m.left_free_space,NULL);
    pthread_cond_init(&m.right_free_space,NULL);

    printf("Insert the asked numbers asked with a space between them in this correct order:\nNumber of cars(int)\nMax cars on bridge(int)\nTime between arrival of each car(double)\nTime for a car to cross the bridge(double)\nMax cars to pass before changing sides(int)\n");

    if (scanf("%d %d %lf %lf %d", &num_of_cars, &max_cars, &interval_car_arrival_time, &bridge_crossing_time, &num_for_change_direction) != 5) {
        printf("Error: Invalid input. Please enter values in the correct format.\n");
        return 0;
    } 

    car_data = (car_args*)malloc(sizeof(car_args)*num_of_cars);
    cars = (pthread_t*)malloc(sizeof(pthread_t)*num_of_cars);

    srand(time(NULL)); 


    for(i = 0; i < num_of_cars; i++) {
        sleep_double(interval_car_arrival_time);
        car_data[i].id = i;


        car_data[i].direction = rand() % 2;

        if (pthread_create(&cars[i], NULL, car_op, &car_data[i]) != 0) {
        fprintf(stderr, "Error creating thread %d.\n", i);
        return 0;
        }
        
    }

    for (i=0; i<num_of_cars; i++) {
        pthread_join(cars[i], NULL);
    }
    free(car_data);
    free(cars);

    return 0;
}