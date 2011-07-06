#include <assert.h>
#include "thread.h"
#include "linked_list-walk.h"
#include "SplitPhase.hpp"

int64_t walk_split_phase(thread* me, SplitPhase* sp, int64_t base, uint64_t listsize, uint64_t num_lists) {
    int64_t sum = 0;

    if (num_lists==1) {
        int64_t index = base;
        uint64_t count = listsize;

        while (count-- > 0) {
            int64_t next_index = ((index<<6)>>3) + 1; // next ptr is offset 1
            mem_tag_t tag = sp->issue(READ, next_index, 0, me);
            thread_yield(me);
            index = sp->complete(tag, me); 
        }

        sum += index;
    } else {
        assert(false);
    }
    
    sp->unregister(me);
    return sum;
}

