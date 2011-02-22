CC=gcc
CFLAGS+=-fopenmp -g -std=gnu99 -O1


gups: gups.c Makefile
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f gups
