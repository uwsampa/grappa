/* CVS info                     */
/* $Date: 2010/03/18 17:22:47 $ */
/* $Revision: 1.4 $:            */
/* $RCSfile: bm_timers.c,v $:   */
/* $State: Stab $:               */


#include "common_inc.h"
#include "error_routines.h"
#include "bm_timers.h"

/* timing */

//#include <sys/time.h>

/* wall-clock time */

double wall(void)

{
  struct timeval tp;

  if( gettimeofday (&tp, NULL) == -1){
    sys_err_exit(errno, "subroutine wall");
  }
  return
    tp.tv_sec + tp.tv_usec/(double)1.0e6;
}

#include <sys/resource.h>

/* cpu + system time */

double cpus(void)

{
  struct rusage ru;

  errno = 0;
  if ( getrusage(RUSAGE_SELF,&ru) == -1){
    sys_err_exit(errno, "subroutine cpus");
  }
  return
    (ru.ru_utime.tv_sec  + ru.ru_stime.tv_sec) +
    (ru.ru_utime.tv_usec + ru.ru_stime.tv_usec)/(double)1.0e6;
}

void timer_clear (timer *t)

{
  t->accum_wall = t->accum_cpus = 0;
  t->start_wall = t->start_cpus = 0;
  t->running = 0;
}

void timer_start (timer *t)

{
  t->start_wall = wall();
  t->start_cpus = cpus();
  t->running = 1;
}

void timer_stop  (timer *t)

{
  if (t->running == 0)
    return;
  t->accum_cpus += cpus() - t->start_cpus;
  t->accum_wall += wall() - t->start_wall;
  t->running = 0;
}


void timer_report (timer *t, double *pwall, double *pcpus,
			  int64 print)

{
  double w, c;

  w = t->accum_wall;
  c = t->accum_cpus;

  if (t->running) {
    c += cpus() - t->start_cpus;
    w += wall() - t->start_wall;
  }
  if (print) {
    printf ("%7.3f secs of wall clock time\n",     w);
    printf ("%7.3f secs of cpu and system time\n", c);
  }

  if (pwall)  *pwall = w;
  if (pcpus)  *pcpus = c;
}


/*
Emacs reads the stuff in this comment to set local variables
Local Variables:
compile-command: "make bm_timers.o"
End:
*/



