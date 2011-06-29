#if HAVE_CONFIG_H
#   include "config.h"
#endif

/** @file
 * These routines use C's knowledge of the sizes of data types
 * to generate a portable mechanism for FORTRAN to translate
 * between bytes, integers and DoublePrecisions. 
 */

#include "sndrcv.h"

/**
 * Return the no. of bytes that n DoublePrecisions occupy
 */
Integer MDTOB_(Integer *n)
{
    if (*n < 0) {
        Error("MDTOB_: negative argument",*n);
    }

    return (Integer) (*n * sizeof(DoublePrecision));
}


/**
 * Return the minimum no. of integers which will hold n DoublePrecisions.
 */
Integer MDTOI_(Integer *n)
{
    if (*n < 0) {
        Error("MDTOI_: negative argument",*n);
    }

    return (Integer) ( (MDTOB_(n) + sizeof(Integer) - 1) / sizeof(Integer) );
}


/**
 * Return the no. of bytes that n ints=Integers occupy
 */
Integer MITOB_(Integer *n)
{
    if (*n < 0) {
        Error("MITOB_: negative argument",*n);
    }

    return (Integer) (*n * sizeof(Integer));
}


/**
 * Return the minimum no. of DoublePrecisions in which we can store n Integers
 */
Integer MITOD_(Integer *n)
{
    if (*n < 0) {
        Error("MITOD_: negative argument",*n);
    }

    return (Integer) ( (MITOB_(n) + sizeof(DoublePrecision) - 1) / sizeof(DoublePrecision) );
}
