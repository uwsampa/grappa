#include "bench.h"

#ifndef __BMM_H
#define __BMM_H
/* CVS info                      */
/* $Date: 2010/04/02 20:35:18 $  */
/* $Revision: 1.6 $              */
/* $RCSfile: bmm.h,v $           */
/* $State: Stab $:             */

#define VLEN   (1L<<17)
#define CLEN   512

void bmm_update (uint64 vec[], int32 cnt[], uint64 mat[]);
#endif

