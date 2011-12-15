#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "defs.h"
#include "globals.h"

#ifdef __MTA__
#include <sys/mta_task.h>
#include <machine/runtime.h>
double timer(void)
{ return((double)mta_get_clock(0) / mta_clock_freq()); }

static int64_t load_ctr = -1, store_ctr, ifa_ctr;

static void
init_stats (void)
{
  load_ctr = mta_rt_reserve_task_event_counter (3, RT_LOAD);
  assert (load_ctr >= 0);
  store_ctr = mta_rt_reserve_task_event_counter (2, RT_STORE);
  assert (store_ctr >= 0);
  ifa_ctr = mta_rt_reserve_task_event_counter (1, RT_INT_FETCH_ADD);
  assert (ifa_ctr >= 0);
  memset (&stats_tic_data, 0, sizeof (stats_tic_data));
  memset (&stats_toc_data, 0, sizeof (stats_toc_data));
}

static void
get_stats (struct stats *s)
{
MTA("mta fence")
  s->clock = mta_get_task_counter(RT_CLOCK);
MTA("mta fence")
  s->issues = mta_get_task_counter(RT_ISSUES);
MTA("mta fence")
  s->concurrency = mta_get_task_counter(RT_CONCURRENCY);
MTA("mta fence")
  s->load = mta_get_task_counter(RT_LOAD);
MTA("mta fence")
  s->store = mta_get_task_counter(RT_STORE);
MTA("mta fence")
  s->ifa = mta_get_task_counter(RT_INT_FETCH_ADD);
MTA("mta fence")
}

void
stats_tic (char *statname)
{
  if (load_ctr < 0)
    init_stats ();
  memset (&stats_tic_data.statname, 0, sizeof (stats_tic_data.statname));
  strncpy (&stats_tic_data.statname, statname, 256);
  get_stats (&stats_tic_data);
}

void
stats_toc (void)
{
  get_stats (&stats_toc_data);
}

void
print_stats (void)
{
  if (load_ctr < 0) return;
  printf ("alg : %s\n", stats_tic_data.statname);
#define PRINT(v) do { printf (#v " : %ld\n", (stats_toc_data.v - stats_tic_data.v)); } while (0)
  PRINT (clock);
  PRINT (issues);
  PRINT (concurrency);
  PRINT (load);
  PRINT (store);
  PRINT (ifa);
#undef PRINT
  printf ("endalg : 1\n");
}


#elif defined(_OPENMP)
#include <omp.h>

double timer(void)
{
	return omp_get_wtime();
}

void
stats_tic (char *c /*UNUSED*/)
{
}

void
stats_toc (void)
{
}

void
print_stats (void)
{
}


#else
#include <time.h>
double timer(void)
{
    struct timespec tp;
#if defined(CLOCK_PROCESS_CPUTIME_ID)
#define CLKID CLOCK_PROCESS_CPUTIME_ID
#elif  defined(CLOCK_REALTIME_ID)
#define CLKID CLOCK_REALTIME_ID
#endif
    clock_gettime(CLKID, &tp);
    return (double)tp.tv_sec + 1.0e-9 * (double)tp.tv_nsec;
}

void
stats_tic (char *c /*UNUSED*/)
{
}

void
stats_toc (void)
{
}

void
print_stats (void)
{
}
#endif
