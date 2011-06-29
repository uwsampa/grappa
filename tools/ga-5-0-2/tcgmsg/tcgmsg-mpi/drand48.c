#if HAVE_CONFIG_H
#   include "config.h"
#endif

#if HAVE_STDIO_H
#   include <stdio.h>
#endif

extern long int random(void);
extern void srandom(unsigned int seed);

#include "typesf2c.h"
#include "sndrcv.h"


DoublePrecision FATR DRAND48_()
{
    DoublePrecision val=((DoublePrecision) random() ) * 4.6566128752458e-10;
    return val;
}


void FATR SRAND48_(Integer *seed)
{
    (void) srandom(*seed);
}
