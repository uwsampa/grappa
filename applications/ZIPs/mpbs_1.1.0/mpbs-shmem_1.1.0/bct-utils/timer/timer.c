#include "timer.h"

#include <stdio.h>
#include <stdlib.h>

#include <sys/resource.h>
#include <sys/time.h>

double wall() {
  struct timeval tp;

  if( gettimeofday (&tp, NULL) == -1 ) {
    perror("gettimeofday failed");
    exit(1);
  }

  return tp.tv_sec + tp.tv_usec/(double)1.0e6;
}

double cpus() {
  struct rusage ru;

  if( getrusage(RUSAGE_SELF,&ru) == -1){
    perror("getrusage failed");
    exit(1);
  }

  return
    (ru.ru_utime.tv_sec  + ru.ru_stime.tv_sec) +
    (ru.ru_utime.tv_usec + ru.ru_stime.tv_usec)/(double)1.0e6;
}

void timer_clear(timer *t) {
  t->accum_wall = t->accum_cpus = 0;
  t->start_wall = t->start_cpus = 0;
  t->running = 0;
}

void timer_start(timer *t) {
  t->start_wall = wall();
  t->start_cpus = cpus();
  t->running = 1;
}

void timer_stop(timer *t) {
  if(t->running == 0)
    return;
  t->accum_cpus += cpus() - t->start_cpus;
  t->accum_wall += wall() - t->start_wall;
  t->running = 0;
}

void timer_peek(timer *t, double *wtime, double *cpustime) {
  double w, c;

  w = t->accum_wall;
  c = t->accum_cpus;

  if (t->running) {
    c += cpus() - t->start_cpus;
    w += wall() - t->start_wall;
  }
 
  if (wtime)     *wtime    = w;
  if (cpustime)  *cpustime = c;
}

void log_times(FILE *fout, int id, char *description, timer *t, uint64_t nops) {
  if(id != 0)
    return;
  
  double rate;
  if(t->accum_wall != 0.0)
    rate = nops / t->accum_wall;
  else
    rate = 0.0;
  
  double c = t->accum_cpus;
  double w = t->accum_wall;
  
  fprintf(fout, "%-20s\t%lf\t%lf\t%lf\n", description, c, w, rate);
}

void print_times_and_rate(int id, timer *t, uint64_t nops, char *op) {
  if(id != 0)
    return;
  
  double rate;
  
  int prefix = 0;
  
  if(t->accum_wall != 0.0)
    rate = nops / t->accum_wall;
  else
    rate = 0.0;
  
  double c = t->accum_cpus;
  double w = t->accum_wall;
  
  const char *units[6] = {"", "Ki", "Mi", "Gi", "Ti", "Pi"};
  int i = 0;
  
  while (i < 6) {
    if (rate > 1024.0) {
      rate /= 1024.0;
      i++;
    }
    else
      break;
  }
  
  printf("\tCPU: %-11.4lf Wall: %-11.4lf Rate: %8.3lf %s %s/s\n", 
	 c, w, rate, units[i], op);
}

void print_times(int id, timer *t) {
  if(id != 0)
    return;
  
  double c = t->accum_cpus;
  double w = t->accum_wall;
  
  printf("\tCPU: %-11.4lf Wall: %-11.4lf\n", c, w);
}
