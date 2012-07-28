/*** UNCLASSIFIED//FOR OFFICIAL USE ONLY ***/

// Copyright © 2008, Institute for Defense Analyses, 4850 Mark Center
// Drive, Alexandria, VA 22311-1882; 703-845-2500.
//
// This material may be reproduced by or for the U.S. Government 
// pursuant to the copyright license under the clauses at DFARS 
// 252.227-7013 and 252.227-7014.

/* $Id: uhash.h,v 1.1 2008/03/17 18:43:21 fmmaley Exp fmmaley $ */

#include <micro64.h>
#include "utypes.h"

#defcases (sort_t,key_t) = (shw,uint32), (s1w,uint64), (s2w,uint64), (s3w,uint64)
int64
remunm_##sort_t(sort_t* restrict a, int64 n, int64 lb, uint64 m,
		uint64* restrict ht, key_t* restrict b, sort_t* restrict c);
#endcases
