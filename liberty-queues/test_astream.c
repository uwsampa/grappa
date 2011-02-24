#define CREATE_PROCESS

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <inttypes.h>
#include <string.h>
#include <sched.h>
#include <time.h>

#ifdef CREATE_PROCESS
#include <sched.h>
#include <sys/wait.h>
#endif

#include "sw_queue_astream.h"

#define TEST_SIZE 100000000 // 10000000

#ifdef NO_PROD
#define NO_CON
#endif

SW_Queue *q;

// *********************************************** /
// ******** Consumer Sync ************************ /
// *********************************************** /
int consumer_sync(void *arg)
{
  long iter = 0;
  unsigned long cost = 0;
  unsigned long j = 0;

  (void) arg;

  sw_queue_t handle = *q[0];
  for ( iter = 0; iter < TEST_SIZE; ++iter) {
    j = (unsigned long) sq_consume(&handle);
    cost += j;
  }

  printf("j: %ld cost: %ld \n", j, cost);

  sq_produce(q[1], cost);
  sq_flushQueue(q[1]);

  return 0;
}


/** ***********************************************/
/** ******** Producer Sync ************************/
/** ***********************************************/
void producer_sync(void)
{
  long iter = 0;
  unsigned long mycost = 0;
  unsigned long j = 1;
  unsigned long  cost;

  sw_queue_t handle = *q[0];
  for (iter = 0; iter < TEST_SIZE; ++iter) {

#ifndef NO_PROD
    sq_produce(&handle, j);
#endif /* NO_PROD */

    mycost += j;
    j++;

    if(j>124) {
      j = 0;
    }
  }

#ifndef NO_PROD
  sq_flushQueue(&handle);
#endif /* NO_PROD */

#ifndef NO_CON
  cost = sq_consume(q[1]);
#endif /* NO_CON */

  printf("j = %ld\n",j);
  printf("Value = %ld mycost= %ld \n",cost, mycost);
}

typedef struct
{
  pid_t pid;
  void * stack;
} thread_t;

/** *************************** **/
/** ******** Main ************* **/
/** *************************** **/
int main(int argc, char **argv)
{
  (void) argc;
  (void) argv;

  /// Make threads
  q = (SW_Queue *) malloc(sizeof(SW_Queue)*2);

  q[0] = sq_createQueue();
  q[1] = sq_createQueue();

  long time1, time2;
  __time_t sec1, sec2;
  struct timespec ts1;
  struct timespec ts2;
  long  totalTime = 0;

#ifdef CREATE_PROCESS
  printf("PROCESS \n");
  thread_t * threads;
#else /* CREATE_PROCESS */
  printf("thread \n");
  void *exit_status;
#endif /* CREATE_PROCESS */

#ifndef NO_CON
#ifdef CREATE_PROCESS
  threads = (thread_t *) malloc (sizeof (thread_t));
  threads->stack = malloc (sizeof(char) * 0x1000000);
  threads->pid = clone (consumer_sync, (char *) threads->stack + 0x1000000, SIGCHLD | CLONE_FS | CLONE_FILES, NULL);
#else /* CREATE_PROCESS */
  pthread_t consumer_thread_sync;
  pthread_create(&consumer_thread_sync, NULL, consumer_sync, NULL);
#endif /* CREATE_PROCESS */
#endif /* NO_CON */

  clock_gettime(CLOCK_REALTIME, &ts1);
  producer_sync();
  clock_gettime(CLOCK_REALTIME, &ts2);

#ifndef NO_CON
#ifdef CREATE_PROCESS
  pid_t pid_sync = wait(NULL);
  if (pid_sync != threads->pid) perror("wait(NULL)");
#else /* CREATE_PROCESS */
  pthread_join(consumer_thread_sync, &exit_status);
#endif /* CREATE_PROCESS */
#endif /* NO_CON */

  sec1 = ts1.tv_sec;
  time1 = ts1.tv_nsec;
  sec2 = ts2.tv_sec;
  time2 = ts2.tv_nsec;
  totalTime = (sec2-sec1)*1000000 + ((time2-time1)/1000);
  printf("%ld us \n", totalTime);
  printf("Done!\n");

#ifndef NO_CON
#ifdef CREATE_PROCESS
  free(threads->stack);
#endif /* CREATE_PROCESS */
#endif /* NO_CON */

  free(threads);

  return 0;
}

