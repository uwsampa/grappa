#if HAVE_CONFIG_H
#   include "config.h"
#endif

#if HAVE_STDIO_H
#    include <stdio.h>
#endif
#if HAVE_SHMALLOC
#    define SHMALLOC shmalloc
#    define SHFREE   shfree
#endif
#if HAVE_SHARE_MALLOC
#    define SHMALLOC share_malloc
#    define SHFREE   share_free
#endif

extern char *SHMALLOC();
extern int SHFREE();

#define MAX_ADDR 20
static int next_id = 0;              /* Keep track of id */
static char *shaddr[MAX_ADDR];       /* Keep track of addresses */


char *CreateSharedRegion(Integer *id, Integer *size)
{
    char *temp;

    if (next_id >= MAX_ADDR) {
        Error("CreateSharedRegion: too many shared regions", (Integer) next_id);
    }

    if ( (temp = SHMALLOC((unsigned) *size)) == (char *) NULL) {
        Error("CreateSharedRegion: failed in SHMALLOC", (Integer) *size);
    }

    *id = next_id++;
    shaddr[*id] = temp;

    return temp;
}


Integer DetachSharedRegion(Integer id, Integer size, char *addr)
{
    /* This needs improving to make more robust */
    return SHFREE(addr);
}


Integer DeleteSharedRegion(Integer id)
{
    /* This needs improving to make more robust */
    return SHFREE(shaddr[id]);
}


char *AttachSharedRegion(Integer id, Integer size)
{
    Error("AttachSharedRegion: cannot do this on SEQUENT or BALANCE",
            (Integer) -1);
}
