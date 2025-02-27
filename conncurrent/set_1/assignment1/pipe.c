#include <stdio.h>
#include <stdlib.h>
#include "pipe.h"

//Initialize pipe_array. It will contain the pointers to all the set pipes
static pipe** pipe_array = NULL;
static int id_num = 0;              //position of the pipe in the pipe_array , which is used as its id
static int num_pipes = 0;           //number of pipes (more can be deleted or created)


//Open a new pipe for reading amd writing and return its id to user
int pipe_open(int size) {

    pipe* new_pipe;
    int i;

    //allocate memory for the struct and its ring buffer
    new_pipe = (pipe*)malloc(sizeof(pipe));
    new_pipe->buffer = (char*)malloc(size * sizeof(char));

    //initialize buffer to 0
    for(i = 0; i < size; i++) {
        new_pipe->buffer[i] = '\0';
    }
    
    //set the struct's values
    new_pipe->size = size;
    new_pipe->id = id_num;
    

    //increase id and number of pipes by 1
    id_num++;
    num_pipes++;

    //set pipe open for writing
    new_pipe->write_open = 1;
    
    //set read and write pointers to the beginning of the ring buffer 
    new_pipe->read_pos = -1;
    new_pipe->write_pos = -1;

    //save the pipe's address on the array of pipes
    pipe** new_pipe_array = (pipe**)realloc(pipe_array, sizeof(pipe*)*num_pipes);
    if(new_pipe_array != NULL) {
        new_pipe_array[id_num-1] = new_pipe;
        pipe_array = new_pipe_array;

        //Print all pipe ids for the test
        printf("Pipes: ");
        for(i = 0; i < num_pipes; i++) {
            printf("%d, ", pipe_array[i]->id);
        }
        putchar('\n');
    }
    else {
    fprintf(stderr, "Failed to allocate memory for pipe_array.\n");
    }

    return new_pipe->id;
}

//Find if a pipe with id: p exists. If it exists return a pointer to its address, else return a NULL pointer 
pipe* find_pipe(int p) {
    int i;
    if(num_pipes > 0 ) {
        for (i = 0; i < num_pipes; i++) {
            if(pipe_array[i]->id == p) {
                return pipe_array[i];
            }
        }
    }

    return NULL; 
}

//Delete an existing pipe with id: p
int delete_pipe(int p) {
    int i=0;
    int index;

    //find the pipe to delete
    pipe* pipe_to_del = find_pipe(p);

    if(pipe_to_del != NULL) {

        //find the pipe in the array of pipes and get its index
        for(i=0; pipe_array[i] != pipe_to_del; i++);
        index = i;

        //free the pipe and its ring buffer
        free(pipe_to_del->buffer);
        free(pipe_to_del);

        //shift the elements in the array to avoid empty positions
        for (i=index; i<num_pipes-1;i++){
            pipe_array[i] = pipe_array[i+1];
        }

        num_pipes--;

        //allocate memory for the array of pipes according to the decreased number of pipes
        pipe** new_pipe_array = (pipe**)realloc(pipe_array, sizeof(pipe*)*num_pipes);
        if(new_pipe_array != NULL) {
            pipe_array = new_pipe_array;
        }

        return 1;
    }
    else return 0;
}

// Close the pipe when write is completed. Returns 1 for success and -1 if the pipe was not open for writing
int pipe_writeDone(int p)
{
    pipe *pipe = find_pipe(p);
    if (pipe != NULL && pipe->write_open == 1)
    {
        pipe->write_open = 0;           //set pipe to closed for writing
        return 1;
    }

    return -1;
}

//Check if the thread can read the next byte in the pipe
//returns 1 if read is possible and 0 if the next position has not been written yet
int check_to_read (pipe* pipe) {

    int next_pos;

    //calculate next to be read position in the buffer
    if(pipe->read_pos == pipe->size - 1) {
        next_pos = 0;
    }
    else next_pos = pipe->read_pos + 1;

    //don't read if nothing has been written in the pipe or if the next position is being written
    if (pipe->write_pos == -1 || (pipe->write_open == 1 && pipe->buffer[next_pos] == '\0'))
    {
        return 0;
    }
    else {
        return 1;
    }
        
    return -1;
}

//Read the next byte from the pipe (make it NULL after reading)
//returns 1 if the read operation is successful, 0 if the pipe is closed for writing and empty
//and -1 if the pipe is not open for reading
int pipe_read(int p, char *c) {

    //find the pipe to be read
    pipe *pipe = find_pipe(p);
    if (pipe != NULL && check_to_read(pipe) != -1) {

        printf("\033[32mPipe read: \033[37mpipe id: %d\033[37m,\033[32m read_pos: %d\033[32m\n", pipe->id, pipe->read_pos);

        int next_pos = 0;
        int i;

        // wait if the next position cannot be read yet (spin)
        while (check_to_read(pipe)==0);         

        //calculate the next read position in the buffer
        if(pipe->read_pos == pipe->size - 1) {
            next_pos = 0;
        }
        else next_pos = pipe->read_pos + 1;
        //printf("Next position: %d\nNext poaition char: %c\n", next_pos, pipe->buffer[next_pos]);

        //check if pipe is closed for writing and empty.
        //if yes, delete the pipe and return 0
        if (pipe->write_open == 0 && pipe->buffer[next_pos] == '\0') {
            delete_pipe(pipe->id);
            return 0;
        }
        //if not, read one character from the pipe and remove it from the buffer
        else {
            *c = (pipe->buffer[next_pos]);
            pipe->buffer[next_pos] = '\0';

            pipe->read_pos = next_pos;

            printf("\033[32mTotal pipe buffer contents in read: \033[32m");
            for(i = 0; i < pipe->size; i++) {
                printf("%c", pipe->buffer[i]);
            }
            printf("\n");

            return 1;
        }
    }
    return -1; 
}

//Check if a thread can write in a pipe.
int check_to_write(pipe* pipe) {

    //calculate next position to be written in the buffer
    int next_pos = -1;
    if (pipe->write_pos < pipe->size - 1) {
       next_pos =  pipe->write_pos + 1;
    }
    else {
        next_pos = 0;
    }

    if (pipe->write_open == 1) {

        //check that not yet read positions are not being overwritten
        if (pipe->buffer[next_pos] != '\0') {
            return 0;
        }
        else {
            return 1;
        }
    }
    return -1;
}


//Write the new 1 byte char in the pipe
//return 1 if the write operation s successful and -1 if the pipe is not open for writing
int pipe_write(int p, char c) {
    int i;

    //find the pipe to be written
    pipe *pipe = find_pipe(p);
    if (pipe != NULL && check_to_write(pipe) != -1) {

        //spin if write will cause an overwrite
        while (check_to_write(pipe) == 0);

        //calculate the position where the char will be written and write the char
        if (pipe->write_pos < pipe->size - 1) {
            pipe->write_pos++;
        }
        else {
            pipe->write_pos = 0;
        }
        pipe->buffer[pipe->write_pos] = c;

        printf("\033[37mpipe id: %d,\033[37m\033[33m write_pos %d, character %c\033[33m\n", pipe->id, pipe->write_pos, c);
        printf("\033[33mTotal pipe buffer contents in write: \033[33m");
        for(i = 0; i < pipe->size; i++) {
            printf("%c", pipe->buffer[i]);
        }
        printf("\n");
        return 1;
    }
    
    return -1;
}
