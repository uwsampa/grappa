
#include <stdint.h>
#include "node.h"

#include "thread.h"
#include "jumpthreads.h"
#include "MCRingBuffer.h"

uint64_t walk_prefetch_switch( thread* me, node* bases[], uint64_t count, int num_refs, int start_index );

uint64_t walk( node* bases[], uint64_t count, int num_refs, int start_index );
uint64_t walk_jump( node* bases[], uint64_t count, int num_refs, int start_index , jthr_memdesc* memdescs, MCRingBuffer* send_queue, MCRingBuffer* receive_queue);

