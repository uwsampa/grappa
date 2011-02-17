CC=gcc
CFLAGS+=-fopenmp -g -std=gnu99 


gups.exe: gups.c Makefile
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f gups.exe
