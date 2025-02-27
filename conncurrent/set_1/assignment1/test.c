#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pipe.h"

#define FILENAME "input"

typedef struct {
    int pipe1_id;  
    int pipe2_id;  
}pipe_ids;

static int read_complete = 0;           //flag that indicates when the reading is complete
static int program_complete = 0;        //flag that indicates when the program is complete

/*
Function for the first thread.
At first create first pipe and send the contents of given file via the pipe to the other thread.
After all the contents of the file are written to the pipe, close pipe for writing and wait for another pipe to open
Read every character from the second pipe opened by the other thread and save it to a new file .copy2
Compare original file with the second copy. If they are the same, the process is a success 
*/
void *thread1_op(void *arg) {
    pipe_ids *ids = (pipe_ids *)arg;        //arguments of the first thread
    FILE* input_text_file;
    FILE* copy2_text_file;

    //allocate memory for char to be read
    char *data = (char*)malloc(sizeof(char));
    //char to be written
    char curr_c = '\0'; 
    
    input_text_file = fopen(FILENAME, "r"); //Opening the file only for reading

    //Check if the file opened successfully
    if (input_text_file < 0) {
        perror("Thread 1:input.txt cannot open");
    }  

    //While we have not reached the end of the input file, scan each character and write them into the pipe
    do {
        curr_c = fgetc(input_text_file);
        if(curr_c == EOF) {
            break;
        }
        pipe_write(ids->pipe1_id, curr_c);
        //printf("Just put char %c on pipe 1\n", curr_c);
    }
    while(curr_c != EOF);

    fclose(input_text_file); // Closing the input file

    //write completed for the first pipe
    pipe_writeDone(ids->pipe1_id);

    //waiting for the reading to complete (spin)
    while(read_complete == 0);

    //open the copy2 file
    copy2_text_file = fopen(FILENAME ".copy2", "w+");
    if(copy2_text_file < 0) {
        perror("Thread 1:Did not create copy2.txt");
        return NULL;
    }

    //write from the second pipe to the copy2 file
    while(pipe_read(ids->pipe2_id, data) == 1) {
        fprintf(copy2_text_file, "%c", *data);
    }
    fclose(copy2_text_file); 

    //set flag that the program finished and free the data array
    program_complete = 1;
    free(data);

    return NULL;
} 

void *thread2_op(void *arg) {
    pipe_ids *ids = (pipe_ids *)arg;            // arguments of the second thread
    FILE* copy_text_file;

    //allocate memory for char to be read
    char *data = (char*)malloc(sizeof(char));
    char curr_c = '\0';

    //open file for reading and writing
    copy_text_file = fopen(FILENAME ".copy", "w+");
    if(copy_text_file < 0) {
        perror("Thread 2:Did not create copy");
        return NULL;
    }

    //write from the first pipe to the copy file
    while(pipe_read(ids->pipe1_id, data) == 1) {
        fprintf(copy_text_file, "%c", *data);
    }
    fclose(copy_text_file);

    //set flag for read completed
    read_complete = 1;

    copy_text_file = fopen(FILENAME ".copy", "r"); //Opening the file only for reading

    //Check if the file opened
    if (copy_text_file < 0) {
        perror("Thread 2:input.copy cannot open");
    }  

    //While we have not reached the end of the input file, scan each character and write them into the second pipe
    do {
        curr_c = fgetc(copy_text_file);
        if(curr_c == EOF) {
            break;
        }
        pipe_write(ids->pipe2_id, curr_c);
        //printf("Just put char %c on pipe 2\n", curr_c);
    }
    while(curr_c != EOF);

    fclose(copy_text_file); // Closing the input file

    //set flag that write operation is done and free the data array
    pipe_writeDone(ids->pipe2_id);
    free(data);
    return NULL;

} 







int main(int argc, char *argv[]) {

    pthread_t thread1, thread2;
    pipe_ids ids;

    //If the user did not enter a size , print a message to remind him
    if (argc < 2) {
        printf("You have to indicate also the size of the pipe!\n");
        return 1;
    }

    //Indicate the size of the pipe from command line
    int pipe_size = atoi(argv[1]);

    // Opening a new pipes
    ids.pipe1_id = pipe_open(pipe_size);
    ids.pipe2_id = pipe_open(pipe_size);
    
    if ((ids.pipe1_id < 0) && (ids.pipe2_id < 0)) {
        fprintf(stderr, "Pipes did not open.\n");
        return 1;
    }

    // Create the first thread 
    if (pthread_create(&thread1, NULL, thread1_op, &ids) != 0) {
        fprintf(stderr, "Error creating thread 1.\n");
        return 1;
    }

    //Create the second thread
    if (pthread_create(&thread2, NULL, thread2_op, &ids) != 0) {
        fprintf(stderr, "Error creating thread 2.\n");
        return 1;
    }

    // waiting for the threads to finish writing and reading
    while(program_complete == 0);
    printf("Main finished\n");

    return 0;
}
