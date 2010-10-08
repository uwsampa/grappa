#include "thread.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

void mythread(thread *me, void *arg) {
  long nyields= (long)arg;
  for (int i = 0; i < nyields; ++i) {
    thread_yield(me);
  }
}

int main(int argc, char *argv[]) {
  assert(argc == 3);

  int nthreads = atoi(argv[1]);
  long nyields = atol(argv[2]);

  thread *master = thread_init();
  scheduler *sched = create_scheduler(master);

  for (int i = 0; i < nthreads; ++i) {
    thread_spawn(master, sched, mythread, (void *)nyields);
  }

  run_all(sched);

  destroy_scheduler(sched);
  destroy_thread(master);

  return 0;
}
