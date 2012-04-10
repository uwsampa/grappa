#include "coro.h"
#include "stack.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

coro *coro_spawn(coro *me, coro_func f, size_t ssize) {
  coro *c = (coro*)malloc(sizeof(coro));
  assert(c != NULL);
  c->running = 0;
  c->base = malloc(ssize);
  assert(c->base != NULL);
  c->stack = (char*) c->base + ssize;
  memset(c->base, 0, ssize);
  makestack(&me->stack, &c->stack, f, c);
  return c;
}

coro *coro_init() {
  coro *me = (coro*)malloc(sizeof(coro));
  me->running = 1;
  // We don't need to free this (it's just the main stack segment)
  // so ignore it.
  me->base = NULL;
  // This'll get overridden when we swapstacks out of here.
  me->stack = NULL;
  return me;
}

void destroy_coro(coro *c) {
  free(c->base);
  free(c);
}
