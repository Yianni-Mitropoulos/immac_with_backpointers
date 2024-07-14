# Define compiler and flags
CC = gcc
CFLAGS = -pthread

# Define the executable
EXEC = a.out

# Define the source file
SRC = main.c

# Default target
all: $(EXEC)

# Build the executable
$(EXEC): $(SRC)
	$(CC) $(CFLAGS) -o $(EXEC) $(SRC)

# Phony targets
.PHONY: all clean

# Clean up the build
clean:
	rm -f $(EXEC)
