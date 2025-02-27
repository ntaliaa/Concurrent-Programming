#Set compiler and flags
CC = gcc
CFLAGS = -Wall -g -fsanitize=address
 
#Set the name of the executable file
TARGET = main

#Object files from the .c files
OBJS = test.o pipe.o

#Rule for creating the executable (include -lpthread for pthread library)
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) -lpthread

#Create .o from test file
test.o: test.c pipe.h
	$(CC) $(CFLAGS) -c test.c

#Create .o file from pipe.c file
pipe.o: pipe.c pipe.h
	$(CC) $(CFLAGS) -c pipe.c

clean:
	rm -f $(OBJS) $(TARGET)
