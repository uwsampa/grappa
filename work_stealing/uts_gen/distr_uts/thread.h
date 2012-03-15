#ifndef THREAD_HPP
#define THREAD_HPP

#include "coro.h"


class ThreadQueue;
class Scheduler;
typedef uint32_t threadid_t; 
typedef void (*thread_func)(thread *, void *arg);

typedef struct thread {
  coro *co;
  // Scheduler responsible for this thread. NULL means this is a system
  // thread.
  Scheduler * sched;
  thread * next; // for queues--if we're not in one, should be NULL
  threadid_t id; 
  ThreadQueue joinqueue;
  int done;
} thread;

thread * thread_spawn ( thread * me, Scheduler * sched, thread_func f, void * arg);

// Turns the current (system) thread into a green thread.
thread * thread_init();

inline void* thread_context_switch( thread * running, thread * next, void * val ) {
    return coro_invoke( running->co, next->co, val );
}

inline int thread_is_running( thread * thr ) {
    return thr->co->running;
}

// Die and return to the master thread.
void thread_exit(thread *me, void *retval);

// Delete a thread.
void destroy_thread(thread *thr);

class ThreadQueue {
    private:
        thread* head;
        thread* tail;

    public:
        ThreadQueue ( ) 
            : head ( NULL ),
            : tail ( NULL ) { }

        thread* dequeue() {
            thread* result = head;
            if (result != NULL) {
                head = result->next;
                result->next = NULL;
            } else {
                tail = NULL;
            }
            return result;
        }

        void enqueue(thread* t) {
            if (head==NULL) {
                head = t;
            } else {
                tail->next = t;
            }
            tail = thread;
            t->next = NULL;
        }
};
