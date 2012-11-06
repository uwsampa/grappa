// SUV3

#ifndef __REQUESTED_SV_LEVEL_H
#define __REQUESTED_SV_LEVEL_H
#define  __ASK_FOR       200112L
#define _POSIX_SOURCE
#define _POSIX_C_SOURCE __ASK_FOR
#define _XOPEN_SOURCE 600
#define _XOPEN_SOURCE_EXTENDED 1
#endif

#ifdef __GNUC__
#define _GNU_SOURCE
#endif

#include <unistd.h>


#include <sys/types.h>
#include <time.h>
#include <limits.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>

// gettimeofday is here
#include <sys/time.h>

// uint64 etc
#include "local_types.h"

void print_cmplr_flags(char* label);
void set_status(int status);
