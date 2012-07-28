#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>

int lprintf_single(int rank, FILE *fp, char *fmt, ... ) {
  int retval = 0;
  va_list args;

  if(rank == 0) {
    va_start(args, fmt);
    retval = vprintf(fmt, args);
    va_end(args);
    if(fp != NULL) {
      va_start(args, fmt);
      retval += vfprintf(fp, fmt, args);
      va_end(args);
    }
  }

  return retval;
}

int fprintf_single( int rank, FILE *fp, char *fmt, ... ) {
  va_list args;
  int retval = 0;

  if(rank != 0)
    return 0;

  va_start(args, fmt);
  retval = vfprintf(fp, fmt, args);
  va_end(args);

  return retval;
}

int printf_single( int rank, char *fmt, ... ) {
  va_list args;
  int retval = 0;

  if(rank != 0)
    return 0;

  va_start(args, fmt);
  retval = vprintf(fmt, args);
  va_end(args);

  return retval;
}

