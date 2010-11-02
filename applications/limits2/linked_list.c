
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <time.h>
#include <string.h>

#include <omp.h>

#include "node.h"
#include "ExperimentRunner.hpp"

#define NPROC 1
#define BITS 23

#define SIZE (1 << BITS)
//#define SIZE ((1 << BITS) + (1 << (BITS-1)))

typedef struct node {
	long unsigned id;
	struct node* next;
	char pad[56];
} node;

void print( node nodes[], int size ) {
  printf("[\n");
  uint64_t i = 0;
  for( i = 0; i < size; ++i ) {
    printf("%p: { id: %lu next: %p }\n", &nodes[i], nodes[i].id, nodes[i].next);
  }
  printf("]\n");
}

//void initshuffle(node nodes[] ) {//, node* locs[]) {
// initialize linked lists.
node** initshuffle(uint64_t size, int num_threads, int num_refs) { //node nodes[] ) {//, node* locs[]) {
  int64_t i = 0;
  //size = size / num_threads;

  node** locs = (node**)malloc(sizeof(node*) * size); // node array
  node* nodes = (node*)malloc(sizeof(node) * size);  // temporary node pointers
  node** bases = (node**)malloc(sizeof(node*) * (num_threads*num_refs)); // initial node array

  // initialize ids
  #pragma omp parallel for num_threads(num_threads)
  for(i = 0; i < size; ++i) {
    nodes[i].id = i;
  }

  // randomly permute nodes (fisher-yates (or knuth) shuffle)
  for(i = size-1; i > 0; --i) {
   uint64_t j = random() % i;
    node temp = nodes[i];
    nodes[i] = nodes[j];
    nodes[j] = temp;
  }

  // extract locs
  #pragma omp parallel for num_threads(num_threads)
  for(i = 0; i < size; ++i) {
    locs[ nodes[i].id ] = &nodes[i];
  }

  // initialize pointers
  // chop id-space into num_threads*num_refs sections and build a circular list for each
  int64_t base = 0;
  #pragma omp parallel for num_threads(num_threads)
  for(i = 0; i < size; ++i) {
    uint64_t id = nodes[i].id;
    int64_t ref_size = size / (num_threads*num_refs);
    int64_t base = id / ref_size;
    uint64_t nextid = nodes[i].id + 1;
    uint64_t wrapped_nextid = (nextid % ref_size) + (base * ref_size);
    //printf("%d: %d %d %d\n", id, base, nextid, wrapped_nextid);
    nodes[i].next = locs[ wrapped_nextid ];
  }

  // grab initial pointers for each list
  // (i.e. thread i will get chains starting at base[i*num_refs],...,base[(i+1)*num_refs-1])
  for(i = 0; i < (num_threads*num_refs); ++i) {
    bases[i] = locs[i * size / (num_threads*num_refs)];
  }

  //print(nodes, size);
  
  free(locs);
  return bases;
}


// walk the list
uint64_t walk( node* bases[], uint64_t count, int num_refs, int start_index ) {
  uint64_t sum = 0;
  int si = start_index;
//printf("thread s\n");
  if (num_refs==1) {
  	node* i = bases[si];

  	while (count > 0) {
    		i = i->next;
   		 count--;
  	}
    	sum += (uint64_t) i; // do some work to make the optimizer happy

   } else if (num_refs==2) { 
	node* i0 = bases[si];
	node* i1 = bases[si+1];
   
	while (count > 0) {
		i0 = i0->next; i1 = i1->next;
		count--;
	} 
	sum += (uint64_t)i0 +  (uint64_t)i1;

   } else if (num_refs==3) {	
	node* i0 = bases[si];
	node* i1 = bases[si+1];
	node* i2 = bases[si+2];   

	while (count > 0) {
		i0 = i0->next; i1 = i1->next; i2 = i2->next;
		count--;
	}
 	sum += (uint64_t)i0 +  (uint64_t)i1 +  (uint64_t)i2;
   
   } else if (num_refs==4) {
	
	node* i0 = bases[si];
	node* i1 = bases[si+1];
	node* i2 = bases[si+2];
	node* i3 = bases[si+3];  
 
	while (count > 0) {
		i0 = i0->next; i1 = i1->next; i2 = i2->next; i3 = i3->next;
		count--;
	} 
	sum += (uint64_t)i0 +  (uint64_t)i1 +  (uint64_t)i2 +  (uint64_t)i3;
   
   } else if (num_refs==5) {
	node* i0 = bases[si];
	node* i1 = bases[si+1];
	node* i2 = bases[si+2];
	node* i3 = bases[si+3];  
	node* i4 = bases[si+4];
 
	while (count > 0) {
		i0 = i0->next; i1 = i1->next; i2 = i2->next; i3 = i3->next;
                i4 = i4->next;
		count--;
	}
	sum += (uint64_t)i0 +  (uint64_t)i1 +  (uint64_t)i2 +  (uint64_t)i3
                        + (uint64_t)i4;
   
   } else if (num_refs==6) {
	node* i0 = bases[si];
	node* i1 = bases[si+1];
	node* i2 = bases[si+2];
	node* i3 = bases[si+3];  
	node* i4 = bases[si+4];
	node* i5 = bases[si+5];
 
	while (count > 0) {
		i0 = i0->next; i1 = i1->next; i2 = i2->next; i3 = i3->next;
                i4 = i4->next; i5 = i5->next; 
		count--;
	}
	sum += (uint64_t)i0 +  (uint64_t)i1 +  (uint64_t)i2 +  (uint64_t)i3
                        + (uint64_t)i4 + (uint64_t)i5;

   } else if (num_refs==7) {
	node* i0 = bases[si];
	node* i1 = bases[si+1];
	node* i2 = bases[si+2];
	node* i3 = bases[si+3];  
	node* i4 = bases[si+4];
	node* i5 = bases[si+5];
	node* i6 = bases[si+6];
 
	while (count > 0) {
		i0 = i0->next; i1 = i1->next; i2 = i2->next; i3 = i3->next;
                i4 = i4->next; i5 = i5->next; i6 = i6->next; 
		count--;
	}
	sum += (uint64_t)i0 +  (uint64_t)i1 +  (uint64_t)i2 +  (uint64_t)i3
                        + (uint64_t)i4 + (uint64_t)i5 + (uint64_t)i6;
   
   } else if (num_refs==8) {
	node* i0 = bases[si];
	node* i1 = bases[si+1];
	node* i2 = bases[si+2];
	node* i3 = bases[si+3];  
	node* i4 = bases[si+4];
	node* i5 = bases[si+5];
	node* i6 = bases[si+6];
	node* i7 = bases[si+7];
 
	while (count > 0) {
		i0 = i0->next; i1 = i1->next; i2 = i2->next; i3 = i3->next;
                i4 = i4->next; i5 = i5->next; i6 = i6->next; i7 = i7->next;
		count--;
	}
	sum += (uint64_t)i0 +  (uint64_t)i1 +  (uint64_t)i2 +  (uint64_t)i3
                        + (uint64_t)i4 + (uint64_t)i5 + (uint64_t)i6 + (uint64_t)i7;

    } else if (num_refs==9) {
	node* i0 = bases[si];
	node* i1 = bases[si+1];
	node* i2 = bases[si+2];
	node* i3 = bases[si+3];  
	node* i4 = bases[si+4];
	node* i5 = bases[si+5];
	node* i6 = bases[si+6];
	node* i7 = bases[si+7];
	node* i8 = bases[si+8];
 
	while (count > 0) {
		i0 = i0->next; i1 = i1->next; i2 = i2->next; i3 = i3->next;
                i4 = i4->next; i5 = i5->next; i6 = i6->next; i7 = i7->next;
		i8 = i8->next;
		count--;
	}
	sum += (uint64_t)i0 +  (uint64_t)i1 +  (uint64_t)i2 +  (uint64_t)i3
                        + (uint64_t)i4 + (uint64_t)i5 + (uint64_t)i6 + (uint64_t)i7
			+ (uint64_t)i8;

    } else if (num_refs==10) {
	node* i0 = bases[si];
	node* i1 = bases[si+1];
	node* i2 = bases[si+2];
	node* i3 = bases[si+3];  
	node* i4 = bases[si+4];
	node* i5 = bases[si+5];
	node* i6 = bases[si+6];
	node* i7 = bases[si+7];
	node* i8 = bases[si+8];
	node* i9 = bases[si+9];
 
	while (count > 0) {
		i0 = i0->next; i1 = i1->next; i2 = i2->next; i3 = i3->next;
                i4 = i4->next; i5 = i5->next; i6 = i6->next; i7 = i7->next;
		i8 = i8->next; i9 = i9->next;
		count--;
	}
	sum += (uint64_t)i0 +  (uint64_t)i1 +  (uint64_t)i2 +  (uint64_t)i3
                        + (uint64_t)i4 + (uint64_t)i5 + (uint64_t)i6 + (uint64_t)i7
			+ (uint64_t)i8 + (uint64_t)i9;

    } else if (num_refs==11) {
	node* i0 = bases[si];
	node* i1 = bases[si+1];
	node* i2 = bases[si+2];
	node* i3 = bases[si+3];  
	node* i4 = bases[si+4];
	node* i5 = bases[si+5];
	node* i6 = bases[si+6];
	node* i7 = bases[si+7];
	node* i8 = bases[si+8];
	node* i9 = bases[si+9];
	node* i10 = bases[si+10];
 
	while (count > 0) {
		i0 = i0->next; i1 = i1->next; i2 = i2->next; i3 = i3->next;
                i4 = i4->next; i5 = i5->next; i6 = i6->next; i7 = i7->next;
		i8 = i8->next; i9 = i9->next; i10 = i10->next;
		count--;
	}
	sum += (uint64_t)i0 +  (uint64_t)i1 +  (uint64_t)i2 +  (uint64_t)i3
                        + (uint64_t)i4 + (uint64_t)i5 + (uint64_t)i6 + (uint64_t)i7
			+ (uint64_t)i8 + (uint64_t)i9 + (uint64_t)i10;

     } else if (num_refs==12) {
	node* i0 = bases[si];
	node* i1 = bases[si+1];
	node* i2 = bases[si+2];
	node* i3 = bases[si+3];  
	node* i4 = bases[si+4];
	node* i5 = bases[si+5];
	node* i6 = bases[si+6];
	node* i7 = bases[si+7];
	node* i8 = bases[si+8];
	node* i9 = bases[si+9];
	node* i10 = bases[si+10];
	node* i11 = bases[si+11];
 
	while (count > 0) {
		i0 = i0->next; i1 = i1->next; i2 = i2->next; i3 = i3->next;
                i4 = i4->next; i5 = i5->next; i6 = i6->next; i7 = i7->next;
		i8 = i8->next; i9 = i9->next; i10 = i10->next; i11 = i11->next;
		count--;
	}
	sum += (uint64_t)i0 +  (uint64_t)i1 +  (uint64_t)i2 +  (uint64_t)i3
                        + (uint64_t)i4 + (uint64_t)i5 + (uint64_t)i6 + (uint64_t)i7
			+ (uint64_t)i8 + (uint64_t)i9 + (uint64_t)i10 + (uint64_t)i11;
     } 
  return sum;
}


int main(int argc, char* argv[]) {

  assert(argc >= 4);
  uint64_t bits = atoi(argv[1]);            // 2^bits=number of nodes
  uint64_t num_threads = atoi(argv[2]);     // number of threads
  uint64_t size = (1 << bits);              

  uint64_t concurrent_refs = atoi(argv[3]); // number of concurrent accesses per thread	

  // optional 4th arg "-i" indicates to use ExperimentRunner
  bool do_monitor = false;
  if (argc==5) {
	if (strncmp("-i", argv[4], 2)==0) {
		do_monitor = true;
        }
   }

  printf("Initializing footprint of size %lu * %lu = %lu bytes....\n", size, sizeof(node), size * sizeof(node));
  // initialize random number generator
  srandom(time(NULL));
  //print( data );


  int n;

  node** data = initshuffle( size, num_threads, concurrent_refs );

  const uint64_t multiplier = 10;

  // experiment runner init and enable
  ExperimentRunner er;
  if (do_monitor) er.enable();


  for(n = 0; n < multiplier; n++) {


    //print( data );
    struct timespec start;
    struct timespec end;

    // start monitoring and timing    
    er.startIfEnabled();
    clock_gettime(CLOCK_MONOTONIC, &start);
    //printf("Start time: %d seconds, %d nanoseconds\n", (int) start.tv_sec, (int) start.tv_nsec);
    int m;
    uint64_t base = 0; 
    const uint64_t procsize = size / (num_threads*concurrent_refs); // size of each chain
    uint64_t* results = (uint64_t*)malloc( sizeof(uint64_t) * num_threads); 
#pragma omp parallel for num_threads(num_threads)
    for(m = 0; m < num_threads; m++) {
  //    printf("num_threads %d\n", omp_get_num_threads());    // XXX does print make some serial b/c all try to access IO?
      //node* d = data[m];
      results[m] = walk( data, procsize, concurrent_refs, m*concurrent_refs);
      //results[m] = walk( d + (m*procsize), procsize*multiplier );
    }

    // stop timing and monitoring
    clock_gettime(CLOCK_MONOTONIC, &end);
    er.stopIfEnabled();
    //printf("End time: %d seconds, %d nanoseconds\n", (int) end.tv_sec, (int) end.tv_nsec);

    uint64_t result = 0;
    for(m = 0; m < num_threads; m++) {
      result += results[m];
    }

    const uint64_t totalsize = procsize * num_threads * concurrent_refs;
    
    // this assert can be wrong because of truncation; test case: 22 1 9
    printf("assert %u %u %u\n", totalsize, size, size-totalsize);
    //if (num_threads == 1) assert(totalsize == size);

    uint64_t runtime_ns = ((uint64_t) end.tv_sec * 1000000000 + end.tv_nsec) - ((uint64_t) start.tv_sec * 1000000000 + start.tv_nsec);
    double runtime_s = (double)runtime_ns / 1000000000.0;
    const uint64_t total_bytes = totalsize * sizeof(node);
    const uint64_t proc_bytes = procsize * sizeof(node);
    const uint64_t total_requests = totalsize;
    const uint64_t proc_requests = procsize;
    const double   request_rate = (double)total_requests / runtime_s;
    const double   proc_request_rate = (double)proc_requests / runtime_s;
    const double   data_rate = (double)total_bytes / runtime_s;
    const double   proc_data_rate = (double)proc_bytes / runtime_s;
    const double   proc_request_time = (double)runtime_ns / proc_requests;
    const double   proc_request_cpu_cycles = proc_request_time / (1.0 / 2.27);

    printf("{ bits:%lu num_threads:%lu total_bytes:%lu proc_bytes:%lu total_requests:%lu proc_requests:%lu request_rate:%f proc_request_rate:%f data_rate:%f proc_data_rate:%f proc_request_time:%f proc_request_cpu_cycles:%f }\n",
	   bits, num_threads, total_bytes, proc_bytes, total_requests, proc_requests, 
	   request_rate, proc_request_rate, data_rate, proc_data_rate, proc_request_time, proc_request_cpu_cycles);

    printf("(%lu/%lu): %lu nanoseconds, %lu requests, %f req/s, %f req/s/proc, %f ns/req, %f B/s, %f clocks each\n", 
	   bits, num_threads,
	   runtime_ns, totalsize, (double)totalsize/((double)runtime_ns/1000000000), 
	   (double)totalsize/((double)runtime_ns/1000000000)/num_threads, 
	   (double)runtime_ns/totalsize, 
	   (double)totalsize*sizeof(node)/((double)runtime_ns/1000000000),
	   ((double)runtime_ns/(totalsize/num_threads)) / (.00000000044052863436 * 1000000000) 
	   );
  }
  //print( data );

  return 0;
}
