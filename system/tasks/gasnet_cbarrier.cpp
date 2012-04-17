#include "gasnet_cbarrier.hpp"
#include "StealQueue.hpp"
#include <queue>
#include "../SoftXMT.hpp"

const int HOME_NODE = 0;
int num_barrier_clients;
int num_waiting_clients;
int my_node;
std::queue<Node>* waiters;
int cb_reply;
int cb_done;

struct enter_cbarrier_request_args {
    Node from;
};

struct exit_cbarrier_request_args {
    int finished;
};

struct cancel_cbarrier_request_args {
//nothing
};

void exit_cbarrier_request_am( exit_cbarrier_request_args * args, size_t size, void * payload, size_t payload_size );

void enter_cbarrier_request_am( enter_cbarrier_request_args * args, size_t size, void * payload, size_t payload_size ) {
    Node source = args->from;

    num_waiting_clients++;
    if (num_waiting_clients == num_barrier_clients) {
        DVLOG(5) << "enter_cbarrier_request_am: last";
        while (!waiters->empty()) {
            Node nod = waiters->front();
            waiters->pop();
            exit_cbarrier_request_args exargs = { 1 };
            DVLOG(5) << "enter_cbarrier_request_am: sending to " << nod;
            SoftXMT_call_on( nod, &exit_cbarrier_request_am, &exargs );
        }
        num_waiting_clients = 0;
        exit_cbarrier_request_args exargs = { 1 };
        DVLOG(5) << "enter_cbarrier_request_am: sending to " << source;
        SoftXMT_call_on( source, &exit_cbarrier_request_am, &exargs );
    } else {
        DVLOG(5) << "enter_cbarrier_request_am: not last";
        waiters->push(source);
    }
}

void exit_cbarrier_request_am( exit_cbarrier_request_args * args, size_t size, void * payload, size_t payload_size ) {
    VLOG(5) << "exit_cbarrier_am: finished=" << args->finished;
    int finished = args->finished;

    cb_reply = 1;
    cb_done = finished;
}

void cancel_cbarrier_request_am( cancel_cbarrier_request_args * args, size_t size, void * payload, size_t payload_size ) {
    VLOG(5) << "home_node called to send cancels";
    while (!waiters->empty()) {
        Node nod = waiters->front();
        waiters->pop();
        exit_cbarrier_request_args exargs = { 0 };
        VLOG(5) << "home_node sends cancel to "<<nod;
        SoftXMT_call_on( nod, &exit_cbarrier_request_am, &exargs );
    }
    num_waiting_clients = 0;
}


void cbarrier_cancel() {
    cancel_cbarrier_request_args cargs;
    SoftXMT_call_on( HOME_NODE, &cancel_cbarrier_request_am, &cargs );
    VLOG(5) << "sent cancel job to home_node";
}

/*
 * This was used with GASNET_BLOCKUNTIL
//define statements that can be used inside gasnet_blockuntil as expressions
#define STATEMENT_WRAP(stmt, id, value) \
        int gasnet_blockuntil_wrapped_##id () { \
            stmt; \
            return (value); \
        } 

#define STATEMENT(id) gasnet_blockuntil_wrapped_##id ()

STATEMENT_WRAP(SoftXMT_yield(), yield, 0)
#define yield_stmt STATEMENT(yield)

STATEMENT_WRAP(SET_LOCK(&cb_lock), set, 1)
#define set_my_lock STATEMENT(set)

STATEMENT_WRAP(UNSET_LOCK(&cb_lock), unset, 0)
#define unset_my_lock STATEMENT(unset)
*/

int cbarrier_wait() {
    enter_cbarrier_request_args enargs = { SoftXMT_mynode() };
    SoftXMT_call_on( HOME_NODE, &enter_cbarrier_request_am, &enargs );

    int isDone;
    while (true) {
        if (cb_reply > 0) 
            break;
        SoftXMT_yield();
    }

    /*    
    GASNET_BLOCKUNTIL((set_my_lock &&
                        cb_reply > 0)
                        || unset_my_lock
                        || yield_stmt);
    */

    isDone = cb_done;
    cb_done = 0;
    cb_reply = 0;
    return isDone;
}

void cbarrier_init(int num_nodes, int rank) {
    num_barrier_clients = num_nodes;
    num_waiting_clients = 0;
    my_node = rank;
    if (my_node == HOME_NODE) {
        waiters = new std::queue <Node>();
    }

    cb_done = 0;
    cb_reply = 0;
}
