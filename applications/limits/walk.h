
#include <stdint.h>
#include "node.h"

uint64_t singlewalk( node nodes[], uint64_t count );
uint64_t multiwalk( node* bases[], 
		    uint64_t concurrent_reads, 
		    uint64_t count );
uint64_t walk( node* bases[], uint64_t count, int num_refs, int start_index );
