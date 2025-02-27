# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -g

# Target executable
TARGET = all

# Source files
SOURCES = library.c cars.c

# Object files
OBJECTS = $(SOURCES:.c=.o)

# Rule to build the executable with pthread
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) -lpthread -lm

# Rule to build object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean rule to remove generated files
clean:
	rm -f $(OBJECTS) $(TARGET)

# Include dependency on library.h for object files
library.o: library.h
test1.o: library.h
