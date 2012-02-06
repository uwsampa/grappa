#include "gasnet_cbarrier.h"
#include "StealQueue.h"

int SET_COND(LOCK_T* lock) {
    SET_LOCK(lock);
    return 1;
}
int UNSET_COND(LOCK_T* lock) {
    UNSET_LOCK(lock);
    return 0;
}


const int HOME_NODE = 0;
gasnet_hsl_t cb_lock = GASNET_HSL_INTIALIZER;
int num_barrier_clients;
int num_waiting_clients;
int my_node;
std::queue<int>* waiters;

void enter_cbarrier_request_handler(gasnet_token_t token) {
    gasnet_node_t source;
    gasnetAMGetMsgSource(token, &source);
    num_waiting_clients++;
    if (num_waiting_clients == num_barrier_clients) {
        while (!waiters->empty()) {
            int nod = waiters->front();
            waiters->pop();
            gasnet_AMRequestShort1(nod, EXIT_CBARRIER_REQUEST_HANDLER, 1);
        }
        num_waiting_clients = 0;
        gasnet_AMReplyShort1(token, EXIT_CBARRIER_REQUEST_HANDLER, 1);
    }
}

void exit_cbarrier_request_handler(gasnet_token_t token, gasnet_handlerarg_t a0) {
    int finished = (int) a0;

    SET_LOCK(&cb_lock);
    cb_reply = 1;
    cb_done = finished;
    UNSET_LOCK(&cb_lock);
}

void cancel_cbarrier_request_handler(gasnet_token_t token) {
    while (!waiters->empty()) {
        int nod = waiters->front();
        waiters->pop();
        gasnet_AMRequestShort1(nod, EXIT_CBARRIER_REQUEST_HANDLER, 0);
    }
    num_waiting_clients = 0;
}


void cbarrier_cancel() {
    gasnet_AMRequestShort0(HOME_NODE, CANCEL_CBARRIER_REQUEST_HANDLER);
}

int cbarrier_wait() {
    gasnet_AMRequestShort0(HOME_NODE, ENTER_CBARRIER_REQUEST_HANDLER);

    int isDone;
    GASNET_BLOCKUNTIL((SET_COND(&cb_lock) &&
                        cb_reply > 0)
                        || UNSET_COND(&cb_lock));
    isDone = cb_done;
    cb_done = 0;
    cb_reply = 0;
    UNSET_LOCK(&cb_lock);
    return isDone;
}

void cbarrier_init(int num_nodes, int rank) {
    num_barrier_clients = num_nodes;
    num_waiting_clients = 0;
    my_node = rank;
    if (my_node == HOME_NODE) {
        waiters = new std::queue <int>();
    }

    SET_LOCK(&cb_lock) {
        cb_done = 0;
        cb_reply = 0;
    }
}
