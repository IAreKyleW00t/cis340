CC = gcc
CFLAGS = -I.
DEPS = shade.h
OBJ = main.c shade.c

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

pl: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o *~ pl