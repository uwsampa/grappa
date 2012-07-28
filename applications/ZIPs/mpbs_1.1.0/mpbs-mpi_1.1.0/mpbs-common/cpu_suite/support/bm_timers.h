
#ifndef __BM_TIMER_H
#define __BM_TIMER_H
#include "local_types.h"
/* timing */

/* wall-clock time */

double wall(void);

/* cpu + system time */

double cpus(void);

typedef struct {
  double accum_wall, accum_cpus;
  double start_wall, start_cpus;
  time_t init_time;
  char running;
} timer;

void timer_clear (timer *t);

void timer_start (timer *t);

void timer_stop  (timer *t);

void timer_report (timer *t, double *pwall, double *pcpus,
		   int64 print);

#endif

/*
Emacs reads the stuff in this comment to set local variables
Local Variables:
compile-command: "make bm_timers.o"
End:
*/



