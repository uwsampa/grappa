
#ifndef __GLOBALLISTALLOCATE_HPP__
#define __GLOBALLISTALLOCATE_HPP__

#include <GlobalListAllocate.hpp>


// initialize linked lists.
//node_t** allocate_heap(uint64_t size, int num_threads, int num_lists) {
template < typename GlobalArray > 
allocate_heap(uint64_t size, int num_threads, int num_lists, GlobalArray* array, GlobalArray* base) {
  uint64_t i = 0;

  //printf("Initializing footprint of size %lu * %lu = %lu bytes....\n", size, sizeof(node_t), size * sizeof(node_t));

  //size = size / num_threads;

  node_t** locs = (node_t**) malloc(sizeof(node_t*) * size); // node array
  node_t* nodes = (node_t*) malloc(sizeof(node_t) * size);  // temporary node pointers
  //  node_t* nodes = (node_t*) get_huge_pages(sizeof(node_t) * size, GHP_DEFAULT);  // temporary node pointers
  node_t** bases = (node_t**) malloc(sizeof(node_t*) * num_threads * num_lists); // initial node array

  // initialize ids
  //#pragma omp parallel for num_threads(num_threads)
  for(i = 0; i < size; ++i) {
    //#ifdef HAVE_ID
    nodes[i].id = i;
    nodes[i].index = i;
    //#endif
    nodes[i].next = (node_t*)i;
  }

  // randomly permute nodes (fisher-yates (or knuth) shuffle)
  for(i = size-1; i > 0; --i) {
    uint64_t j = random() % i;
    node_t temp = nodes[i];
    nodes[i] = nodes[j];
    nodes[j] = temp;
  }

  // extract locs
  //#pragma omp parallel for num_threads(num_threads)
  for(i = 0; i < size; ++i) {
#ifdef HAVE_ID
    //printf("%u %u\n",  nodes[i].id, &nodes[i] - &nodes[0] );
    //assert( nodes[i].id == &nodes[i] - &nodes[0] );
    locs[ nodes[i].id ] = &nodes[i];
#else
    locs[ (uint64_t) nodes[i].next ] = &nodes[i];
#endif
  }

    for(int i = 0; i < size; ++i) {
    assert(locs[i] != NULL);
  }

// initialize pointers
  // chop id-space into num_lists sections and build a circular list for each
  //int64_t base = 0;
  //#pragma omp parallel for num_threads(num_threads)
  for(i = 0; i < size; ++i) {
#ifdef HAVE_ID
    uint64_t id = nodes[i].id;
#else
    //assert( nodes[i].id == &nodes[i] - &nodes[0] );
    uint64_t id = i; //&nodes[i] - &nodes[0];
    node_t* current = locs[i];
#endif
    int64_t thread_size = size / num_lists;
    int64_t base = id / thread_size;
    uint64_t nextid = id + 1;
    uint64_t wrapped_nextid = (nextid % thread_size) + (base * thread_size) % size;
    //printf("%d: %d %d %d\n", id, base, nextid, wrapped_nextid);
    current->next = locs[ wrapped_nextid ];
    if (current->next) current->index = current->next->id;
  }

  // grab initial pointers for each list
  for(i = 0; i < (uint64_t) num_threads * num_lists; ++i) {
    bases[i] = locs[i * size / (num_threads * num_lists)];
  }

  //print(nodes, size);
  
  free(locs);
  return bases;
}

#endif
