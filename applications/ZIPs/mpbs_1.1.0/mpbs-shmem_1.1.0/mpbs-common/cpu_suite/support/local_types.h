/******************************************************************************\
*  Local version of EXACT-SIZE-INTEGER TYPES before C99 and stdint.h           *
*  Equivalent to C99 extented types:                                           *
*  int8_t                                                                      *
*  uint8_t                                                                     *
*  int16_t                                                                     *
*  uint16_t                                                                    *
*  int32_t                                                                     *
*  uint32_t                                                                    *
*  int64_t                                                                     *
*  uint64_t                                                                    *
\******************************************************************************/

#ifndef __LOCAL_TYPES_H
#define __LOCAL_TYPES_H

/* LP64 types */

typedef   signed  char  int8 ;
typedef unsigned  char uint8 ;
typedef          short  int16;
typedef unsigned short uint16;
typedef            int  int32;
typedef unsigned   int uint32;
typedef           long  int64;
typedef unsigned  long uint64;
#endif



/*
Emacs reads the stuff in this comment to set local variables
Local Variables:
compile-command: "make .o name"
End:
*/



