#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mta_task.h>
#include <machine/runtime.h>

#include "globals.h"
double timer()
{ return((double)mta_get_clock(0) / mta_clock_freq()); }
