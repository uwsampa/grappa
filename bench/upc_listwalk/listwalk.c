#include <upc_relaxed.h>
#include <stdio.h>
#include <stdint.h>
#include "vertex.h"

#define NUM_VERTICES_PER_LIST (1<<10)
#define NUM_LISTS_PER_THREAD 2
#define SHUFFLE_LISTS 1
#define SEQUENTIAL_SHUFFLE 1

shared vertex vertices[NUM_VERTICES_PER_LIST * THREADS * NUM_LISTS_PER_THREAD];

void printVertices(const char* title) {
    if (MYTHREAD==0) {
        printf("%s:[", title);
        for (int i=0; i<NUM_VERTICES_PER_LIST*THREADS; i++) {
            printf(" (%lu, %lu)", vertices[i].id, vertices[i].next->id);
        }
        printf(" ]\n");
    }
}

void printArray(const char* title, shared uint64_t* buf, uint64_t len) {
    if (MYTHREAD==0) {
        printf("%s:[", title);
        for (int i=0; i<len; i++) {
            printf(" %lu", buf[i]);
        }
        printf(" ]\n");
    }
}

void allocate_lists(shared vertex* vs, shared vertex** myhead, uint64_t num_vertices, uint64_t num_vertices_per_list, uint64_t num_lists_per_thread) {
    // initialize vertices 
    uint64_t j;
    upc_forall(j=0; j<num_vertices-1; j++; j) {
        vs[j].id = j;
        vs[j].next = &vs[j+1];       // just link in index order so access order is round robin accross nodes
    }
    // link last
    vs[num_vertices-1].id = num_vertices-1;
    vs[num_vertices-1].next = &vs[0];

    uint64_t num_vertices_per_thread = num_vertices_per_list * num_lists_per_thread;


    #if SHUFFLE_LISTS
        upc_barrier;

//printVertices("after init, point to next");
//upc_barrier;
        
       
     #if SEQUENTIAL_SHUFFLE
        if (MYTHREAD==0) {
            uint64_t local_end = num_vertices-1;
            uint64_t local_begin = 0;
            uint64_t stride = 1;
     #else  
        {
        // shuffle (not sound since multiple unsynchronized accessors)
        uint64_t local_end = THREADS*(num_vertices_per_thread-1)+MYTHREAD;
        uint64_t local_begin = MYTHREAD;
        uint64_t stride = THREADS;
     #endif
        for (uint64_t i = local_end; i != local_begin; i-=stride) {   // THREAD stride because blocksize=1
            uint64_t j = random() % i;

            vertex temp1 = vs[i];
            vertex temp2 = vs[j];

            vs[i] = temp2;
            vs[j] = temp1;
        }
     } 
        upc_barrier;
      
//printVertices("after shuffle");
//upc_barrier;
        
        // get the locations of the ids
        shared uint64_t* locs = (shared uint64_t*)upc_all_alloc(THREADS, sizeof(uint64_t)*num_vertices_per_thread);
        upc_forall(j=0; j<num_vertices; j++; j) {
            locs[vs[j].id] = j;
        }
        upc_barrier;

//printArray("locs",locs, num_vertices); 
//upc_barrier;

        // create cycles of length num_vertices_per_thread (currently all chained together)
        upc_forall(j=0; j<num_vertices; j++; j) {
            vs[j].next = &vs[locs[(vs[j].id+1)%num_vertices]];
        }
       
        // starting location is the vertex with this threads starting id,
        // where array is chunked into contiguous pieces
        for (j=0; j<num_lists_per_thread; j++) {
            myhead[j] = &vs[locs[num_vertices_per_thread*MYTHREAD+j*num_vertices_per_list]];
        }

        upc_barrier;
        if (MYTHREAD==0)  upc_free(locs);

//upc_barrier;
//printVertices("after cycles");
//upc_barrier;

    #else 
        *myhead = &vs[MYTHREAD];
    #endif
}


int main(int argc, char** argv) {
    const uint64_t num_vertices_per_list = NUM_VERTICES_PER_LIST;
    const uint64_t num_vertices = num_vertices_per_list * THREADS * NUM_LISTS_PER_THREAD;
/*
    if (MYTHREAD==0) {
        for (int i=0; i<num_vertices_per_list; i++) {
            printf("threadof(vertex[%d])=%lu\n", i, upc_threadof(vertices + i));
        }
    }
    printf("%d threadof(vertices)=%lu\n", MYTHREAD, upc_threadof(vertices+num_vertices_per_list+8));

    upc_barrier;

    if (MYTHREAD==1) {
        for (int i=0; i<num_vertices_per_list; i++) {
            printf("threadof(vertex[%d])=%lu\n", i, upc_threadof(vertices + i));
        }
    }

    upc_barrier;
  */  
    shared vertex* myhead[NUM_LISTS_PER_THREAD];
    allocate_lists(vertices, myhead, num_vertices, num_vertices_per_list, NUM_LISTS_PER_THREAD); 

    upc_barrier;
    if (MYTHREAD==0) printf("starting the traversals\n");
    upc_barrier;

    // do the work
    uint64_t result;
    uint64_t count = num_vertices_per_list;
    uint64_t global_count = 0;
    if (NUM_LISTS_PER_THREAD==1) {
        shared vertex* myvertex = myhead[0];
        while(--count > 0) {
            global_count = (upc_threadof(myvertex)!=MYTHREAD) ? global_count+1 : global_count;
            myvertex = myvertex->next;
            //printf("thread %d: id:%lu\n", MYTHREAD, myvertex->id);
        }
        result = myvertex->id;
    } else if (NUM_LISTS_PER_THREAD==2) {
        shared vertex* myvertex1 = myhead[0];
        shared vertex* myvertex2 = myhead[1];
        while(--count > 0) {
            global_count = (upc_threadof(myvertex1)!=MYTHREAD) ? global_count+1 : global_count;
            global_count = (upc_threadof(myvertex2)!=MYTHREAD) ? global_count+1 : global_count;
            myvertex1 = myvertex1->next;
            myvertex2 = myvertex2->next;
            //printf("thread %d: id:%lu\n", MYTHREAD, myvertex->id);
        }
        result = myvertex1->id + myvertex2->id;
    }

    printf("thread%d -- result:%lu, global_refs:%lu (%f)\n", MYTHREAD, result, global_count, (double)global_count/(num_vertices_per_list*NUM_LISTS_PER_THREAD));
}
        
