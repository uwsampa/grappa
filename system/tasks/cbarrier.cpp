
#include "cbarrier.hpp"
#include "StealQueue.hpp"
#include <list>
#include "../SoftXMT.hpp"

const int HOME_NODE = 0;
Node num_barrier_clients;
Node num_waiting_clients;
std::list<Node>* waiters;
bool * notified;
bool cb_reply;
bool cb_done;
Thread * wakeme;

struct enter_cbarrier_request_args {
    Node from;
};

struct exit_cbarrier_request_args {
    bool finished;
};

struct cancel_cbarrier_request_args {
//nothing
};

void exit_cbarrier_request_am( exit_cbarrier_request_args * args, size_t size, void * payload, size_t payload_size );

void enter_cbarrier_request_am( enter_cbarrier_request_args * args, size_t size, void * payload, size_t payload_size ) {
    Node source = args->from;

    // ensure we haven't gotten a duplicate message
    // (supports efficient cancel_local)
    if ( !notified[source] ) {
        notified[source] = true;
        num_waiting_clients++;
        if (num_waiting_clients == num_barrier_clients) {
            num_waiting_clients = 0;
            // only one thread can enter by proper usage of barrier (only one last enter AM)
            // And after this message no cancel can be received
            DVLOG(5) << "enter_cbarrier_request_am: last";
            while (!waiters->empty()) {
                Node nod = waiters->front();
                waiters->pop_front();
                exit_cbarrier_request_args exargs = { true };
                DVLOG(5) << "enter_cbarrier_request_am: sending to " << nod;
                notified[nod] = false;
                SoftXMT_call_on( nod, &exit_cbarrier_request_am, &exargs ); 
            }
            exit_cbarrier_request_args exargs = { true };
            DVLOG(5) << "enter_cbarrier_request_am: sending to " << source;
            notified[source] = false;
            SoftXMT_call_on( source, &exit_cbarrier_request_am, &exargs );
        } else {
            DVLOG(5) << "enter_cbarrier_request_am: not last (" << num_waiting_clients << "/" << num_barrier_clients;
            waiters->push_back(source);
        }
    } else {
        DVLOG(5) << "enter_cbarrier_request_am: duplicate (" << num_waiting_clients << "/" << num_barrier_clients;;
    }
}

void exit_cbarrier_request_am( exit_cbarrier_request_args * args, size_t size, void * payload, size_t payload_size ) {
    VLOG(5) << "exit_cbarrier_am: finished=" << args->finished;

    if ( args->finished ) {
        cb_done = true;
    }

    if ( wakeme ) {  // might have been woken by local cancel, or by an enter-finish
        cb_reply = true;
        SoftXMT_wake( wakeme );
        wakeme = NULL;
    }
}

void cancel_cbarrier_request_am( cancel_cbarrier_request_args * args, size_t size, void * payload, size_t payload_size ) {
    VLOG(5) << "home_node called to send cancels";
    num_waiting_clients = 0;
        
    while (!waiters->empty()) {
        Node nod = waiters->front();
        waiters->pop_front();
        exit_cbarrier_request_args exargs = { false };
        VLOG(5) << "home_node sends cancel to "<<nod;
        notified[nod] = false;
        SoftXMT_call_on( nod, &exit_cbarrier_request_am, &exargs );
    }
}

void cbarrier_cancel() {
    CHECK( wakeme == NULL ) << "Node cannot cancel cbarrier while waiting on it";
    
    cancel_cbarrier_request_args cargs;
    SoftXMT_call_on( HOME_NODE, &cancel_cbarrier_request_am, &cargs );
    VLOG(5) << "sent cancel job to home_node";
}


bool cbarrier_wait() {
    CHECK( wakeme == NULL ) << "more than one thread entered cbarrier on this Node";
    
    
    // NOTE: due to cancel_local making the waiting count not decrement
    // it was possible that user_main exit causing Node0 cbarrier_wait,
    // causing count to trigger, causing cb_done on Node1 to be set
    // before it enters the barrier again. So instead of asserting
    // cb_done=false, we just check to see if already set.
    // TODO: really we want user_main to probably just set a done bit,
    //       orthogonal to the cbarrier
    //CHECK( !cb_done ) << "cannot call wait on a finished barrier";
    if ( !cb_done ) {

        enter_cbarrier_request_args enargs = { SoftXMT_mynode() };
        SoftXMT_call_on( HOME_NODE, &enter_cbarrier_request_am, &enargs );

        wakeme = CURRENT_THREAD;

        SoftXMT_suspend( );
        //CHECK( cb_reply ) << "waiter woke without cb_reply=1";

        cb_reply = false;
    }
    
    return cb_done;
}

void cbarrier_init( Node num_nodes ) {
    num_barrier_clients = num_nodes;
    num_waiting_clients = 0;
    
    if ( SoftXMT_mynode() == HOME_NODE ) {
        waiters = new std::list <Node>();
        notified = new bool[SoftXMT_nodes()];
        for (Node i=0; i<SoftXMT_nodes(); i++) {
            notified[i] = false;
        }
    }

    cb_done = false;
    cb_reply = false;
    wakeme = NULL;
}

/// cancel just this Node being in barrier
/// It is okay that the waiter list is a superset
/// of things actually waiting. This is because
/// user_main should not exit until things are done.
void cbarrier_cancel_local( ) {
   if ( wakeme ) {
        SoftXMT_wake( wakeme );
        wakeme = NULL;
    }
}
