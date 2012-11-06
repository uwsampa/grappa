/*** UNCLASSIFIED//FOR OFFICIAL USE ONLY ***/

// Copyright © 2008, Institute for Defense Analyses, 4850 Mark Center
// Drive, Alexandria, VA 22311-1882; 703-845-2500.
//
// This material may be reproduced by or for the U.S. Government 
// pursuant to the copyright license under the clauses at DFARS 
// 252.227-7013 and 252.227-7014.

/* $Id: ipsort.h,v 0.3 2008/03/17 18:39:08 fmmaley Exp fmmaley $ */

#include <micro64.h>
#include "utypes.h"

#defcases IP_SORTTYPE = shw, s1w, s2w, s3w
void  isort_##IP_SORTTYPE(IP_SORTTYPE *arr, int64 n, uint64 mask);
int64 qpart_##IP_SORTTYPE(IP_SORTTYPE *arr, int64 n, uint64 bit);
void  rxpart_##IP_SORTTYPE(IP_SORTTYPE *arr, uint64 *c, int64 lb, uint64 m);
#endcases

#defcases IP_SORTTYPE = shw, s2w, s3w
void  nisort_##IP_SORTTYPE(IP_SORTTYPE *s, IP_SORTTYPE *t, int64 n, uint64 m);
#endcases

// For some reason, the one-word case of insertion sort is faster this way:
void nisort_uint64(uint64 *s, uint64 *t, int64 n, uint64 m);
