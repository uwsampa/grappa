
#ifndef __NODE_H__
#define __NODE_H__


#include <stdio.h>
#include <stdint.h>


//#define HAVE_ID

typedef struct node {
  struct node* next;
} node;

void print_node( node* np );
void print_nodes( node nodes[], int size );
void print_walk( node* np, int size );

#endif
