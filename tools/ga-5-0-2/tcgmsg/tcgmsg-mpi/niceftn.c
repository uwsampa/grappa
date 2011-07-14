#if HAVE_CONFIG_H
#   include "config.h"
#endif

#if HAVE_UNISTD_H
#   include <unistd.h>
#endif

#include "typesf2c.h"
#include "srftoc.h"

/**
 * Wrapper around nice for FORTRAN users courtesy of Rick Kendall
 * ... C has the system interface.
 */
Integer NICEFTN_(Integer *ival)
{
    int val = (int)(*ival);
    return nice(val);
}
