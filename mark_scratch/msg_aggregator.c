#include "msg_aggregator.h"
#include "global_memory.h"
#include "global_array.h"

#define MAX_MSGS         16834
#define BUFFERS_PER_NODE 4

#define AUTO_PUSH_SIZE_BYTES    (MSG_BUFFER_SIZE / 2)
#define AUTO_PUSH_SIZE_MSGS     (MAX_MSGS / 2)
#define AUTO_PUSH_MAX_DELAY     (50000)

struct msg {
    uint64 size;
    struct global_address   to_address;
    int function_dispatch_id;
    };

struct msg_queue {
    uint64 offset;
    int msgs;
    int head;
    volatile int tail;
    int dispatched_tail;
    uint64 oldest_message;
    uint64 bytes_in_block[BUFFERS_PER_NODE];
    bool replies[BUFFERS_PER_NODE];
    };

struct msg_queue    *outbound_msg_queues;
struct global_array *outbound_msg_packed_structs[BUFFERS_PER_NODE];
struct global_array *inbound_msg_packed_structs[BUFFERS_PER_NODE];

void msg_aggregator_init() {
    int i;
    
    outbound_msg_queues = malloc(sizeof(struct msg_queue) * gasnet_nodes());

    for (i = 0; i < BUFFERS_PER_NODE; i++) {
        outbound_msg_packed_structs[i] = ga_allocate(1, MSG_BUFFER_SIZE * gasnet_nodes());
        inbound_msg_packed_structs[i] = ga_allocate(1, MSG_BUFFER_SIZE * gasnet_nodes());
        }
            
    for (i = 0; i < gasnet_nodes(); i++) {
        outbound_msg_queues[i].head = outbound_msg_queues[i].tail = 0;
        outbound_msg_queues[i].offset = 0;
        }        
    }

void msg_aggregator_reply_handler(gasnet_token_t token,
    gasnet_handlerarg_t from_node,
    gasnet_handlerarg_t block) {
    outbound_msg_queues[from_node].replies[block] = true;
    // advance tail as far as we can
    while (outbound_msg_queues[from_node].tail !=
            outbound_msg_queues[from_node].dispatched_tail) {
        if (outbound_msg_queues[from_node].replies[
            outbound_msg_queues[from_node].tail])
            outbound_msg_queues[from_node].tail =
                (outbound_msg_queues[from_node].tail + 1) % BUFFERS_PER_NODE;
        else
            break;
        }
    }

void msg_aggregator_dispatch_handler(gasnet_token_t token,
    uint8 *buffer, size_t size,
    gasnet_handlerarg_t from_node,
    gasnet_handlerarg_t block) {
    size_t index = 0;
    struct msg *m;
    
    while (index < size) {
        m = (struct msg *) buffer;
        buffer += sizeof(struct msg);
        if (m->function_dispatch_id != NO_FUNCTION)
            function_dispatch(m->function_dispatch_id, buffer, m->size);
        else
            // Default function is a memcpy
            memcpy(gm_local_gm_address_to_local_ptr(&m->to_address), buffer, m->size);
        buffer += m->size;
        index += sizeof(struct msg) + m->size;
        }
    gasnet_AMReplyShort2(token, MSG_AGGREGATOR_REPLY, gasnet_mynode(), block);
    }
    
static void _dispatch_block(int node, int block) {
    void   *block_ptr, *dest_ptr;
    struct global_address   ga;
    
    ga_index(outbound_msg_packed_structs[block],
             gasnet_mynode() * MSG_BUFFER_SIZE,
             &ga);
    block_ptr = gm_local_gm_address_to_local_ptr(&ga);
    
    ga_index(inbound_msg_packed_structs[block],
             node * MSG_BUFFER_SIZE,
             &ga);
    dest_ptr = gm_address_to_ptr(&ga);
     
    outbound_msg_queues[node].replies[block] = false;

    gasnet_AMRequestLongAsync2(node, MSG_AGGREGATOR_DISPATCH,
        block_ptr, outbound_msg_queues[node].bytes_in_block[block],
        dest_ptr, gasnet_mynode(), block);        
    }
    
static void _advance_head_pointer(int node) {
    outbound_msg_queues[node].bytes_in_block[outbound_msg_queues[node].head] =
        outbound_msg_queues[node].offset;
        
    outbound_msg_queues[node].head =
            (outbound_msg_queues[node].head + 1) % BUFFERS_PER_NODE;
    outbound_msg_queues[node].offset = 0;
    outbound_msg_queues[node].msgs = 0;
    }

static void _potentially_dispatch_messages(int node) {
    while (outbound_msg_queues[node].head != outbound_msg_queues[node].dispatched_tail) {
        _dispatch_block(node, outbound_msg_queues[node].dispatched_tail);
        outbound_msg_queues[node].dispatched_tail = (outbound_msg_queues[node].dispatched_tail + 1) %
            BUFFERS_PER_NODE;
        }
    // Now look at head and decide whether it should be early dispatched
    if (outbound_msg_queues[node].offset >= AUTO_PUSH_SIZE_BYTES ||
        outbound_msg_queues[node].msgs >= AUTO_PUSH_SIZE_MSGS ||
        (outbound_msg_queues[node].msgs != 0 &&
         (rdtsc() - outbound_msg_queues[node].oldest_message) >= AUTO_PUSH_MAX_DELAY)) {
        _advance_head_pointer(node);
        _dispatch_block(node, outbound_msg_queues[node].dispatched_tail);
        outbound_msg_queues[node].dispatched_tail = (outbound_msg_queues[node].dispatched_tail + 1) %
            BUFFERS_PER_NODE;
        }
    }
    
void msg_send(  void *from_address,
                struct global_address *to_address,
                uint64 size,
                int function_dispatch_id) {
    struct msg *msg;
    struct global_address ga;
    uint8 *bptr;
        
    assert((sizeof(struct msg) + size) < MSG_BUFFER_SIZE);
    
    if ((outbound_msg_queues[to_address->node].offset +
        sizeof(struct msg) +
        size) > MSG_BUFFER_SIZE) {    
        while (((outbound_msg_queues[to_address->node].head + 1) % BUFFERS_PER_NODE) ==
            outbound_msg_queues[to_address->node].tail) {
            _potentially_dispatch_messages(to_address->node);
            gasnet_AMPoll();
            address_expose();
            }
        _advance_head_pointer(to_address->node);
        }
    ga_index(outbound_msg_packed_structs[outbound_msg_queues[to_address->node].head],
             gasnet_mynode() * MSG_BUFFER_SIZE + outbound_msg_queues[to_address->node].offset,
             &ga);
             
    bptr = (uint8 *) gm_local_gm_address_to_local_ptr(&ga);
    msg = (struct msg *) bptr;
    bptr += sizeof(struct msg);
    msg->size = size;
    msg->to_address = *to_address;
    msg->function_dispatch_id = function_dispatch_id;
    memcpy(bptr, from_address, size);
    
    outbound_msg_queues[to_address->node].offset += sizeof(struct msg) + size;
    if (outbound_msg_queues[to_address->node].msgs == 0)
        outbound_msg_queues[to_address->node].oldest_message = rdtsc();
    ++outbound_msg_queues[to_address->node].msgs;
    
    _potentially_dispatch_messages(to_address->node);
    return;
    }

void msg_poll() {
    int i;
    
    for (i = 0; i < gasnet_nodes(); i++)
        _potentially_dispatch_messages(i);
    }
    
void msg_flush(int node) {
    outbound_msg_queues[node].oldest_message = 0;
    _potentially_dispatch_messages(node);
    }
    
void msg_flush_all() {
    int i;
    
    for (i = 0; i < gasnet_nodes(); i++)
        msg_flush(i);
    }
