
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
  int enqueue_count;
  int dequeue_count;
} stack_t;

static inline void stack_push( stack_t * stack, STACK_DATA_T data ) {
  int next = stack->index + 1;
  ASSERT_NZ( next < stack->max );
  ++stack->enqueue_count;
  stack->data[ next ] = data;
  stack->index = next;
}

static inline STACK_DATA_T stack_get( stack_t * stack ) {
  ASSERT_NZ( 0 <= stack->index && stack->index < stack->max );
  return stack->data[ stack->index ];
}

static inline int stack_empty( stack_t * stack ) {
  return stack->index < 0;
}

static inline int stack_full( stack_t * stack ) {
  return stack->max - 1 <= stack->index;
}

static inline void stack_pop( stack_t * stack ) {
  int next = stack->index - 1;
  ASSERT_NZ( -1 <= next );
  ++stack->dequeue_count;
  stack->index = next;
}

void stack_init( stack_t * stack, int max );
void stack_teardown( stack_t * stack );

#endif
