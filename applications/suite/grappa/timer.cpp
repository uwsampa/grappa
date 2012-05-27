// Originally from GraphCT, version 0.6.0
// License: GeorgiaTech

#define BILLION 1000000000
#define MILLION 1000000

#include "defs.hpp"

#if defined(__MTA__)
#include <sys/mta_task.h>
#elif defined(__MACH__)
#include <mach/mach_time.h>
#else
#include <time.h>
#endif

double timer() {
#if defined(__MTA__)
	return((double)mta_get_clock(0) / mta_clock_freq());
#elif defined(__MACH__)
	static mach_timebase_info_data_t info;
	mach_timebase_info(&info);
	uint64_t now = mach_absolute_time();
	now *= info.numer;
	now /= info.denom;
	return 1.0e-9 * (double)now;
#else
	struct timespec tp;
	clock_gettime(CLOCK_MONOTONIC, &tp);
	return (double)tp.tv_sec + (double)tp.tv_nsec / BILLION;
#endif
}

#define TIME(timevar, what) \
do { double _start; _start = timer(); what; timevar = timer() - _start; } while (0)
