
#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdlib.h>
#include <stdio.h>


#ifndef DEBUG
#define DEBUG 0
#endif

#define LOG_INFO printf
#define LOG_WARN printf
#define LOG_ERROR printf

#define ASSERT_Z(x) \
  do { \
    if ( (x)) { \
      LOG_ERROR("error: " #x " failed (returned non-zero).\n"); \
      exit(EXIT_FAILURE); \
    } \
  } while (0)

#define ASSERT_NZ(x) \
  do { \
    if (!(x)) { \
      LOG_ERROR("error: " #x " failed (returned zero/null).\n"); \
      exit(EXIT_FAILURE); \
    } \
  } while (0)

#endif
