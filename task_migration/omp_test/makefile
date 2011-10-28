CFLAGS += -fopenmp

BIN += bin

all: integrate hello

hello: omp_hello.c
	gcc $(CFLAGS) -o $(BIN)/$@ $^

integrate: omp_integrate.c
	gcc $(CFLAGS) -o $(BIN)/$@ $^

clean:
	rm -f $(BIN)/*

