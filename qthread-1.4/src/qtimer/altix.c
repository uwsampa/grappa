#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <qthread/qtimer.h>

#include <stdlib.h>		       /* for calloc() */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#if HAVE_SN_MMTIMER_H
# include <sn/mmtimer.h>
#elif HAVE_LINUX_MMTIMER_H
# include <linux/mmtimer.h>
#endif
#include <unistd.h>
#include <errno.h>
#ifndef MMTIMER_FULLNAME
# define MMTIMER_FULLNAME "/dev/mmtimer"
#endif

static double timer_freq_conv;
static volatile unsigned long *timer_address = NULL;
static unsigned long *mmdev_map = NULL;

struct qtimer_s {
    unsigned long start;
    unsigned long stop;
};

Q_NOINLINE static unsigned long vol_read_ul(volatile unsigned long *ptr)
{
    return *ptr;
}

int qtimer_init(void)
{
    int fd, ret;
    unsigned long val;
    long offset;

    fd = open(MMTIMER_FULLNAME, O_RDONLY);
    if (fd < 0)
	return -1;

    /* make sure we can map the timer */
    ret = ioctl(fd, MMTIMER_MMAPAVAIL, 0);
    if (1 != ret)
	return -1;

    /* find the frequency of the timer */
    ret = ioctl(fd, MMTIMER_GETFREQ, &val);
    if (ret == -ENOSYS)
	return -1;
    timer_freq_conv = 1.0 / val;

    /* find the address of the counter */
    ret = ioctl(fd, MMTIMER_GETOFFSET, 0);
    if (ret == -ENOSYS)
	return -1;
    offset = ret;

    mmdev_map = mmap(0, getpagesize(), PROT_READ, MAP_SHARED, fd, 0);
    if (NULL == mmdev_map)
	return -1;
    timer_address = mmdev_map + offset;
    close(fd);

    return 0;
}

void qtimer_start(qtimer_t q)
{
    q->start = vol_read_ul(timer_address);
}


void qtimer_stop(qtimer_t q)
{
    q->stop = vol_read_ul(timer_address);
}


double qtimer_secs(qtimer_t q)
{
    return ((double)(q->stop - q->start)) * timer_freq_conv;
}


qtimer_t qtimer_create(void)
{
    if (NULL == timer_address) {
	if (0 != qtimer_init())
	    return NULL;
    }

    return calloc(1, sizeof(struct qtimer_s));
}


void qtimer_destroy(qtimer_t q)
{
    free(q);
}
