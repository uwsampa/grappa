#include "Scheduler.hpp"


void Scheduler::run ( ) {
    while (thread_wait( NULL ) != NULL) { } // nothing
}

void Scheduler::thread_join( thread* wait_on ) {
    if ( !wait_on->done ) {
        wait_on->joinqueue.enqueue( current_thread );
        thread_suspend( );
    }
}


thread * Scheduler::thread_wait( void **result ) {
    CHECK( current_thread == master ) << "only meant to be called by system thread";

    thread * next = nextCoroutine( false );
    if (next == NULL) {
        // no user threads remain
        return NULL;
    } else {
        current_thread = next;

        thread * died = (thread *) thread_context_switch( master, next, NULL);

        // At the moment, we only return from a thread in the case of death.

        if (result != NULL) {
            void *retval = (void *)died->next;
            *result = retval;
        }
        return died;
    }
}

