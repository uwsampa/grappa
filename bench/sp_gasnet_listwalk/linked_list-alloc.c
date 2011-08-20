#include <stdio.h>
#include "linked_list-alloc.h"
#include "linked_list-def.h"
#include "node.hpp"
#include <gasnet.h>

#define SHUFFLE_LISTS 1
#define GLOBAL_SHUFFLE 1

uint64_t get_long(global_array* ga, uint64_t index) {
    global_address gaddr;
    ga_index(ga, index, &gaddr);
    if (gm_is_address_local(&gaddr)) {
        uint64_t* v = (uint64_t*) gm_local_gm_address_to_local_ptr(&gaddr);
        return *v;
    } else {
        void* srcptr = gm_address_to_ptr(&gaddr);
        int nodeid = gm_node_of_address(&gaddr);
        uint64_t res;
        gasnet_get (&res, nodeid, srcptr, sizeof(uint64_t));
        return res;
    }
}

void put_long(global_array* ga, uint64_t index, uint64_t data) {
    global_address gaddr;
    ga_index(ga, index, &gaddr);
    if (gm_is_address_local(&gaddr)) {
        uint64_t* v = (uint64_t*) gm_local_gm_address_to_local_ptr(&gaddr);
        *v = data;
    } else {
        void* destptr = gm_address_to_ptr(&gaddr);
        int nodeid = gm_node_of_address(&gaddr);
        gasnet_put (nodeid, destptr, &data, sizeof(uint64_t));
    }
}

node get_vertex(global_array* ga, uint64_t index) {
    global_address gaddr;
    ga_index(ga, index, &gaddr);
    if (gm_is_address_local(&gaddr)) {
        node* v = (node*) gm_local_gm_address_to_local_ptr(&gaddr);
        return *v;
    } else {
        void* srcptr = gm_address_to_ptr(&gaddr);
        int nodeid = gm_node_of_address(&gaddr);
        node res;
        gasnet_get (&res, nodeid, srcptr, sizeof(node));
        return res;
    }
}


void put_vertex(global_array* ga, uint64_t index, node data) {
    global_address gaddr;
    ga_index(ga, index, &gaddr);
    if (gm_is_address_local(&gaddr)) {
        node* v = (node*) gm_local_gm_address_to_local_ptr(&gaddr);
        *v = data;
    } else {
        void* destptr = gm_address_to_ptr(&gaddr);
        int nodeid = gm_node_of_address(&gaddr);
        gasnet_put (nodeid, destptr, &data, sizeof(node));
    }
}


void allocate_lists(global_array* vertices, uint64_t num_vertices, uint64_t startIndex, uint64_t endIndex, int myrank, uint64_t* localHeads, uint64_t num_lists_per_node, uint64_t num_vertices_per_list ) {

    // initialize vertices
    for (uint64_t i=startIndex; i!=endIndex; i++) {
        global_address a;
        ga_index(vertices, i, &a);  
        node* np = (node*) gm_local_gm_address_to_local_ptr(&a);
        np->id = i;
        np->next = i+1;
    }
    // do last link properly (avoid modulo above)
    {
        global_address a;
        ga_index(vertices, num_vertices-1, &a);
        if (gm_is_address_local(&a)) {
            node* np = (node*) gm_local_gm_address_to_local_ptr(&a);
            np->next = 0;
        }
    }


    #if SHUFFLE_LISTS
        WAIT_BARRIER;

        #if GLOBAL_SHUFFLE
            if (0==myrank) {
                uint64_t shuf_start = 0;
                uint64_t shuf_end = num_vertices-1;
        
        #else
        {
            // to get all vertices shuffled within a node, allocation assume contiguous chunks (i.e. blocksize = num_vertices/num_nodes)
            uint64_t shuf_start = startIndex;
            uint64_t shuf_end = endIndex;
        #endif

            for (uint64_t i=shuf_end; i!=shuf_start; i--) {
                uint64_t j = (random() % (i-shuf_start)) + shuf_start;
                        
                node temp1 = get_vertex(vertices, i);
                node temp2 = get_vertex(vertices, j);

                put_vertex(vertices, i, temp2);
                put_vertex(vertices, j, temp1);
            }
        }
        WAIT_BARRIER;

        // get the locations of the ids
        global_array* locs = ga_allocate(sizeof(uint64_t), num_vertices);

        for (uint64_t i=startIndex; i<=endIndex; i++) {
            node v = get_vertex(vertices, i);
            put_long(locs, v.id, i);
        }

        WAIT_BARRIER;

        
        // chain together (TODO cycles)
        for (uint64_t i=startIndex; i<=endIndex; i++) {
            node vi = get_vertex(vertices, i);
            uint64_t locindex = (vi.id+1)%num_vertices;
            uint64_t l = get_long(locs, locindex);
            vi.next = l;
            put_vertex(vertices, i, vi);
        }

        for (uint64_t i=0; i<num_lists_per_node; i++) {
            localHeads[i] = get_long(locs, startIndex+i*num_vertices_per_list);
        }

        WAIT_BARRIER;

        /* TODO
         * ga_free(locs); */

    #else  // (not SHUFFLE_LISTS)
        for (uint64_t i=0; i<num_lists_per_node; i++) {
            localHeads[i] = startIndex+i*num_vertices_per_list;
        }

    #endif // SHUFFLE_LISTS
}





