
#include <iostream>

#include <ctime>
#include <cstdlib>
#include <cassert>

#include "mpi.h"
#include "ga++.h"

#include "node.hpp"
#include "ga_alloc.hpp"

#ifndef DEBUG
#define DEBUG 0
#endif

int main( int argc, char* argv[] ) {
  

  GA::Initialize(argc, argv);
  
  int rank = GA::nodeid();
  int num_nodes = GA::nodes();

  int64_t num_threads_per_node = NUM_JUMP_THREADS;
  int64_t num_lists_per_thread = 1;

  int64_t size_log = SIZE_LOG;
  int64_t size = 1 << size_log;
  int64_t vertices_size_in_words = size * sizeof(node) / sizeof(int64_t);
  int64_t bases_size_in_words = num_nodes * num_threads_per_node * num_lists_per_thread;

  // initialize random number generator
  srandom(time(NULL));
  //srandom(0);

  if (0 == rank) std::cout << "Start." << std::endl;

  {
    GA::GlobalArray vertices = GA::GlobalArray( MT_C_LONGLONG, // type
						1,             // one-dimensional
						&vertices_size_in_words,   // number of elements along each dimension
						"vertices",  // name
						NULL );        // distribute evenly
  
    GA::GlobalArray bases = GA::GlobalArray( MT_C_LONGLONG, // type
					     1,             // one-dimensional
					     &bases_size_in_words,  // number of elements along each dimension
					     "bases",  // name
					     NULL );        // distribute evenly
  
    GA::GlobalArray times = GA::GlobalArray( MT_C_DBL, // type
					     1,             // one-dimensional
					     &num_nodes,  // number of elements along each dimension
					     "times",  // name
					     NULL );        // distribute evenly
  

    int64_t local_begin = 0;
    int64_t local_end = 0;
    int64_t * local_array;
    
    allocate_ga( rank, num_nodes,
		 size, num_threads_per_node, num_lists_per_thread,
		 &vertices, &bases,
		 &local_begin, &local_end, &local_array );

  if (0 == rank) std::cout << "Running." << std::endl;
    

    //if (DEBUG) std::cout << "node " << rank << " has range " << local_begin << " to " << local_end << " at " << local_array << std::endl;
    vertices.printDistribution();


    MPI_Barrier( MPI_COMM_WORLD );
    if (DEBUG) {
      bases.print();
      //vertices.print();
      if (0 == rank) {
	for(int64_t i = 0; i < size; ++i) {
	  int64_t index = ((i << 6) >> 3);
	  int64_t id = 0;
	  vertices.get( &index, &index, &id, NULL );
	  int64_t next = 0;
	  index++;
	  vertices.get( &index, &index, &next, NULL );
	  std::cout << "index=" << i << " id=" << -id << " next=" << next << std::endl;
	}
      }

      if (0) {
	for(int64_t i = local_begin; i < local_end ; ++i) {
	  std::cout << "local_array[" << i << "] =";
	  for(int64_t j = 0; j < 8; ++j) {
	    std::cout << " " << local_array[((i - local_begin) << 3) + j];
	  }
	  std::cout << std::endl;
	}
      }
    }
    MPI_Barrier( MPI_COMM_WORLD );

    {
      int64_t count = 20 * size / (num_nodes * num_threads_per_node);
      int64_t sum = 0;
      if (DEBUG) std::cout << "node " << rank << " starting with count " << count << std::endl;

      timespec start_time, end_time;
      clock_gettime(CLOCK_MONOTONIC, &start_time);
      MPI_Barrier( MPI_COMM_WORLD );
      #include "jumpwalk.cunroll"
      MPI_Barrier( MPI_COMM_WORLD );
      clock_gettime(CLOCK_MONOTONIC, &end_time);

      double runtime = (end_time.tv_sec + 1.0e-9 * end_time.tv_nsec) - (start_time.tv_sec + 1.0e-9 * start_time.tv_nsec);
      double min_runtime = 0.0;
      MPI_Reduce( &runtime, &min_runtime, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD );
      double max_runtime = 0.0;
      MPI_Reduce( &runtime, &max_runtime, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD );

      double rate = count * num_lists_per_thread * num_threads_per_node * num_nodes / min_runtime;

      std::cout << "node " << rank << " runtime is " << runtime << std::endl;
      std::cout << "node " << rank << " sum is " << sum << std::endl;
      if (DEBUG) if (0 == rank) std::cout << "runtime is in " << min_runtime << ", " << max_runtime << std::endl;
      if (0 == rank) std::cout << "rate is " << rate / 1000000.0 << " Mref/s" << std::endl;
    }
  }

  GA::Terminate();
}
