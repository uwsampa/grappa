#include <upc_relaxed.h>
#include <stdio.h>
#include <stdint.h>
#include "vertex.h"

#define NUM_VERTICES_PER_THREAD (1<<10)
#define SHUFFLE_LISTS 1
#define SEQUENTIAL_SHUFFLE 1

shared vertex vertices[NUM_VERTICES_PER_THREAD * THREADS];

void printVertices(const char* title) {
    if (MYTHREAD==0) {
        printf("%s:[", title);
        for (int i=0; i<NUM_VERTICES_PER_THREAD*THREADS; i++) {
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

void allocate_lists(shared vertex* vs, shared vertex** myhead, uint64_t num_vertices, uint64_t num_vertices_per_thread) {
    // initialize vertices 
    uint64_t j;
    upc_forall(j=0; j<num_vertices-1; j++; j) {
        vs[j].id = j;
        vs[j].next = &vs[j+1];       // just link in index order so access order is round robin accross nodes
    }
    // link last
    vs[num_vertices-1].id = num_vertices-1;
    vs[num_vertices-1].next = &vs[0];


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

        // create cycles of length num_vertices_per_thread
        upc_forall(j=0; j<num_vertices; j++; j) {
            vs[j].next = &vs[locs[(vs[j].id+1)%num_vertices]];
        }
       
        // starting location is the vertex with this threads starting id,
        // where array is chunked into contiguous pieces
        *myhead = &vs[locs[num_vertices_per_thread*MYTHREAD]];

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
    const uint64_t num_vertices_per_thread = NUM_VERTICES_PER_THREAD;
    const uint64_t num_vertices = num_vertices_per_thread * THREADS;
/*
    if (MYTHREAD==0) {
        for (int i=0; i<num_vertices_per_thread; i++) {
            printf("threadof(vertex[%d])=%lu\n", i, upc_threadof(vertices + i));
        }
    }
    printf("%d threadof(vertices)=%lu\n", MYTHREAD, upc_threadof(vertices+num_vertices_per_thread+8));

    upc_barrier;

    if (MYTHREAD==1) {
        for (int i=0; i<num_vertices_per_thread; i++) {
            printf("threadof(vertex[%d])=%lu\n", i, upc_threadof(vertices + i));
        }
    }

    upc_barrier;
  */  
    shared vertex* myhead;
    allocate_lists(vertices, &myhead, num_vertices, num_vertices_per_thread); 

    upc_barrier;
    if (MYTHREAD==0) printf("starting the traversals\n");
    upc_barrier;

    // do the work
    uint64_t result;
    uint64_t count = num_vertices_per_thread;
    uint64_t global_count = 0;
    shared vertex* myvertex = myhead;
    while(--count > 0) {
        global_count = (upc_threadof(myvertex)!=MYTHREAD) ? global_count+1 : global_count;
        myvertex = myvertex->next;
        //printf("thread %d: id:%lu\n", MYTHREAD, myvertex->id);
    }
    result = myvertex->id;

    printf("thread%d -- result:%lu, global_refs:%lu (%f)\n", MYTHREAD, result, global_count, (double)global_count/num_vertices_per_thread);
}
        
