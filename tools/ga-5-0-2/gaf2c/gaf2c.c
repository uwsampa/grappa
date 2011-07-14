#if HAVE_CONFIG_H
#   include "config.h"
#endif

#if HAVE_STRING_H
#   include <string.h>
#endif

#include "farg.h"


/**
 * Converts C strings to Fortran strings.
 *
 * @param cstring C buffer
 * @param fstring Fortran string
 * @param flength length of fstring
 */
void ga_c2fstring(char *cstring, char *fstring, int flength)
{
    int clength = strlen(cstring);

    /* remove terminal \n character if any */
    if(cstring[clength] == '\n') {
        clength--;
    }

    /* Truncate C string into Fortran string */
    if (clength > flength) {
        clength = (int)flength;
    }

    /* Copy characters over */
    flength -= clength;
    while (clength--) {
        *fstring++ = *cstring++;
    }

    /* Now terminate with blanks */
    while (flength--) {
        *fstring++ = ' ';
    }
}


/**
 * Converts Fortran strings to C strings.
 *
 * Strip trailing blanks from fstring and copy it to cstring,
 * truncating if necessary to fit in cstring, and ensuring that
 * cstring is NUL-terminated.
 *
 * @param fstring Fortran string
 * @param flength length of fstring
 * @param cstring C buffer
 * @param clength max length (including NUL) of cstring
 */
void ga_f2cstring(char *fstring, int flength, char *cstring, int clength)
{
    /* remove trailing blanks from fstring */
    while (flength-- && fstring[flength] == ' ') ;

    /* the postdecrement above went one too far */
    flength++;

    /* truncate fstring to cstring size */
    if (flength >= clength) {
        flength = clength - 1;
    }

    /* ensure that cstring is NUL-terminated */
    cstring[flength] = '\0';

    /* copy fstring to cstring */
    while (flength--) {
        cstring[flength] = fstring[flength];
    }
}

#if NOFORT

/* To avoid missing symbols even though these should never be called. */

/**
 */
void F2C_GETARG(Integer *a, char *b, int c)
{
    GA_Error("GA was built without support for Fortran.  You have attempted "
            "to retreive command line arguments from a Fortran program. "
            "Please recompile GA and avoid using --disable-f77", 0L);
}


/**
 */
Integer F2C_IARGC()
{
    GA_Error("GA was built without support for Fortran.  You have attempted "
            "to retreive command line arguments from a Fortran program. "
            "Please recompile GA and avoid using --disable-f77", 0L);
    return 0;
}

#endif /* NOFORT */
