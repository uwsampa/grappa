#include <stdlib.h>

// prototype
void xmtcompat_initialize_rand(void);

int64_t xmtcompat_rand_initialized = 0;

void xmtcompat_initialize_rand(void) {
  srand48(894893uL); /* constant picked by hitting the keyboard */
  xmtcompat_rand_initialized = 1;
}
