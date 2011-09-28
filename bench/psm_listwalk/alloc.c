#include "node.h"
#include "alloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>

#include <sys/mman.h>

#include <hugetlbfs.h>

#define DEBUG 0

// initialize linked lists.
//node** allocate_heap(uint64_t size, int num_threads, int num_lists) {
void allocate_heap(uint64_t size, int num_threads, int num_lists, node*** bases_p, node** nodes_p) {
  int64_t i = 0;

  printf("Initializing footprint of size %lu * %lu = %lu bytes....\n", size, sizeof(node), size * sizeof(node));

  //size = size / num_threads;

  //node* nodes = (node*) malloc(sizeof(node) * size);  // temporary node pointers
  if( NULL == *nodes_p ) {
    *nodes_p = (node*) malloc(sizeof(node) * size);
    //*nodes_p = (node*) get_hugepage_region( sizeof(node) * size, GHR_DEFAULT );
  }
  node* nodes = *nodes_p;
  //void* nodes_start = (void*) 0x0000111122220000UL;
  //node* nodes = mmap( nodes_start, sizeof(node) * size, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, 0, 0 );
  //assert( nodes == nodes_start );

  node** locs = (node**) malloc(sizeof(node*) * size); // node array


  if( NULL == *bases_p ) {
    *bases_p = (node**) malloc(sizeof(node*) * num_threads * num_lists); // initial node array
  }
  node** bases = *bases_p;

  // initialize ids
  // #pragma omp parallel for num_threads(num_threads)
  for(i = 0; i < size; ++i) {
#ifdef HAVE_ID
    nodes[i].id = i;
#endif
    nodes[i].next = (node*)i;
  }

  // randomly permute nodes (fisher-yates (or knuth) shuffle)
  for(i = size-1; i > 0; --i) {
    uint64_t j = random() % i;
    node temp = nodes[i];
    nodes[i] = nodes[j];
    nodes[j] = temp;
  }

  // extract locs
  // #pragma omp parallel for num_threads(num_threads)
  for(i = 0; i < size; ++i) {
#ifdef HAVE_ID
    printf("%u %u\n",  nodes[i].id, &nodes[i] - &nodes[0] );
    assert( nodes[i].id == &nodes[i] - &nodes[0] );
    locs[ nodes[i].id ] = &nodes[i];
#else
    locs[ (uint64_t) nodes[i].next ] = &nodes[i];
#endif
  }

  // initialize pointers
  // chop id-space into num_lists sections and build a circular list for each
  //int64_t base = 0;
  // #pragma omp parallel for num_threads(num_threads)
  for(i = 0; i < size; ++i) {
#ifdef HAVE_ID
    uint64_t id = nodes[i].id;
#else
    //assert( nodes[i].id == &nodes[i] - &nodes[0] );
    uint64_t id = i; //&nodes[i] - &nodes[0];
    node* current = locs[i];
#endif
    int64_t thread_size = size / num_lists;
    int64_t base = id / thread_size;
    uint64_t nextid = id + 1;
    uint64_t wrapped_nextid = (nextid % thread_size) + (base * thread_size) % size;
    if( wrapped_nextid == size ) wrapped_nextid = 0;
    //printf("%d: %d %d %d\n", id, base, nextid, wrapped_nextid);
    current->next = locs[ wrapped_nextid ];
  }

  // grab initial pointers for each list
  for(i = 0; i < num_threads * num_lists; ++i) {
    bases[i] = locs[i * size / (num_threads * num_lists)];
  }

  //print(nodes, size);
  
  free(locs);
}

node** allocate_heap2(uint64_t size, int num_threads, int num_lists, node** nodes_ptr) {
  int64_t i = 0;

  printf("Initializing footprint of size %lu * %lu = %lu bytes....\n", size, sizeof(node), size * sizeof(node));

  //size = size / num_threads;

  node** locs = (node**) malloc(sizeof(node*) * size); // node array
  node* nodes = (node*) malloc(sizeof(node) * size);  // temporary node pointers
  node** bases = (node**) malloc(sizeof(node*) * num_threads * num_lists); // initial node array
  *nodes_ptr = nodes;
  // initialize ids
  #pragma omp parallel for num_threads(num_threads)
  for(i = 0; i < size; ++i) {
#ifdef HAVE_ID
    nodes[i].id = i;
#endif
    nodes[i].next = (node*)i;
  }

  if (0) {
  // randomly permute nodes (fisher-yates (or knuth) shuffle)
  for(i = size-1; i > 0; --i) {
    uint64_t j = random() % i;
    node temp = nodes[i];
    nodes[i] = nodes[j];
    nodes[j] = temp;
  }
  }

  // extract locs
  #pragma omp parallel for num_threads(num_threads)
  for(i = 0; i < size; ++i) {
#ifdef HAVE_ID
    printf("%u %u\n",  nodes[i].id, &nodes[i] - &nodes[0] );
    assert( nodes[i].id == &nodes[i] - &nodes[0] );
    locs[ nodes[i].id ] = &nodes[i];
#else
    locs[ (uint64_t) nodes[i].next ] = &nodes[i];
#endif
  }

  // initialize pointers
  // chop id-space into num_lists sections and build a circular list for each
  //int64_t base = 0;
  #pragma omp parallel for num_threads(num_threads)
  for(i = 0; i < size; ++i) {
#ifdef HAVE_ID
    uint64_t id = nodes[i].id;
#else
    //assert( nodes[i].id == &nodes[i] - &nodes[0] );
    uint64_t id = i; //&nodes[i] - &nodes[0];
    node* current = locs[i];
#endif
    int64_t thread_size = size / num_lists;
    int64_t base = id / thread_size;
    uint64_t nextid = id + 1;
    uint64_t wrapped_nextid = (nextid % thread_size) + (base * thread_size) % size;
    if( wrapped_nextid == size ) wrapped_nextid = 0;
    printf("%d: %d %d %d\n", id, base, nextid, wrapped_nextid);
    current->next = locs[ wrapped_nextid ];
  }

  // grab initial pointers for each list
  for(i = 0; i < num_threads * num_lists; ++i) {
    bases[i] = locs[i * size / (num_threads * num_lists)];
  }

  //print(nodes, size);
  
  free(locs);
  return bases;
}
