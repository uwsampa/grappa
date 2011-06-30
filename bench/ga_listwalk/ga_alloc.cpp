//#include "linked_list-node.h"
//#include "linked_list-alloc.h"

//#include <cstdio>
#include <iostream>

#include <cstdlib>
#include <cstdint>
#include <cassert>

#include "ga_alloc.hpp"
#include "node.hpp"

#include "mpi.h"
#include "ga++.h"

#ifndef DEBUG
#define DEBUG 0
#endif

void ga_put_struct( GA::GlobalArray * ga, int64_t index, int64_t offset, uint64_t data ) {
  int ga_index = (index * sizeof(node) + offset) / sizeof(uint64_t);
  ga->put( &ga_index, &ga_index, &data, NULL );
}

uint64_t ga_get_struct( GA::GlobalArray * ga, int64_t index, int64_t offset ) {
  int64_t ga_index = (index * sizeof(node) + offset) / sizeof(uint64_t);
  uint64_t data;
  ga->get( &ga_index, &ga_index, &data, NULL );
  return data;
}


// this function runs on all nodes--watch out for duplicate work
void allocate_ga( int rank, int num_nodes,
		  int64_t size, int num_threads_per_node, int num_lists_per_thread,
		  GA::GlobalArray * vertices, GA::GlobalArray* bases,
		  int64_t *local_begin, int64_t * local_end,  int64_t** local_array) {
  if (0 == rank) std::cout << "Allocating." << std::endl;

  //if (rank == 0) 
    //printf("Initializing footprint of size %lu * %lu = %lu bytes....\n", size, sizeof(node), size * sizeof(node));



  // get index range for local array
  int local_index_begin = 0;
  int local_index_end = 0;
  vertices->distribution( rank, &local_index_begin, &local_index_end );

  // get pointer for local array
  vertices->access( &local_index_begin, &local_index_end, local_array, NULL );

  *local_begin = local_index_begin * sizeof(uint64_t) / sizeof(node);
  *local_end = (local_index_end + 1) * sizeof(uint64_t) / sizeof(node);

  GA::GlobalArray locs = GA::GlobalArray( MT_C_LONGLONG, // type
					  1,             // one-dimensional
					  &size,  // number of elements along each dimension
					  "locs",  // name
					  NULL );        // distribute evenly

  // initialize ids
  for (int64_t i = *local_begin; i < *local_end; ++i) {
    ga_put_struct( vertices, i, offsetof(node, id), -i );
    ga_put_struct( vertices, i, offsetof(node, next), i );
  }


  MPI_Barrier( MPI_COMM_WORLD );

  // randomly permute nodes (fisher-yates (or knuth) shuffle)
  if (rank == 0) {
    for(int64_t i = size-1; i != 0; --i) {
      uint64_t j = random() % i;

      int64_t temp1id = ga_get_struct( vertices, i, offsetof(node, id) );
      int64_t temp1next = ga_get_struct( vertices, i, offsetof(node, next) );

      int64_t temp2id = ga_get_struct( vertices, j, offsetof(node, id) );
      int64_t temp2next = ga_get_struct( vertices, j, offsetof(node, next) );
      
      ga_put_struct( vertices, i, offsetof(node, id), temp2id );
      ga_put_struct( vertices, i, offsetof(node, next), temp2next );

      ga_put_struct( vertices, j, offsetof(node, id), temp1id );
      ga_put_struct( vertices, j, offsetof(node, next), temp1next );
    }
  }

  MPI_Barrier( MPI_COMM_WORLD );

  // extract locs
  for(int64_t i = *local_begin; i < *local_end; ++i) {
    int64_t next = ga_get_struct( vertices, i, offsetof(node, next) );
    locs.put( &next, &next, &i, NULL );
  }

  MPI_Barrier( MPI_COMM_WORLD );

  // initialize pointers
  // chop id-space into num_lists sections and build a circular list for each
  for(int64_t i = *local_begin; i < *local_end; ++i) {
    uint64_t id = i;
    int64_t current = 0;
    locs.get( &i, &i, &current, NULL );
    int64_t thread_size = size / (num_threads_per_node * num_nodes * num_lists_per_thread);
    int64_t base = id / thread_size;
    uint64_t nextid = id + 1;
    int64_t wrapped_nextid = ( (nextid % thread_size) + (base * thread_size) ) % size;
    
    int64_t nextid_loc = 0;
    locs.get( &wrapped_nextid, &wrapped_nextid, &nextid_loc, NULL );

    ga_put_struct( vertices, current, offsetof(node, next), nextid_loc );
  }

  MPI_Barrier( MPI_COMM_WORLD );

  // grab initial pointers for each list
  if (rank == 0) {
    for(int64_t i = 0; i < num_nodes * num_threads_per_node * num_lists_per_thread; ++i) {
      int64_t loc = i * size / (num_nodes * num_threads_per_node * num_lists_per_thread);
      int64_t base = 0;
      locs.get( &loc, &loc, &base, NULL );
      if (DEBUG) std::cout << "i=" << i << " loc=" << loc << " base=" << base << std::endl;
      bases->put( &i, &i, &base, NULL );
    }
  }

  MPI_Barrier( MPI_COMM_WORLD );
}
