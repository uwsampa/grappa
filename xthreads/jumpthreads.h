
#ifndef __jumpthreads_h__
#define __jumpthreads_h__


/*typedef struct jthr_memdesc {
	void* address;
	uint64_t data;
} jthr_memdesc;
*/

typedef struct jthr_thread {
    void* returnAddress;
    struct jthr_thread* next;
} jthr_thread;

typedef struct jthr_sched {
    jthr_thread* head;
    jthr_thread* tail;
} jthr_sched;

void jthr_sched_init(jthr_sched* sched) {
    sched->head = 0;
    sched->tail = 0;
}

void jthr_thread_init(jthr_thread* th) {
    th->returnAddress = 0;
    th->next = 0;
}

inline void jthr_enqueue(jthr_sched* sched, jthr_thread* me, void* returnAddress) {
    me->returnAddress = returnAddress;
    if (sched->head!=0) {
        sched->tail->next = me;
    } else {
        sched->head = me;
    }
    sched->tail = me;

    //Assumes me->next is does not need to be set
}

inline void* jthr_dequeue(jthr_sched* sched) {
    jthr_thread* ja = sched->head;
    sched->head = ja->next; // don't need to change tail since head==null => tail==null
    ja->next = 0;
    return ja->returnAddress;
}
    


#endif
