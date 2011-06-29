#if HAVE_CONFIG_H
#   include "config.h"
#endif

/** @file
 *
 * $Header: /tmp/hpctools/ga/tcgmsg/ipcv5.0/mitob.c,v 1.2 1994-12-30 20:55:54 d3h325 Exp $
 *
 * These routines use C's knowledge of the sizes of data types
 * to generate a portable mechanism for FORTRAN to translate
 * between bytes, integers and doubles. Note that we assume that
 * FORTRAN integers are the same size as C longs.
 */
#include "typesf2c.h"
#include "sndrcv.h"


/**
 * Return the no. of bytes that n ints=longs occupy
 */
Integer MITOB_(Integer *n)
{
    if (*n < 0)
        Error("MITOB_: negative argument",*n);

    return (Integer) (*n * sizeof(Integer));
}
