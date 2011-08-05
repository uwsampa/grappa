#include <upc_relaxed.h>
#include <stdio.h>
#include <stdint.h>
#include "vertex.h"

#define NUM_VERTICES_PER_THREAD 1024

shared vertex vertices[NUM_VERTICES_PER_THREAD * THREADS];

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
    
    // initialize vertices 
    int i;
    upc_forall(i=0; i<THREADS; i++; i) {
        for (int j=0; j<num_vertices_per_thread-1; j++) {
            vertices[j*THREADS+i].next = &vertices[(j+1)*THREADS+i];       // local linked lists
        }
    }

    upc_barrier;

    // do the work
    uint64_t result;
    upc_forall(i=0; i<THREADS; i++; i) {
        shared vertex* myvertex = &vertices[i];  // starting points are first THREADS elements
        uint64_t count = num_vertices_per_thread;
        while (--count > 0) {
            myvertex = myvertex->next;
        }
        result = (uint64_t) myvertex;
    }

    printf("thread%d result is %lu\n", MYTHREAD, result);
}
        
