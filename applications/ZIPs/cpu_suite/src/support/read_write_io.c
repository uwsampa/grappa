/* CVS info                         */
/* $Date: 2010/03/18 17:17:18 $     */
/* $Revision: 1.5 $                 */
/* $RCSfile: read_write_io.c,v $    */
/* $State: Stab $:                   */

/*
     This file contains the lowlevel programs that deal with files.

     ch_p_reads   report partial reads
     ch_p_writes  report partial writes
     syserror     convert errno to words
     bm_read      read a file
     bm_write     write a file
     bm_close     close a file
     bm_open      open a file
     bm_lseek     seek to a position in a file

*/

#include "common_inc.h"

#include <sys/stat.h>

#include "error_routines.h"

// The macro GOODBLKSZ is a tunable number. Routines
// nbm_read and nbm_write use this number. Large IO
// is divided into operations of this size.

#ifndef GOODBLKSZ
#  define GOODBLKSZ  ((size_t)4*1024*1024L)
#endif

int partial_read   = 0;
int partial_writes = 0;
int r_err_type = 0;
int w_err_type = 0;
int o_err_type = 0;
int c_err_type = 0;


void ch_p_reads( void ){
  if( partial_read )
    err_msg_ret("%d partial reads\n", partial_read );
}

void ch_p_writes( void ){
  if( partial_writes )
    err_msg_ret("%d partial writes\n", partial_writes );
}

//97
ssize_t bm_read( int fd, const void *buf, size_t nbyte ){

  ssize_t nread = 0, n;

  do {
    errno = 0;

    if((n = read(fd, &((char *)buf)[nread], nbyte - nread)) == -1){
      if (errno == EINTR){
	partial_read++;
	continue;
      }else{
	r_err_type = errno;
	sys_err_ret( errno, "ERROR in bm_read");
	return (ssize_t) -1;
      }
    }

    if (n == 0)
      return nread;
    nread += n;
  }while ((size_t)nread < nbyte);
  return nread;
}


// 95
ssize_t bm_write( int fd, const void *buf, size_t nbyte ){

  ssize_t nwritten = 0, n;

  int partial_writes = 0;

  do {
    errno = 0;
    if(( n = write(fd, &((const char *) buf)[nwritten],
		   nbyte - nwritten)) == -1){
      if(errno == EINTR){
	partial_writes++;
	continue;
      }
      else{
	w_err_type = errno;
        sys_err_ret( errno, "ERROR in bm_write");
	return -1;
      }
    }
    nwritten += n;
  } while ( (size_t)nwritten < nbyte );

  return nwritten;
}


int bm_close( int fd  ){
  errno = 0;
  if ( close( fd  ) == -1 ){
    c_err_type = errno;
    sys_err_ret( errno, "ERROR in bm_close");
    return -1;
  }
  return 0;
}

int bm_open( const char *path, int flags, mode_t perms){
  int fd = -1;

  errno = 0;
  if((fd = open( path, flags, perms)) == -1){
    sys_err_ret( errno, "ERROR in bm_open file %s ", path);
    return -1;
  }

  return fd;
}

off_t bm_lseek( int fd, off_t pos, int whence ){
    off_t new_off;

    errno = 0;
    if( (new_off = lseek( fd, pos, whence ) ) == (off_t) -1 ){
      sys_err_ret( errno, "ERROR in bm_lseek");
    }
    return new_off;
}

ssize_t nbm_read( const int fd, const void *buf, const size_t nbyte ){

  ssize_t n;
  size_t
    nread,          // Number of bytes read into current block
    tnread = 0,     // Total number of bytes read
    b_per_chunk,    // Bytes in a full or partial block
    bcnt,           // Number of blocks to read
    chnksz = GOODBLKSZ;
  int i;

  // Convert a read of nbytes into bcnt reads. The first read is
  // either a full or partial chunk of data. Any remaining data is
  // read as full chunks.

  bcnt = (nbyte + chnksz - 1)/chnksz; // Count of full and partial blocks
  b_per_chunk = nbyte % chnksz;       // Any remaining bytes

  if ( b_per_chunk == 0 ){
    b_per_chunk = chnksz;
  }

  for( i=0; (size_t)i<bcnt; i++ ){      // Loop over chunks
    nread = 0;
    do {  // Read in data and handle any partial reads

     errno = 0;
      if((n = read(fd, &((char *)buf)[tnread], b_per_chunk - nread)) == -1){
	if( errno == EINTR ){
	  partial_read++;
	}else{
	  r_err_type = errno;
	  sys_err_ret( errno, "ERROR in nbm_read");
	  return (ssize_t) -1;
	}
      }
      if( n == 0)
	break;
      if ( n != -1 ){
	nread  += (size_t)n;
      }
    }while (nread < b_per_chunk);
    tnread +=  nread;
    b_per_chunk = chnksz;  // The rest of the reads are of full blocks
  }
  return (ssize_t)tnread;
}

ssize_t nbm_write( const int fd, const void *buf, size_t nbyte ){

  ssize_t n;
  size_t
    nwritten = 0,
    tnwritten,
    b_per_chunk,
    chnksz = GOODBLKSZ;
  int i, bcnt;

  bcnt = (int)((nbyte + chnksz - 1)/chnksz); // Count of full and partial blocks
  b_per_chunk = nbyte % chnksz;       // Any remaining bytes

  if ( b_per_chunk == 0 ){
    b_per_chunk = chnksz;
  }

  for( i=0; i<bcnt; i++ ){      // Loop over chunks
    tnwritten = 0;
    do {
      errno = 0;
      if(( n = write(fd, &((const char *) buf)[nwritten],
		     nbyte - nwritten)) == -1){
	if(errno == EINTR){
	  partial_writes++;
	}
	else{
	  w_err_type = errno;
	  sys_err_ret( errno, "ERROR in nbm_write");
	  return -1;
	}
      }
      if( n == 0)
	break;
      nwritten += n;
      tnwritten += n;
    }while (nwritten < b_per_chunk);
    b_per_chunk = chnksz;  // The rest of the writes are of full blocks
  }

  return (ssize_t)nwritten;
}

/*
Emacs reads the stuff in this comment to set local variables
Local Variables:
compile-command: "make read_write_io.o "
End:
*/

