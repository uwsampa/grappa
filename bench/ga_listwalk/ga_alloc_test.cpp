
#include <iostream>

#include <ctime>
#include <cstdlib>

#include "ga++.h"

#include "ga_alloc.hpp"

int main( int argc, char* argv[] ) {
  

  int64_t num_threads = 1;
  int64_t num_lists_per_thread = 1;

  int64_t size_log = 2;
  int64_t size = 1 << size_log;
  int64_t vertices_size_in_words = size << 3;
  int64_t bases_size_in_words = num_threads * num_lists_per_thread;

  // initialize random number generator
  srandom(time(NULL));


  GA::Initialize(argc, argv);
  
  int rank = GA::nodeid();
  int num_nodes = GA::nodes();

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
  

    int local_low = 0;
    int local_high = 0;
    uint64_t * local_array;
    
    allocate_ga( rank, num_nodes,
		 size, num_threads, num_lists_per_thread,
		 &vertices, &bases,
		 &local_low, &local_high, &local_array );
    
    std::cout << "node " << rank << " has range " << local_low << " to " << local_high << " at " << local_array << std::endl;

  }

  GA::Terminate();
}
