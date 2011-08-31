
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
    int val = 0;				\
    if ((val = (x))) {							\
      LOG_ERROR(__FILE__ ":%d: error: " #x " failed (returned %d).\n", __LINE__, val); \
      exit(EXIT_FAILURE); \
    } \
  } while (0)

#define ASSERT_NZ(x) \
  do { \
    void * val = NULL;				\
    if (!(val = (x))) {							\
      LOG_ERROR(__FILE__ ":%d: error: " #x " failed (returned %p).\n", __LINE__, val); \
      exit(EXIT_FAILURE); \
    } \
  } while (0)

#endif
