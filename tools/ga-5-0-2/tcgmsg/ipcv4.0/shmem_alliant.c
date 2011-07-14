#if HAVE_CONFIG_H
#   include "config.h"
#endif

#if HAVE_STDIO_H
#   include <stdio.h>
#endif
#if HAVE_SYS_TIME_H
#   include <sys/time.h>
#endif
#if HAVE_MALLOC_H
#   include <malloc.h>
#endif

char *CreateSharedRegion(Integer *id, Integer *size)
{
    struct timeval tp;
    struct timezone tzp;
    char *temp;
    int status;

    /* Have to round up to a multiple of page size before allocating
       on a page boundary */

    *size = ( (*size + 4095) / 4096 ) * 4096;

    if ( (temp = valloc((unsigned) *size)) == (char *) NULL) {
        Error("CreateSharedRegion: failed in valloc", (Integer) 0);
    }

    /* Now have to get a unique id ... try using time of day in centi-sec */

    if ( (status = gettimeofday(&tp, &tzp)) != 0) {
        Error("CreateSharedRegion: error from gettimeofday", (Integer) status);
    }

    *id = (tp.tv_sec + 10000*tp.tv_usec) & 0xffffff;

    /* Now make the region */

    if ( (status = create_shared_region(*id, temp, *size, 0)) != 0) {
        Error("CreateSharedRegion: error from create_shared_region",
                (Integer) status);
    }

    return temp;
}


Integer DetachSharedRegion(Integer id, Integer size, char *addr)
{
    return detach_shared_region( id, addr, size);
}


Integer DeleteSharedRegion(Integer id)
{
    return delete_shared_region(id);
}


char *AttachSharedRegion(Integer id, Integer size)
{
    char *temp;
    int status;

    if (size !=  (((size + 4095) / 4096) * 4096)) {
        Error("AttachSharedRegion: input size is not multiple of 4096",
                (Integer) size);
    }

    if ( (temp = valloc((unsigned) size)) == (char *) NULL) {
        Error("AttachSharedRegion: failed in valloc", (Integer) 0);
    }

    /* Now try to attach */

    if ( (status = attach_shared_region(id, temp, size)) != 0) {
        Error("AttachSharedRegion: error from attach_shared_region",
                (Integer) status);
    }

    return temp;
}
