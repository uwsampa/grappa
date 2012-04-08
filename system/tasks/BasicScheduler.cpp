#include "BasicScheduler.hpp"

/// TODO: this should be based on some actual time-related metric so behavior is predictable across machines
DEFINE_int64( periodic_poll_ticks, 500, "number of ticks to wait before polling periodic queue");

void BasicScheduler::run ( ) {
    while (thread_wait( NULL ) != NULL) { } // nothing
}

void BasicScheduler::thread_join( Thread* wait_on ) {
    while ( !wait_on->done ) {
        wait_on->joinqueue.enqueue( current_thread );
        thread_suspend( );
    }
}


Thread * BasicScheduler::thread_wait( void **result ) {
    CHECK( current_thread == master ) << "only meant to be called by system Thread";

    Thread * next = nextCoroutine( false );
    if (next == NULL) {
        // no user threads remain
        return NULL;
    } else {
        current_thread = next;

        Thread * died = (Thread *) thread_context_switch( master, next, NULL);

        // At the moment, we only return from a Thread in the case of death.

        if (result != NULL) {
            void *retval = (void *)died->next;
            *result = retval;
        }
        return died;
    }
}


