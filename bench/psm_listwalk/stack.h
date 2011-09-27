
#ifndef __STACK_H__
#define __STACK_H__

#include "debug.h"

#ifndef STACK_DATA_T
#define STACK_DATA_T void *
#endif

typedef struct stack {
  int index;
  int max;
  STACK_DATA_T * data;
} stack_t;

static inline void stack_push( stack_t * stack, STACK_DATA_T data ) {
  int next = stack->index + 1;
  ASSERT_NZ( next < stack->max );
  stack->data[next] = data;
  stack->index = next;
}

static inline STACK_DATA_T stack_get( stack_t * stack ) {
  ASSERT_NZ( 0 <= stack->index );
  return stack->data[ stack->index ];
}

static inline int stack_check( stack_t * stack ) {
  return 0 <= stack->index ;
}

static inline void stack_pop( stack_t * stack ) {
  int next = stack->index - 1;
  ASSERT_NZ( -1 <= next );
  stack->index = next;
}

void stack_init( stack_t * stack, int max );
void stack_teardown( stack_t * stack );

#endif
