/*********************************************************\
*                                                         *
*                                                         *
*                                                         *
*                                                         *
\*********************************************************/

#ifndef __IOBMARK_H
#define __IOBMARK_H

#include "common_inc.h"

void    ch_p_reads( void );
void    ch_p_writes( void );
ssize_t bm_read( int fd, const void *buf, size_t nbyte );
ssize_t bm_write( int fd, const void *buf, size_t nbyte );
int     bm_close( int fd  );
int     bm_open( const char *path, int flags, mode_t perms);
off_t   bm_lseek( int fd, off_t pos, int whence );

#include "iobmversion.h"

#endif



/*
Emacs reads the stuff in this comment to set local variables
Local Variables:
compile-command: "make error_routines.o"
End:
*/

