#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static char* input_file_name; //string  to store the name of the file

    // Arguments passed from parent thread to children
    typedef struct
{
    int left;               //left position of the section
    int right;              //right position of the section
    int size_of_section;    //size of the section
    int comm_flag;          //flag to indicate if the thread is done or is 0 if it is not done
} thread_data;

//quicksort swap
void swap(int* a, int* b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}
//quicksort partition
int partition(int arr[], int low, int high) {

    // Initialize pivot to be the first element
    int p = arr[low];
    int i = low;
    int j = high;

    while (i < j) {

        // Find the first element greater than
        // the pivot (from starting)
        while (arr[i] <= p && i <= high - 1) {
            i++;
        }

        // Find the first element smaller than
        // the pivot (from last)
        while (arr[j] > p && j >= low + 1) {
            j--;
        }
        if (i < j) {
            swap(&arr[i], &arr[j]);
        }
    }
    swap(&arr[low], &arr[j]);
    return j;
}
//quicksort used when 64 ints RAM is available
void quickSort(int arr[], int low, int high) {
    int index;
    if (low < high) {

        // call partition function to find Partition Index
        index = partition(arr, low, high);

        // Recursively call quickSort() for left and right
        // half based on Partition Index
        quickSort(arr, low, index - 1);
        quickSort(arr, index + 1, high);
    }
}

//Merge function
int merge(thread_data curr){

    int left;  // left position
    int mid;   // mid position
    int right; //right position
    int pos;   //position initialized to mid and moves
    int left_value,right_value, temp, value; //variables to keep the integers and to comparisons
    FILE *input_file; 
    int i=0;   //counter

    //initialize the positions with the data of the thread
    left = curr.left; 
    mid = curr.left + ceil((curr.right - curr.left)/2)+1;
    pos = mid;
    right = curr.right;

    //opening the file
    input_file = fopen(input_file_name, "rb+");
    if (!input_file)
    {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }
    temp = 0;

    while(left < mid && pos<=right){
        fseek(input_file, left * sizeof(int), SEEK_SET); // Move to the correct position
        fread(&left_value, sizeof(int), 1, input_file);        // Read the left integer

        fseek(input_file, pos * sizeof(int), SEEK_SET); // Move to the correct position
        fread(&right_value, sizeof(int), 1, input_file);        // Read the right integer

        if(left_value<=right_value){

            left++;
        }
        else{
           
            temp = right_value; //keep the right_value to an new variable
           
            //shift the integers in the file
            for (i = mid; i > left; i--) {
                fseek(input_file, (i-1) * sizeof(int), SEEK_SET);  // Move to the correct position
                fread(&value, sizeof(int), 1, input_file); // Read the integer

                fseek(input_file, i * sizeof(int), SEEK_SET); // Move to the correct position
                fwrite(&value, sizeof(int), 1, input_file);   //Write the integer
                
            }
            fseek(input_file, left * sizeof(int), SEEK_SET); // Move to the correct position
            fwrite(&temp, sizeof(int), 1, input_file); //Write the integer
    
            //adding one to the positions to check the next integers
            left++;
            pos++;
            mid++;
        }
  
    }
    fclose(input_file);

    return 1;
}

//Thread function
void* thread_child(void* arg){

    thread_data *curr =(thread_data*)arg;
    int mid=0;
    thread_data left_child_data;
    thread_data right_child_data;
    pthread_t left_child;
    pthread_t right_child;
    int *array;
    FILE* input_file;

    //If you have to handle more than 64 integers on this file's section, split into 2 thread children
    if(curr->size_of_section>64){
        //Create data and pass it to children
        mid = curr->left + (curr->right - curr->left)/2;

        left_child_data.left = curr->left;

        left_child_data.right = mid;

        right_child_data.left = mid+1;

        right_child_data.right = curr->right;

        left_child_data.size_of_section = mid - curr->left +1;

        right_child_data.size_of_section = curr->right - (mid+1) +1;

        left_child_data.comm_flag = 0;

        right_child_data.comm_flag = 0;

        //Create 2 children 
        if(pthread_create(&right_child, NULL, thread_child, &right_child_data)!=0){
            fprintf(stderr, "Error creating thread \n");
            return 0;
        }

        if(pthread_create(&left_child, NULL, thread_child, &left_child_data)!=0){
            fprintf(stderr, "Error creating thread.\n");
            return 0;
        }

        //Actively wait for the current thread's children threads to finish sorting file
        while(right_child_data.comm_flag == 0 || left_child_data.comm_flag == 0);
        printf("\033[31mParent returning with child left %d %d, child right %d %d\033[31m\n", left_child_data.left, left_child_data.right, right_child_data.left, right_child_data.right);

        //call the merge function so the parrent merge as the returns are happening
        merge(*curr);
        printf("\033[36mMerge returned from %d to %d\033[36m\n", curr->left, curr->right);

        // Set flag to 1 so the parent thread stops waiting and return
        curr->comm_flag = 1;
    }
    //Perform quicksort on the <=64 integers viewed by the branch on the file and return
    else{
        //Open file
        input_file = fopen(input_file_name, "rb+");
        if (!input_file)
        {
            perror("Failed to open file");
            exit(EXIT_FAILURE);
        }

        //Allocate memory for the array that the thread will use to perform quicksort and pass the file's current
        //section to it, then sort
        array = malloc(curr->size_of_section * sizeof(int));
        fseek(input_file, curr->left * sizeof(int), SEEK_SET);
        fread(array, sizeof(int), curr->size_of_section, input_file);

        quickSort(array,0,curr->size_of_section -1);
        printf("\033[32mSorted array after quicksort: \033[32m");
        for(int i = 0; i < curr->size_of_section; i++  ){
            printf("\033[32m%d \033[32m", array[i]);
        }
        printf("\n");

        //Write sorted section back to the file
        fseek(input_file, curr->left * sizeof(int), SEEK_SET);

        fwrite(array, sizeof(int), curr->size_of_section, input_file);
        fclose(input_file);

        free(array);

        curr->comm_flag = 1; //indicates that the thread is finished
        printf("\033[33mChild finished with left, right and size: %d, %d, %d \033[33m\n", curr->left, curr->right, curr->size_of_section);

        return NULL;

    }

    return NULL;
}

//Print file contents
void print_file(FILE *filename) {
    int number;

    if (filename == NULL) {
        printf("Error opening file!\n");
        return;
    }
    printf("\033[35mPRINTING FINAL FILE NUMBERS\033[35m\n");
    // Read and print each integer from the file
    while (fread(&number, sizeof(int), 1, filename)) {
        printf("\033[35m%d \033[35m", number);
    }
    return;
}


int main(int argc, char *argv[])
{

    FILE *input_file;
    pthread_t root; //thread for the root
    thread_data data;
    int num_of_ints; //number of integers in file
    long file_size;

    // Check if the filename is provided
    if (argc < 2)
    {
        fprintf(stderr, "No filename provided\n");
        exit(EXIT_FAILURE);
    }

    // Open the file in binary mode
    input_file = fopen(argv[1], "rb+");

    // input_file_name = (char*)malloc(strlen(argv[1]) *sizeof(char));
    input_file_name = argv[1];

    // Check if the file opened successfully
    if (!input_file)
    {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    // Get the file's size
    fseek(input_file, 0, SEEK_END);
    file_size = (int)ftell(input_file);
    rewind(input_file);

    // Calculate number of integers in the file
    num_of_ints = file_size / sizeof(int);


    fclose(input_file);

    //initialize data for the root thread
    data.left = 0;
    data.right = num_of_ints-1;
    data.size_of_section=num_of_ints;
    data.comm_flag = 0;

    //creating thread for the root
    pthread_create(&root, NULL, thread_child, &data);

    printf("Main waiting for children to finish\n");

    //main waiting for the all the threads to finish
    while(data.comm_flag == 0);

    printf("\033[35mMain finished\033[35m\n");

    // Open the .bin file for reading in binary mode
    input_file = fopen(input_file_name, "rb");
    print_file(input_file);

    // Close the file
    fclose(input_file);

    input_file_name = '\0';

    return 0;
}