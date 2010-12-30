
#ifndef __NODE_H__
#define __NODE_H__


#include <stdio.h>
#include <stdint.h>


//#define HAVE_ID

typedef struct node {
  struct node* next;
  uint64_t id;
  char padding[48];
} node  __attribute__ ((aligned (64)));

void print_node( node* np );
void print_nodes( node nodes[], int size );
void print_walk( node* np, int size );

#endif
