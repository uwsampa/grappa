#pragma once
#include "base.h"
#include "global_memory.h"

#define MSG_AGGREGATOR_DISPATCH 135
#define MSG_AGGREGATOR_REPLY   136

#define NO_FUNCTION   0
#define PUT_FUNCTION 1
#define PULL_FUNCTION 2

// Users should provide this function
void function_dispatch(int func_id, void *buffer, uint64 size);

void msg_send(  void *from_address,
                struct global_address *to_address,
                uint64 size,
                int function_dispatch_id);

void msg_send_delayed(void *from_address,
                            struct global_address *to_address,
                            uint64 size,
                            int function_dispatch_id);
                             
struct msg_pull_request {
    struct global_address to_address;
    uint64 size;
    };

static inline void msg_copy_to(void *from_address,
    struct global_address *to_address,
    uint64 size) {
    msg_send(from_address, to_address, size, PUT_FUNCTION);
    }

static inline void msg_copy_from(struct global_address *from_address,
                struct msg_pull_request *request) {
    msg_send(request, from_address,
        sizeof(struct msg_pull_request), PULL_FUNCTION);
    }
    
void msg_poll();
void msg_flush(int node);
void msg_flush_all();
void msg_aggregator_init();

void msg_aggregator_reply_handler(gasnet_token_t token,
    gasnet_handlerarg_t from_node,
    gasnet_handlerarg_t block);

void msg_aggregator_dispatch_handler(gasnet_token_t token,
    uint8 *buffer, size_t size,
    gasnet_handlerarg_t from_node,
    gasnet_handlerarg_t block);    
