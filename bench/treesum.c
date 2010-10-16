#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "tree.h"

#include "greenery/thread.h"

void sumthread(thread *me, void *arg) {
  node *tree = (node *)arg;
  node *last;
  double sum = 0;
  while (tree != NULL) {
    sum += tree->value;
    if (tree->right != NULL) {
      thread_spawn(me, me->sched, sumthread, (void *)tree->right);
      thread_yield(me);
    }
    last = tree;
    tree = tree->left;
  }
  last->value = sum;
  thread_exit(me, (void *)last);
  exit(1); // thread_exit shouldn't return
}

double sum(node *tree) {
  thread *master = thread_init();
  scheduler *sched = create_scheduler(master);
  thread_spawn(master, sched, sumthread, tree);
  thread *thr;
  void *result;
  double sum = 0;
  while ((thr = thread_wait(sched, &result))) {
    node *last = (node *)result;
    sum += last->value;
    destroy_thread(thr);
  }
  return sum;
}

int main(int argc, char *argv[]) {
  assert(argc == 2);
  assert(sizeof(void *) == sizeof(double));
  FILE *treefile = fopen(argv[1], "r");
  assert (treefile != NULL);
  node *tree = read_tree(treefile);
  assert(tree != NULL);
  fclose(treefile);

  struct timeval tv;
  assert(0 == gettimeofday(&tv, NULL));
  int before = tv.tv_usec;
  double answer = sum(tree);

  assert(0 == gettimeofday(&tv, NULL));
  int after = tv.tv_usec;
  free(tree);
  printf("%d ms\n", after - before);
  printf("sum is %f\n", answer);
  return 0;
}
