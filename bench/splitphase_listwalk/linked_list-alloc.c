#include "linked_list-node.h"
#include "linked_list-alloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <numa.h>

#define DEBUG 0

node** allocate_page(uint64_t size, int num_threads, int num_lists_per_thread) {

  size = size / (num_threads * num_lists_per_thread);
  printf("Chain size is %lu * %lu = %lu\n", 
	 size, sizeof(node), size *sizeof(node));
  printf("Initializing footprint of size %d * %d * %lu * %lu = %d * %lu = %lu bytes....\n", 
	 num_threads, num_lists_per_thread, size, sizeof(node), 
	 num_threads * num_lists_per_thread, size * sizeof(node), 
	 num_threads * num_lists_per_thread * size * sizeof(node));

/** Page size choices **/
  //unsigned page_size = 2 * 1024 * 1024;        // for huge pages (2 MiB)
  //unsigned page_size = 1024*1024*1024;           // for huge pages (1 GiB)
  unsigned page_size = sysconf(_SC_PAGESIZE);  // for regular pages
/**********************/

  printf("page size is %u.\n", page_size);
  

  unsigned num_chains = num_threads * num_lists_per_thread;
  node** bases = (node**) malloc( sizeof(node*) * num_chains );
  node** ends  = (node**) malloc( sizeof(node*) * num_chains );

  uint64_t i, j, k;

  // for each chain
  for( i = 0; i < num_chains; ++i ) {


    uint64_t total_chain_size = sizeof(node) * size;
    uint64_t chunk_size;
    uint64_t current_chain_size = 0;
    for( j = 0, chunk_size = total_chain_size < page_size ? total_chain_size : page_size ; 
	 current_chain_size < total_chain_size ;
	 ++j, current_chain_size += chunk_size ) {
      node* nodes = (node*) valloc( chunk_size );
      uint64_t chunk_node_count = chunk_size / sizeof(node);
      node** locs = (node**) malloc( sizeof(node*) * chunk_node_count );

      if (DEBUG) printf("%lu %lu %lu %lu\n", size, total_chain_size, chunk_size, chunk_node_count);

      // initialize ids
      for( k = 0 ; k < chunk_node_count ; ++k ) {
	nodes[k].next = (node*) k;
      }

      if (DEBUG) print_nodes( nodes, chunk_node_count );

      // randomly permute nodes (fisher-yates (or knuth) shuffle)
      for(k = chunk_node_count-1; k > 0; --k) {
	uint64_t x = random() % k;
	if (DEBUG) printf("swapping %lu with %lu\n", k, x);
	if (DEBUG) print_node(&nodes[k]);
	if (DEBUG) print_node(&nodes[x]);
	node temp = nodes[k];
	nodes[k] = nodes[x];
	nodes[x] = temp;
      }

      if (DEBUG) print_nodes( nodes, chunk_node_count );

      // extract locs
      for( k = 0 ; k < chunk_node_count ; ++k ) {
	locs[ (uint64_t) nodes[k].next ] = &nodes[k];
      }

      // initialize pointers
      for( k = 0 ; k < chunk_node_count ; ++k ) {
	uint64_t id = k;
	uint64_t nextid = id + 1;
	//uint64_t wrapped_nextid = nextid % chunk_node_count;

	node* current = locs[id];
	if ( nextid == chunk_node_count ) {
	  current->next = NULL;
	} else {
	  current->next = locs[ nextid ];
	}

	//current->next = locs[ wrapped_nextid ];
	//current->next = nextid == chunk_node_count ? bases[i] : locs[ wrapped_nextid ]; 
      }    

      if (DEBUG) print_nodes( nodes, chunk_node_count );  
      
      if (j == 0) {
	bases[i] = locs[0];
	ends[i] = locs[chunk_node_count-1];
      } else {
	ends[i]->next = locs[0];
	ends[i] = locs[chunk_node_count-1];
      }

      free(locs);
    }

  }
  free(ends);
  return bases;
}


// initialize linked lists.
node** allocate_heap(uint64_t size, int num_threads, int num_lists) {
  int64_t i = 0;

  printf("Initializing footprint of size %lu * %lu = %lu bytes....\n", size, sizeof(node), size * sizeof(node));

  //size = size / num_threads;

  node** locs = (node**) malloc(sizeof(node*) * size); // node array
  node* nodes = (node*) malloc(sizeof(node) * size);  // temporary node pointers
  node** bases = (node**) malloc(sizeof(node*) * num_threads * num_lists); // initial node array

  // initialize ids
  #pragma omp parallel for num_threads(num_threads)
  for(i = 0; i < size; ++i) {
#ifdef HAVE_ID
    nodes[i].id = i;
#endif
    nodes[i].next = (node*)i;
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
#ifdef HAVE_ID
    printf("%u %u\n",  nodes[i].id, &nodes[i] - &nodes[0] );
    assert( nodes[i].id == &nodes[i] - &nodes[0] );
    locs[ nodes[i].id ] = &nodes[i];
#else
    locs[ (uint64_t) nodes[i].next ] = &nodes[i];
#endif
  }

  // initialize pointers
  // chop id-space into num_lists sections and build a circular list for each
  //int64_t base = 0;
  #pragma omp parallel for num_threads(num_threads)
  for(i = 0; i < size; ++i) {
#ifdef HAVE_ID
    uint64_t id = nodes[i].id;
#else
    //assert( nodes[i].id == &nodes[i] - &nodes[0] );
    uint64_t id = i; //&nodes[i] - &nodes[0];
    node* current = locs[i];
#endif
    int64_t thread_size = size / num_lists;
    int64_t base = id / thread_size;
    uint64_t nextid = id + 1;
    uint64_t wrapped_nextid = (nextid % thread_size) + (base * thread_size) % size;
    //printf("%d: %d %d %d\n", id, base, nextid, wrapped_nextid);
    current->next = locs[ wrapped_nextid ];
  }

  // grab initial pointers for each list
  for(i = 0; i < num_threads * num_lists; ++i) {
    bases[i] = locs[i * size / (num_threads * num_lists)];
  }

  //print(nodes, size);
  
  free(locs);
  return bases;
}



node* nodes (int index, uint64_t size, node* node0_nodes, node* node1_nodes) {
    if (index < size/2) {
        return node0_nodes;
    } else {
        return node1_nodes - size/2;
    }
}

// initialize linked lists
node** allocate_2numa_heap(uint64_t size, int num_threads, int num_lists, 
                            node** node0_low, node** node0_high,
                            node** node1_low, node** node1_high,
                            int numa_node0, int numa_node1) {
    int64_t i = 0;

    printf("Initializing footprint of size %lu * %lu = %lu bytes....\n", size, sizeof(node), size * sizeof(node));

    node** locs = (node**) malloc (sizeof(node*)*size); // node array

    //node* nodes = (node*) malloc(sizeof(node) * size);  // temporary node pointers
    if (numa_available() == -1) {
        printf("ERROR: numa support not available\n");
        exit(1);
    }

    numa_set_strict(1);
    node* node0_nodes = (node*) numa_alloc_onnode( sizeof(node) * size / 2, numa_node0);
    node* node1_nodes = (node*) numa_alloc_onnode( sizeof(node) * size / 2, numa_node1);

    if (node0_nodes==NULL || node1_nodes==NULL) {
        printf("node allocation failed (%lx, %lx)\n", (unsigned long)node0_nodes, (unsigned long)node1_nodes);
        exit(1);
    }

    *node0_low = node0_nodes;
    *node1_low = node1_nodes;
    *node0_high = node0_nodes + size / 2;
    *node1_high = node1_nodes + size / 2;

    node** bases = (node**) malloc(sizeof(node*) * num_threads * num_lists); // initial node array

    // initialize ids
    #pragma omp parallel for num_threads(num_threads)
    for(i = 0; i < size; ++i) {
      nodes(i,size,node0_nodes,node1_nodes)[i].next = (node*)i;
    }

    // randomly permute nodes (fisher-yates (or knuth) shuffle)
    for(i = size-1; i > 0; --i) {
      uint64_t j = random() % i;
      node temp = nodes(i,size,node0_nodes,node1_nodes)[i];
      nodes(i,size,node0_nodes,node1_nodes)[i] = nodes(j,size,node0_nodes,node1_nodes)[j];
      nodes(j,size,node0_nodes,node1_nodes)[j] = temp;
    }

    // extract locs
    #pragma omp parallel for num_threads(num_threads)
    for(i = 0; i < size; ++i) {
      locs[ (uint64_t) nodes(i,size,node0_nodes,node1_nodes)[i].next ] = &(nodes(i,size,node0_nodes,node1_nodes)[i]);
    }

    // initialize pointers
    // chop id-space into num_lists sections and build a circular list for each
    //int64_t base = 0;
    #pragma omp parallel for num_threads(num_threads)
    for(i = 0; i < size; ++i) {
      uint64_t id = i;
      node* current = locs[i];
      int64_t thread_size = size / num_lists;
      int64_t base = id / thread_size;
      uint64_t nextid = id + 1;
      uint64_t wrapped_nextid = (nextid % thread_size) + (base * thread_size) % size;
      //printf("%d: %d %d %d\n", id, base, nextid, wrapped_nextid);
      current->next = locs[ wrapped_nextid ];
    }

   // grab initial pointers for each list
   for(i = 0; i < num_threads * num_lists; ++i) {
       bases[i] = locs[i * size / (num_threads * num_lists)];
   }
   
   //print(nodes, size);
   
   free(locs);
   return bases;
} 


int alloc_test() {
  uint64_t size = 16; //1 << 10;
  uint64_t threads = 2;
  uint64_t chains = 2;
  node** bases = allocate_page(size,threads,chains);
  if (DEBUG) {
    printf("final answer:\n");
    int i, j;
    for(i = 0; i < threads; i++) {
      for(j = 0; j < chains; j++) {
	printf("%d,%d: ", i, j);
	print_walk(bases[i*chains+j],size);
      }
    }
  }
  return 0;
}
