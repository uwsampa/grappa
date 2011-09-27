
#ifndef __NODE_H__
#define __NODE_H__


#include <stdint.h>


//#define HAVE_ID

typedef struct node {
  uint64_t id;
  struct node * next;
  char padding[48];
} node;

#endif
