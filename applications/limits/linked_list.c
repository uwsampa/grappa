
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <time.h>



#define BITS 26
#define SIZE (1 << BITS)

typedef struct node {
  uint64_t id;
  struct node* next;
  char padding[48];
} node;

void print( node nodes[] ) {
  printf("[\n");
  uint64_t i = 0;
  for( i = 0; i < SIZE; ++i ) {
    printf("{ id: %lu next: %p }\n", nodes[i].id, nodes[i].next);
  }
  printf("]\n");
}

//void initshuffle(node nodes[], node* locs[]) {
void initshuffle(node nodes[] ) {//, node* locs[]) {
  int64_t i = 0;
  node* locs[SIZE] = { NULL };
  // initialize ids
  #pragma omp parallel for
  for(i = 0; i < SIZE; ++i) {
    nodes[i].id = i;
  }

  // shuffle
  for(i = SIZE-1; i > 0; --i) {
    uint64_t j = random() % i;
    node temp = nodes[i];
    nodes[i] = nodes[j];
    nodes[j] = temp;
  }
  
  // extract locs
  #pragma omp parallel for
  for(i = 0; i < SIZE; ++i) {
    locs[ nodes[i].id ] = &nodes[i];
  }

  // initialize pointers
  #pragma omp parallel for
  for(i = 0; i < SIZE; ++i) {
    uint64_t nextid = nodes[i].id + 1;
    nodes[i].next = locs[ nextid % SIZE ];
  }
}



uint64_t walk( node nodes[], uint64_t count ) {
  uint64_t sum = 0;
  node* i = &nodes[0];

  //printf("Running on thread %d....\n", omp_get_thread_num());

  while (count > 0) {
    sum += i->id;
    i = i->next;
    count--;
  }

  return sum;
}



//node* locs[SIZE] = { NULL };
node data[SIZE] = { 0, NULL };

int main(int argc, char* argv[]) {

  printf("Initializing footprint of size %d * %lu = %lu bytes....\n", SIZE, sizeof(node), SIZE * sizeof(node));
  // initialize random number generator
  srandom(time(NULL));

  //print( data );

  // initialize array
  initshuffle( data );//, locs );

  //print( data );
  int n;
  for(n = 0; n < 10; n++) {
  struct timespec start;
  struct timespec end;
  clock_gettime(CLOCK_MONOTONIC, &start);
  //printf("Start time: %d seconds, %d nanoseconds\n", (int) start.tv_sec, (int) start.tv_nsec);

#define NPROC 16

  int m;
  uint64_t base = 0; 
  const uint64_t procsize = SIZE / NPROC;
  const uint64_t multiplier = 10;
  uint64_t results[NPROC] = { 0 };
  #pragma omp parallel for
  for(m = 0; m < NPROC; m++) {
    node* d = &data[0];
    results[m] = walk( d + (m*procsize), procsize*multiplier );
  }

  clock_gettime(CLOCK_MONOTONIC, &end);
  //printf("End time: %d seconds, %d nanoseconds\n", (int) end.tv_sec, (int) end.tv_nsec);

  uint64_t result = 0;
  for(m = 0; m < NPROC; m++) {
    result += results[m];
  }

  const uint64_t totalsize = multiplier * procsize * NPROC;
  uint64_t runtime_ns = ((uint64_t) end.tv_sec * 1000000000 + end.tv_nsec) - ((uint64_t) start.tv_sec * 1000000000 + start.tv_nsec);
  printf("Result: %lu; Runtime: %lu nanoseconds, %d requests, %f req/s, %f ns/req, %f B/s\n", 
	 result, runtime_ns, totalsize, (double)totalsize/((double)runtime_ns/1000000000), 
	 (double)runtime_ns/totalsize, (double)totalsize*sizeof(node)/((double)runtime_ns/1000000000));
  }
  //print( data );

  return 0;
}
