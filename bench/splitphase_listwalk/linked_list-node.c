
#include "linked_list-node.h"

void print_node( node* np ) {
#ifdef HAVE_ID 
    printf("%p: { id: %lu next: %p }\n", np, np->id, np->next);
#else
    printf("%p: { next: %p }\n", np, np->next);
#endif
} 

void print_nodes( node nodes[], int size ) {
  printf("[\n");
  uint64_t i = 0;
  for( i = 0; i < size; ++i ) {
    print_node(&nodes[i]);
  }
  printf("]\n");
}

void print_walk( node* np, int size ) {
  printf("[\n");
  for ( ; size > 0; --size ) {
    print_node(np);
    np = np->next;
  }
  printf("]\n");
}

