#include <stdio.h>
#include <assert.h>
#include <stddef.h>
#include "node.hpp"
#include "thread.h"
#include "linked_list-walk.h"
#include "SplitPhase.hpp"

#include <gasnet.h>

uint64_t walk_split_phase(thread* me, SplitPhase* sp, global_array* vertices, uint64_t head, uint64_t listsize, uint64_t num_lists) {

    //GASNET_BEGIN_FUNCTION(); // but this probably might not even help for inlined stuff containing gasnet calls
    gasnet_threadinfo_t thr_info = GASNET_GET_THREADINFO();


    int64_t sum = 0;

    if (num_lists==1) {
        uint64_t index = head;
        uint64_t count = listsize;

        while (count-- > 0) {
            global_address a;
            ga_index(vertices, index, &a);
            a.offset += offsetof(node, next); //select 'next' field

            mem_tag_t tag = sp->issue(READ, a, 0, me, thr_info);
            thread_yield(me);
            index = sp->complete(tag, me, thr_info); 
        }

        sum += index;
    } else {
        printf("%lu lists unsupported\n", num_lists);
        assert(false);
    }
    
    sp->unregister(me);
    return sum;
}

