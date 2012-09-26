#include "coro.h"
#include "stack.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include <stdio.h>

// list of all coroutines
// (used only for debugging)
coro * all_coros = NULL;
size_t total_coros = 0;

// insert a coroutine into the list of all coroutines
// (used only for debugging)
void insert_coro( coro * c ) {
  // try to insert us in list going left
  if( all_coros ) {
    assert( all_coros->prev == NULL );
    all_coros->prev = c;
  }
    
  // insert us in list going right
  c->next = all_coros;
  all_coros = c;
}

// remove a coroutine from the list of all coroutines
// (used only for debugging)
void remove_coro( coro * c ) {
  // is there something to our left?
  if( c->prev ) {
    // remove us from next list
    c->prev->next = c->next;
  }
  // is there something to our right?
  if( c->next ) {
    // remove us from next list
    c->next->prev = c->prev;
  }
}

coro *coro_spawn(coro *me, coro_func f, size_t ssize) {
  coro *c = (coro*)malloc(sizeof(coro));
  assert(c != NULL);
  c->running = 0;
  c->suspended = 0;
  c->prev = NULL;
  c->next = NULL;

  // allocate stack and guard page
  c->base = valloc(ssize+4096*2);
  c->ssize = ssize;
  assert(c->base != NULL);

  // set stack pointer
  c->stack = (char*) c->base + ssize + 4096;

  // clear stack
  memset(c->base, 0, ssize);

  // arm guard page
  assert( 0 == mprotect( c->base, 4096, PROT_NONE ) );  
  assert( 0 == mprotect( c->base + ssize + 4096, 4096, PROT_NONE ) );  

  // set up coroutine to be able to run next time we're switched in
  makestack(&me->stack, &c->stack, f, c);

#ifdef CORO_PROTECT_UNUSED_STACK
  // disable writes to stack until we're swtiched in again.
  assert( 0 == mprotect( (void*)((intptr_t)c->base + 4096), ssize, PROT_READ ) );
#endif

  total_coros++;
  insert_coro( c ); // insert into debugging list of coros
  return c;
}

coro *coro_init() {
  coro *me = (coro*)malloc(sizeof(coro));
  me->running = 1;
  me->suspended = 0;
  me->prev = NULL;
  me->next = NULL;

  // We don't need to free this (it's just the main stack segment)
  // so ignore it.
  me->base = NULL;
  // This'll get overridden when we swapstacks out of here.
  me->stack = NULL;

  total_coros++;
  insert_coro( me ); // insert into debugging list of coros
  return me;
}

void destroy_coro(coro *c) {
  total_coros++;
  remove_coro(c); // remove from debugging list of coros
  if( c->base != NULL ) {
    // disarm guard page
    assert( 0 == mprotect( c->base, 4096, PROT_READ | PROT_WRITE ) );
    assert( 0 == mprotect( c->base + ssize + 4096, 4096, PROT_READ | PROT_WRITE ) );
#ifdef CORO_PROTECT_UNUSED_STACK
    // enable writes to stack so we can deallocate
    assert( 0 == mprotect( (void*)((intptr_t)c->base + 4096), c->ssize, PROT_READ | PROT_WRITE ) );
#endif
    free(c->base);
  }
  free(c);
}

