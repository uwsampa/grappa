#if HAVE_CONFIG_H
#   include "config.h"
#endif

#include "typesf2c.h"
#include "tcgmsgP.h"

Integer NNODES_(void)
{
    return (Integer) TCGMSG_nnodes;
}
