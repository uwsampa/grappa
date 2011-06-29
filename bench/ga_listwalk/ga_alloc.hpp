
#include "ga++.h"

void allocate_ga( int rank, int num_nodes,
		  int64_t size, int num_threads, int num_lists,
		  GA::GlobalArray * nodes, GA::GlobalArray* bases,
		  int64_t *local_low, int64_t * local_high,  int64_t** local_array);
