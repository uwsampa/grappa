
#include "stack.h"
#include "debug.h"


void stack_init( stack_t * stack, int max ) {
  stack->max = max;
  stack->index = -1;
  stack->data = malloc( sizeof( STACK_DATA_T ) * max );
}

void stack_teardown( stack_t * stack ) {
  free( stack->data );
}
