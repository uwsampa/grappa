/*** UNCLASSIFIED//FOR OFFICIAL USE ONLY ***/

// Copyright © 2008, Institute for Defense Analyses, 4850 Mark Center
// Drive, Alexandria, VA 22311-1882; 703-845-2500.
//
// This material may be reproduced by or for the U.S. Government 
// pursuant to the copyright license under the clauses at DFARS 
// 252.227-7013 and 252.227-7014.

/* $Id: utypes.h,v 1.1 2008/03/17 18:41:52 fmmaley Exp fmmaley $ */

#ifndef UTYPES_H
#define UTYPES_H
#include <micro64.h>

typedef struct { uint32 key; } shw;
typedef struct { uint64 key; } s1w;
typedef struct { uint64 key; uint64 data; } s2w;
typedef struct { uint64 key; uint64 data[2]; } s3w;

#define MAXR 9
#define MAXRX 7
#endif
