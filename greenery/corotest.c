#include "coro.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

void myfunc(coro *me, void *arg) {
  coro *master = (coro *)arg;
  arg = coro_invoke(me, master, 0);
  while (1) {
    arg = coro_invoke(me, master, ++arg);
    //    printf("child: %p\n", arg);
  }
}

int main(int argc, char *argv[]) {
  assert(argc == 2);

  int npasses = atoi(argv[1]);

  coro *me = coro_init();
  coro *other = coro_spawn(me, myfunc, 256);

  void *arg = coro_invoke(me, other, (void *)me);
  for (int i = 0; i < npasses; ++i) {
    arg = coro_invoke(me, other, ++arg);
    //printf("master: %p\n", arg);
  }

  return 0;
}
