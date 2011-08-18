#ifndef _BASE_H_
#define _BASE_H_

// base types and declarations
#include <stdio.h>

#include <stdint.h>
/*
typedef unsigned int uint32;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned long long uint64_t;
typedef signed int int32;
typedef signed char int8;
typedef signed short int16;
typedef signed long long int64;
typedef unsigned int bool;
*/
#define true 1
#define false 0

#ifdef DEBUG
#define panic(ss)  { \
    char *s = (char *)0; \
    printf("panic: %s in %s - %s(%d)\n", \
        ss, __FILE__, __FUNCTION__, __LINE__); \
    *s = 'D';   \
}
#else
#define panic(ss)  { \
    printf("panic: %s in %s - %s(%d)\n", \
        ss, __FILE__, __FUNCTION__, __LINE__); \
    exit(1);\
}
#endif

#include <assert.h>
/*
#ifdef DEBUG
#define assert(x) if (!(x)) { printf("panic: failed assert: %s at %s - %s(%d)\n", \
    #x, __FILE__, __FUNCTION__, __LINE__); \
    panic("Failed assert"); \
    }
#else
#define assert(x)
    
#endif
*/


#define ONE_THOUSAND 1000
#define ONE_MILLION (ONE_THOUSAND * ONE_THOUSAND)
#define ONE_BILLION (ONE_MILLION * ONE_THOUSAND)

#define ONE_KILO    1024
#define ONE_MEGA    (ONE_KILO * ONE_KILO)
#define ONE_GIGA    (ONE_KILO * ONE_MEGA)
#define ONE_TERA    (ONE_KILO * ONE_GIGA)

static inline uint64_t rdtsc() {
    uint32_t  h, l;
    uint64_t r;
    
    __asm__ volatile("rdtsc\n"
            : "=a" (l), "=d" (h));

    r = h;
    r = r << 32;
    r = r | l;
    return r;
    }

static inline uint64_t atomic_cmpset_uint64(
    volatile uint64_t *dest,
    uint64_t expect,
    uint64_t set_to) {
    __asm__ volatile("lock cmpxchg %1, %2\n"
        : "=a" (expect)
        : "q" (set_to), "m" (*dest), "0" (expect));
    return expect;
    }

static inline uint64_t atomic_fetch_and_add_uint64(
    volatile uint64_t *dest,
    uint64_t amount) {
    __asm__ volatile("lock xadd %0, %1\n" : "=r" (amount), "=m" (*dest) : "0" (amount));
    return amount;
    }
    
static inline void address_expose() {
    __asm__ volatile("" ::: "memory");
    }
    

#endif // _BASE_H_
