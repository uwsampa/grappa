
#ifndef __NODE_HPP__
#define __NODE_HPP__


//#include <cstdint>
#include <stdint.h>


//#define HAVE_ID

typedef struct node {
  uint64_t id;
  uint64_t next;
  char padding[48];
} node;


#endif
