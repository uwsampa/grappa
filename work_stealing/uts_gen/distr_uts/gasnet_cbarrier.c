#include "gasnet_cbarrier.h"
#include "StealQueue.h"
#include <queue>


const int HOME_NODE = 0;
gasnet_hsl_t cb_lock = GASNET_HSL_INITIALIZER;
int num_barrier_clients;
int num_waiting_clients;
int my_node;
std::queue<int>* waiters;
int cb_reply;
int cb_done;

void enter_cbarrier_request_handler(gasnet_token_t token) {
    gasnet_node_t source;
    gasnet_AMGetMsgSource(token, &source);
    num_waiting_clients++;
    if (num_waiting_clients == num_barrier_clients) {
        while (!waiters->empty()) {
            int nod = waiters->front();
            waiters->pop();
            gasnet_AMRequestShort1(nod, EXIT_CBARRIER_REQUEST_HANDLER, 1);
        }
        num_waiting_clients = 0;
        gasnet_AMReplyShort1(token, EXIT_CBARRIER_REQUEST_HANDLER, 1);
    } else {
        waiters->push(source);
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

#define STATEMENT_WRAP(stmt, id, value) \
        int _wrapped_(id)_ () { \
            (stmt); \
            return (value); \
        } 

#define STATEMENT(id) \
        _wrapped_(id)_ ()

#define STATEMENT_WRAP(SET_LOCK(&cb_lock), 0, 1);
#define set_my_lock = STATEMENT(0)

#define STATEMENT_WRAP(UNSET_LOCK(&cb_lock), 1, 0);
#define unset_my_lock = STATEMENT(1)

#define STATEMENT_WRAP(SoftXMT_yield(), 2, 0);
#define yield_stmt = STATEMENT(3)

int cbarrier_wait() {
    gasnet_AMRequestShort0(HOME_NODE, ENTER_CBARRIER_REQUEST_HANDLER);

    int isDone;
    GASNET_BLOCKUNTIL((set_my_lock &&
                        cb_reply > 0)
                        || unset_my_lock
                        || yield_stmt);
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
