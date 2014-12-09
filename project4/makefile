CC = gcc
CFLAGS = -std=gnu99 -Wall -g -I.

all: client server

client: client.c network.c command.h network.h Fat12Boot.h Fat12Entry.h
	$(CC) -o $@ $^ $(CFLAGS)

server: server.c command.h
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean
clean:
	rm -f *.o *~ client server
