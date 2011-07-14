#ifndef ARMCI_TESTING_TIMER_H_
#define ARMCI_TESTING_TIMER_H_

#if defined(__i386__) || defined(__x86_64__) || defined(__powerpc__)
#   if defined(__i386__)
static __inline__ unsigned long long rdtsc(void)
{
    unsigned long long int x;
    __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
    return x;
}
#   elif defined(__x86_64__)
static __inline__ unsigned long long rdtsc(void)
{
    unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}
#   elif defined(__powerpc__)
static __inline__ unsigned long long rdtsc(void)
{
    unsigned long long int result=0;
    unsigned long int upper, lower,tmp;
    __asm__ volatile(
            "0:                  \n"
            "\tmftbu   %0        \n"
            "\tmftb    %1        \n"
            "\tmftbu   %2        \n"
            "\tcmpw    %2,%0     \n"
            "\tbne     0b        \n"
            : "=r"(upper),"=r"(lower),"=r"(tmp)
            );
    result = upper;
    result = result<<32;
    result = result|lower;

    return(result);
}
#   endif
#elif HAVE_WINDOWS_H
#   include <windows.h>
#elif HAVE_SYS_TIME_H
#   include <sys/time.h>
#endif

static unsigned long long timer_start()
{
#if defined(__i386__) || defined(__x86_64__) || defined(__powerpc__)
    return rdtsc();
#elif HAVE_WINDOWS_H
    LARGE_INTEGER timer;
    QueryPerformanceCounter(&timer);
    return timer.QuadPart * 1000 / frequency.QuadPart;
#elif HAVE_SYS_TIME_H
    struct timeval timer;
    (void)gettimeofday(&timer, NULL);
    return timer.tv_sec*1000000 + timer.tv_usec;
#else
    return 0;
#endif
}

static unsigned long long timer_end(unsigned long long begin)
{
    return timer_start()-begin;
}

static void timer_init()
{
#if HAVE_WINDOWS_H
    QueryPerformanceFrequency(&frequency);
#endif
}

static const char* timer_name()
{
#if defined(__i386__) || defined(__x86_64__) || defined(__powerpc__)
    return "rdtsc";
#elif HAVE_WINDOWS_H
    return "windows QueryPerformanceCounter";
#elif HAVE_SYS_TIME_H
    return "gettimeofday";
#else
    return "no timers";
#endif
}

#endif /* ARMCI_TESTING_TIMER_H_ */
