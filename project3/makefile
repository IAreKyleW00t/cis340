CC = gcc
CFLAGS = -std=gnu99 -Wall -g -I.
DEPS = process_management.h boot.h Fat12Boot.h Fat12Entry.h
OBJ = main.c process_management.c boot.c

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

proj3: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

run:
	./proj3

.PHONY: clean
clean:
	rm -f *.o *~ proj3
