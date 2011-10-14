#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_THREADS 4

void* say_hello(void *threadID) {
	int tid = *((int*)threadID);
	printf("Hello world from thread %d/%d!\n", tid, NUM_THREADS);
}

int main(int argc, const char * argv[]) {
	pthread_t threads[NUM_THREADS];
	int threadIDs[NUM_THREADS];
	
	for (int i = 0; i < NUM_THREADS; i++) {
		threadIDs[i] = i;
		pthread_create(&threads[i], NULL, say_hello, (void*)&threadIDs[i]);
	}
	
	for (int i = 0; i < NUM_THREADS; i++) {
		pthread_join(threads[i], NULL);
	}
	
	return 0;
}