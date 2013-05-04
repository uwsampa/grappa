#ifndef _XMT_OPS_H
#define _XMT_OPS_H

#include <limits.h> /* for INT_MAX */
#include <stdlib.h> /* for drand48 */
#include <stdint.h>

#define MTA(x)

#define readfe(ptr) (*(ptr))
#define readff(ptr) (*(ptr))
#define purge(ptr) (*(ptr)=0)
#define writeef(ptr, val) (*(ptr) = (val))

static inline int64_t int_fetch_add(int64_t *p, int64_t v) {
  int64_t tmp = *p;
  *p += v;
  return tmp;
}

static inline int64_t MTA_CLOCK(int x) {
  static int64_t seed = 0;
  return (seed+=1)*1229227;
}

static inline int64_t mta_get_num_teams() {
  return 1;
}

static inline int64_t int64_fetch_add(int64_t *p, int64_t v) {
  int64_t tmp = *p;
  *p += v;
  return tmp;
}

void prand(int64_t n, double * v);
uint64_t MTA_BIT_LEFT_ZEROS(uint64_t y);

#endif /* _XMT_OPS_H */
