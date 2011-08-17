#include "msg_aggregator.h"
#include "global_memory.h"
#include "global_array.h"

#define MAX_MSGS         16834
#define BUFFERS_PER_NODE 4

#define MSG_BUFFER_SIZE    (32 * ONE_KILO)
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
    uint64 oldest_message;
    uint64 bytes_in_block[BUFFERS_PER_NODE];
    bool replies[BUFFERS_PER_NODE];
    };

struct msg_queue    *outbound_msg_queues;
struct global_array *outbound_msg_packed_structs[BUFFERS_PER_NODE];
struct global_array *inbound_msg_packed_structs[BUFFERS_PER_NODE];

struct push_request {
    void *from_address;
    struct global_address to_address;
    uint64 size;
    int function_dispatch_id;
    };

#define PUSH_REQUESTS_PER_BLOCK 128

struct push_requests {
    struct push_requests *next;
    int count;
    struct push_request pr[PUSH_REQUESTS_PER_BLOCK];
    };
    
struct push_requests *pr_free_list = NULL;
struct push_requests *pr_to_send_list = NULL;

static struct push_requests *_pr_allocate() {
    struct push_requests *pr;
    
    if (pr_free_list) {
        pr = pr_free_list;
        pr_free_list = pr_free_list->next;
        goto exit_function;
        }
    pr = malloc(sizeof(*pr));

    exit_function:
    
    pr->count = 0;
    pr->next = NULL;
    return pr;
    }

void   _pr_free(struct push_requests *pr) {
    pr->next = pr_free_list;
    pr_free_list = pr;
    }

static void _process_push_requests() {
    struct push_requests *pr;
    int c;
    
    while (pr_to_send_list) {
        pr = pr_to_send_list;
        pr_to_send_list = pr_to_send_list->next;
        c = 0;
        while (c < pr->count) {
            msg_send(pr->pr[c].from_address, &pr->pr[c].to_address, pr->pr[c].size,
                pr->pr[c].function_dispatch_id);
            ++c;
            }
        _pr_free(pr);
        }
}

static void _attach_push_requests(struct push_requests *pr) {
    pr->next = pr_to_send_list;
    pr_to_send_list = pr;
    }

void msg_send_delayed(void *from_address,
                            struct global_address *to_address,
                            uint64 size,
                            int function_dispatch_id) {                        
    if (!pr_to_send_list || pr_to_send_list->count == PUSH_REQUESTS_PER_BLOCK)
        _attach_push_requests(_pr_allocate());
    
    pr_to_send_list->pr[pr_to_send_list->count].from_address = from_address;
    pr_to_send_list->pr[pr_to_send_list->count].to_address = *to_address;
    pr_to_send_list->pr[pr_to_send_list->count].size = size;
    pr_to_send_list->pr[pr_to_send_list->count].function_dispatch_id =
        function_dispatch_id;
    ++pr_to_send_list->count;
    }
    
void msg_aggregator_init() {
    int i;
    
    outbound_msg_queues = malloc(sizeof(struct msg_queue) * gasnet_nodes());

    for (i = 0; i < BUFFERS_PER_NODE; i++) {
        outbound_msg_packed_structs[i] = ga_allocate(1, MSG_BUFFER_SIZE * gasnet_nodes()
            * gasnet_nodes());
        inbound_msg_packed_structs[i] = ga_allocate(1, MSG_BUFFER_SIZE * gasnet_nodes()
            * gasnet_nodes());
        }
            
    for (i = 0; i < gasnet_nodes(); i++) {
        outbound_msg_queues[i].head = outbound_msg_queues[i].tail = 0;
        outbound_msg_queues[i].offset = 0;
        outbound_msg_queues[i].msgs = 0;
        outbound_msg_queues[i].oldest_message = 0;
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
        if (m->function_dispatch_id == PULL_FUNCTION) {
            struct msg_pull_request *request = (struct msg_pull_request *)buffer;
            struct global_address *from_address = &m->to_address;    //yes, swapped
            void *local_ptr = gm_local_gm_address_to_local_ptr(from_address);
            msg_send_delayed(local_ptr, &request->to_address, request->size,
                PUT_FUNCTION);
            }
        else if (m->function_dispatch_id == PUT_FUNCTION) {
            memcpy(gm_local_gm_address_to_local_ptr(&m->to_address), buffer, m->size);
            }
        else {
            function_dispatch(m->function_dispatch_id, buffer, m->size);
            }
        buffer += m->size;
        index += sizeof(struct msg) + m->size;
        }
    assert(index == size);

    gasnet_AMReplyShort2(token, MSG_AGGREGATOR_REPLY, gasnet_mynode(), block);
    }

void msg_aggregator_reply_handler(gasnet_token_t token,
    gasnet_handlerarg_t from_node,
    gasnet_handlerarg_t block) {
    outbound_msg_queues[from_node].replies[block] = true;
    // advance tail as far as we can
    while (outbound_msg_queues[from_node].tail !=
            outbound_msg_queues[from_node].head) {
        if (outbound_msg_queues[from_node].replies[
             outbound_msg_queues[from_node].tail])
            outbound_msg_queues[from_node].tail =
                (outbound_msg_queues[from_node].tail + 1) % BUFFERS_PER_NODE;
        else
            break;
        }
    }

static void _dispatch_block(int node, int block) {
    void   *block_ptr, *dest_ptr;
    struct global_address   ga;
    
    assert(block <= BUFFERS_PER_NODE);
    assert(node <= gasnet_nodes());    
    assert(outbound_msg_queues[node].bytes_in_block[block] != 0);
    assert(outbound_msg_queues[node].bytes_in_block[block] <= MSG_BUFFER_SIZE);

    ga_index(outbound_msg_packed_structs[block],
             (gasnet_mynode() * gasnet_nodes() + node) * MSG_BUFFER_SIZE,
             &ga);

    block_ptr = gm_local_gm_address_to_local_ptr(&ga);
    
    ga_index(inbound_msg_packed_structs[block],
             (node * gasnet_nodes() + gasnet_mynode()) * MSG_BUFFER_SIZE,
             &ga);
    assert(ga.node == node);

    dest_ptr = gm_address_to_ptr(&ga);
    
    outbound_msg_queues[node].replies[block] = false;

    gasnet_AMRequestLongAsync2(node, MSG_AGGREGATOR_DISPATCH,
        block_ptr, outbound_msg_queues[node].bytes_in_block[block],
        dest_ptr, gasnet_mynode(), block);
    }
    
static void _potentially_dispatch_messages(int node, bool force_head) {
    if (outbound_msg_queues[node].msgs != 0 &&
        ((outbound_msg_queues[node].head + 1) % BUFFERS_PER_NODE) !=
            outbound_msg_queues[node].tail) {
        if (force_head ||
            outbound_msg_queues[node].offset >= AUTO_PUSH_SIZE_BYTES ||
            outbound_msg_queues[node].msgs >= AUTO_PUSH_SIZE_MSGS ||
            (rdtsc() - outbound_msg_queues[node].oldest_message)
                >= AUTO_PUSH_MAX_DELAY) {
            if (outbound_msg_queues[node].offset == 0) {
                printf("offset = 0 but msgs=%d\n", outbound_msg_queues[node].msgs);
                }
            outbound_msg_queues[node].bytes_in_block[outbound_msg_queues[node].head] =
                outbound_msg_queues[node].offset;
            _dispatch_block(node, outbound_msg_queues[node].head);

            outbound_msg_queues[node].head =
                (outbound_msg_queues[node].head + 1) % BUFFERS_PER_NODE;
            outbound_msg_queues[node].offset = 0;
            outbound_msg_queues[node].msgs = 0;
            }
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
    
    while ((outbound_msg_queues[to_address->node].offset +
        sizeof(struct msg) + size) > MSG_BUFFER_SIZE) {
            _potentially_dispatch_messages(to_address->node, true);
            gasnet_AMPoll();
            address_expose();
        }

    ga_index(outbound_msg_packed_structs[outbound_msg_queues[to_address->node].head],
             (gasnet_mynode() * gasnet_nodes() + to_address->node) * MSG_BUFFER_SIZE +
                outbound_msg_queues[to_address->node].offset,
             &ga);
             
    bptr = (uint8 *) gm_local_gm_address_to_local_ptr(&ga);
    msg = (struct msg *) bptr;
    bptr += sizeof(struct msg);
    msg->size = size;
    msg->to_address = *to_address;
    msg->function_dispatch_id = function_dispatch_id;
    memcpy(bptr, from_address, size);
    
    outbound_msg_queues[to_address->node].offset += sizeof(struct msg) + size;
    assert(outbound_msg_queues[to_address->node].offset <= MSG_BUFFER_SIZE);

    if (outbound_msg_queues[to_address->node].msgs == 0)
        outbound_msg_queues[to_address->node].oldest_message = rdtsc();
    ++outbound_msg_queues[to_address->node].msgs;
    
    _potentially_dispatch_messages(to_address->node, false);
    return;
    }

void msg_poll() {
    int i;
    
    _process_push_requests();
    for (i = 0; i < gasnet_nodes(); i++)
        _potentially_dispatch_messages(i, false);
    }
    
void msg_flush(int node) {
    outbound_msg_queues[node].oldest_message = 0;
    _process_push_requests();
    _potentially_dispatch_messages(node, true);
    }
    
void msg_flush_all() {
    int i;
    
    for (i = 0; i < gasnet_nodes(); i++)
        msg_flush(i);
    }
