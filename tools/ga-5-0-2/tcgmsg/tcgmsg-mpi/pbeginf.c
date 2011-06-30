#if HAVE_CONFIG_H
#   include "config.h"
#endif

#if HAVE_STDIO_H
#   include <stdio.h>
#endif
#if HAVE_STRING_H
#   include <string.h>
#endif

#include "farg.h"
#include "typesf2c.h"
#include "sndrcv.h"
#include "srftoc.h"
#define LEN 255

/**
 * Hewlett Packard Risc box, SparcWorks F77 2.* and Paragon compilers.
 * Have to construct the argument list by calling FORTRAN.
 */
void PBEGINF_()
{
    Integer argc = F2C_IARGC();
    Integer i, len;
    char *argv[LEN], arg[LEN];

    for (i=0; i<argc; i++) {
        F2C_GETARG(&i, arg, LEN);
        for(len = LEN-2; len && (arg[len] == ' '); len--);
        len++;
        arg[len] = '\0'; /* insert string terminator */
        /*printf("%10s, len=%d\n", arg, len);  fflush(stdout);*/ 
        argv[i] = strdup(arg);
    }

    tcgi_pbegin(argc, argv);
}


/**
 * Alternative entry for those senstive to FORTRAN making reference
 * to 7 character external names
 */
void PBGINF_()
{
    PBEGINF_();
}
