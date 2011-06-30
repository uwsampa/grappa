
#ifndef __NODE_HPP__
#define __NODE_HPP__


#include <cstdint>


//#define HAVE_ID

typedef struct node {
  uint64_t id;
  struct node* next;
  char padding[48];
} node;


#endif
