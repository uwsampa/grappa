
#include <stdint.h>
#include "node.h"

uint64_t walk( node nodes[], uint64_t count );
uint64_t multiwalk( node* bases[], 
		    uint64_t concurrent_reads, 
		    uint64_t count );
