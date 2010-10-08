#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <qthread/qtimer.h>

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

struct qtimer_s
{
    struct timeval start, stop;
};

void qtimer_start(qtimer_t q)
{
    gettimeofday(&(q->start), NULL);
}

void qtimer_stop(qtimer_t q)
{
    gettimeofday(&(q->stop), NULL);
}

double qtimer_secs(qtimer_t q)
{
    return (q->stop.tv_sec + q->stop.tv_usec*1e-6) - (q->start.tv_sec + q->start.tv_usec*1e-6);
}

qtimer_t qtimer_create()
{
    return calloc(1, sizeof(struct qtimer_s));
}

void qtimer_destroy(qtimer_t q)
{
    free(q);
}
