
CC = gcc
CFLAGS = -Wall -std=c11
 
all: recife
 
recife: src/main.c
	$(CC) $(CFLAGS) -o recife src/main.c
 
clean:
	rm -f recife