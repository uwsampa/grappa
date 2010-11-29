/*******************************************************************************
 * Copyright (c) 2006 International Business Machines Corporation.             *
 * All rights reserved. This program and the accompanying materials            *
 * are made available under the terms of the Common Public License v1.0        *
 * which accompanies this distribution, and is available at                    *
 * http://www.opensource.org/licenses/cpl1.0.php                               *
 *                                                                             *
 * Contributors:                                                               *
 *    Douglas M. Pase - initial API and implementation                         *
 *******************************************************************************/


#include <stdio.h>
#include <sys/time.h>

#include "Timer.h"

#include "Types.h"

static int64  read_rtc();
static void   calibrate_rtc(int n);
static double wall_seconds();

static int    wall_ticks   = -1;
static int    rtc_ticks    = -1;
static double wall_elapsed = -1;
static int64  rtc_elapsed  = -1;
static double time_factor  = -1;

#if !defined(RTC) && !defined(GTOD)
#define RTC
#endif

#if defined(RTC)

double
Timer::seconds()
{ 
    return (double) read_rtc() * time_factor;
}

int64
Timer::ticks()
{ 
				// See pg. 406 of the AMD x86-64 Architecture
				// Programmer's Manual, Volume 2, System Programming
    unsigned int eax=0, edx=0;

    __asm__ __volatile__(
	"rdtsc ;"
	"movl %%eax,%0;"
	"movl %%edx,%1;"
	""
    : "=r"(eax), "=r"(edx)
    :
    : "%eax", "%edx"
    );

    return ((int64) edx << 32) | (int64) eax;
}

static int64
read_rtc()
{
				// See pg. 406 of the AMD x86-64 Architecture
				// Programmer's Manual, Volume 2, System Programming
    unsigned int eax=0, edx=0;

    __asm__ __volatile__(
	"rdtsc ;"
	"movl %%eax,%0;"
	"movl %%edx,%1;"
	""
    : "=r"(eax), "=r"(edx)
    :
    : "%eax", "%edx"
    );

    return ((int64) edx << 32) | (int64) eax;
}

void
Timer::calibrate()
{
    Timer::calibrate(1000);
}

void
Timer::calibrate(int n)
{
    wall_ticks = n;

    double wall_start,wall_finish,t;
    t = wall_seconds();
    while (t == (wall_start=wall_seconds())) {
	;
    }
    int64 rtc_start = read_rtc();
    for (int i=0; i < wall_ticks; i++) {
	t = wall_seconds();
	while (t == (wall_finish=wall_seconds())) {
	    ;
	}
    }
    int64 rtc_finish = read_rtc();

    wall_elapsed = wall_finish - wall_start;
    rtc_elapsed  = rtc_finish  - rtc_start;
    time_factor  = wall_elapsed / (double) rtc_elapsed;
}

static double
wall_seconds()
{ 
    struct timeval t;
    gettimeofday(&t, NULL);

    return (double) t.tv_sec + (double) t.tv_usec * 1E-6;
}

#else

double
Timer::seconds()
{ 
    struct timeval t;
    gettimeofday(&t, NULL);

    return (double) t.tv_sec + (double) t.tv_usec * 1E-6;
}

int64
Timer::ticks()
{ 
    struct timeval t;
    gettimeofday(&t, NULL);

    return 1000000 * (int64) t.tv_sec + (int64) t.tv_usec;
}

void
Timer::calibrate()
{
}

void
Timer::calibrate(int n)
{
}

#endif

static double
min( double v1, double v2 )
{
    if (v2 < v1) return v2;
    return v1;
}

double
Timer::resolution()
{ 
    double a,b,c=1E9;
    for (int i=0; i < 10; i++) {
	a = Timer::seconds();
	while (a == (b=Timer::seconds()))
	    ;
	a = Timer::seconds();
	while (a == (b=Timer::seconds()))
	    ;
	c = min(b - a, c);
    }

    return c;
}
