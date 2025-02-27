#ifndef PIPE_H
#define PIPE_H

//Define FIFO pipe / ring buffer struct used to transfer characters between the threads
typedef struct {
    char* buffer;               //FIFO buffer
    int size;                   //total capacity of buffer in bytes
    int read_pos, write_pos;    //index to read and write postion of buffer
    int id;                     //pipe id
    int write_open;             //flag to indicate if the pipe is open for write
}pipe;

//Open a new pipe for reading and writing and return its id to user
int pipe_open(int size);

////Write the new 1 byte char in the pipe
int pipe_write(int p, char c);

//Close the pipe when write is completed
int pipe_writeDone(int p);

//Read the next byte from the pipe (make it NULL after reading)
int pipe_read(int p, char *c);

#endif