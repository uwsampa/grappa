/* CVS info                     */
/* $Date: 2010/04/01 21:59:23 $ */
/* $Revision: 1.7 $             */
/* $RCSfile: gen_file.c,v $     */

/*
  This is a simple program to produce files of a given size.
  The file is filled with a sequence of lower case letters.
  Every 1024 byte block is identical and has the same checksum.
  The checksum is defined in data_blks.h  .
  File sizes ending in k or K indicate KiBytes 1024.
  File sizes ending in m or M indicate MiBytes 1024*1024.
*/

#include "common_inc.h"
#include "data_blks.h"
#include "bm_timers.h"

#define NUL ('\0')

void usage(char* prgn){
  printf("Usage: %s [-h] [-v] filename size[K|M|G|T|P]\n", prgn );
  printf("   -h print this message\n");
  printf("   -v verbose, print timing data\n");
  printf("   filename name of file created\n");
  printf("   size of file in units of KiBytes, MiBytes, etc\n");
}

int main(int argc, char** argv){

  FILE* File_ptr;
  char* filename;
  long  fsize;         // in KiBs
  int len;
  long scale = 1024L;
  char t;
  int fd;
  int j = 0;
  long i;
  char* data;
  long blksz = 1024L;
  unsigned long cksm = 0;
  int verbose = 0;;
  int opt;
  char* size;
  int first = 1;

  if (  (data=malloc(1024L*1024L)) == NULL ){
    fprintf(stderr, "ERROR: Can't allocate space\n");
    exit( 1 );
  }

  while ((opt = getopt(argc, argv, "hv")) != -1) {
    switch (opt) {
    case 'h':
      usage(argv[0]);
      return 0;
      break;
    case 'v':
      verbose = 1;
      break;
    default: /* '?' */
      usage(argv[0]);
      exit(EXIT_FAILURE);
    }
  }

  if ( (argc - optind)  < 2 ){
    fprintf(stderr, "ERROR: Missing argument(s)\n");
    usage(argv[0]);
    exit(EXIT_FAILURE);
  }
  if ( (argc - optind)  > 2 ){
    fprintf(stderr, "ERROR: Too many arguments\n");
    usage(argv[0]);
    exit(EXIT_FAILURE);
  }

  filename = argv[optind];
  size = argv[optind+1];
  len = strlen(size);
  t = size[len-1];

  /*@unused@*/
  static char cvs_info[] = "BMARKGRP $Date: 2010/04/01 21:59:23 $ $Revision: 1.7 $ $RCSfile: gen_file.c,v $ $Name:  $";

  if ( (t=='k') || (t=='K') ){
    scale = 1024L;
    blksz = 1024L;
  }
    // MiBytes
  else if ( (t=='m') || (t=='M') ){
    scale = 1024*1024L;
    blksz = 1024*1024L;
  }
  // GiBytes
  else if ( (t=='g') || (t=='G') ){
    scale = 1024L*1024L*1024L;
    blksz = 1024L*1024L;
  }
  // TiBytes
  else if ( (t=='t') || (t=='T') ){
    scale = 1024L*1024L*1024L*1024L;
    blksz = 1024L*1024L;
  }
  // PiBytes
  else if ( (t=='p') || (t=='P') ){
    scale = 1024L*1024L*1024L*1024L*1024L;
    blksz = 1024L*1024L;
  }
  else{
	printf("Invalid size given\n");
	printf("You must use of these indicators - K M G T P - as part of the size\n");
	printf("For example, gen_file large_file 490456K\n");
	exit (-1);
  }

  fsize = atoi(size)*(scale/blksz);

  /* Place 1024 bytes into array data and checksum */
  if ( (cksm = data_block_1024(data)) != __BM_VALID_IO_CHSM ){
    fprintf(stderr, "ERROR checksum is %016lx should be %016lx\n", cksm, __BM_VALID_IO_CHSM  );
    exit( EXIT_FAILURE );
  }

  /* Fill buffer with blocks of data */
  fill_buffer(data, (size_t) 1024L);

  // Open the file for writing
  errno = 0;
  if( (File_ptr = fopen(filename, "w") ) == NULL){
    fprintf(stderr,"ERROR open output \"%s\" failed: %s\n",
            filename, strerror(errno));
    exit( 2 );
  }

  double t0, t1,ttt;
  ttt = t0 = wall();
  for (i=0; i<fsize; i++){
    size_t wrtn = 0;
    wrtn = fwrite( data , blksz, 1, File_ptr );  // Writes KiB or MiB blocks
    if ( wrtn != 1){
      fprintf(stderr, "ERROR incomplete write of file %s\n", filename);
      exit( EXIT_FAILURE );
    }
    if ( (verbose == 1) && (((i+1)%1024) == 0) ){
      double a = wall();
      printf("%15.2lf MiBytes/s  %# 4.2lf%% remaining\n",
	     (blksz*1024/(a-ttt))/(1024L*1024L), (double)(fsize-i)*100/(double)fsize );
      ttt=a;
    }
  }

  t1 = wall();

  if ( (verbose == 1) ){
    printf("%8.2lf sec   %8.2lf MiBytes/s \n",  t1 - t0, ((double)blksz*fsize/(t1-t0))/(1024L*1024L) );
  }

  // Close the file
  if ( fclose( File_ptr ) == EOF ){
    exit( 4 );
  }

  errno = 0;
  if ( (fd = open(filename, O_RDONLY)) == -1){
    fprintf(stderr, "ERROR couldn't reopen file %s : %s", filename, strerror(errno) );
  }

  errno = 0;
  if ( fsync(fd) == -1){
    fprintf(stderr, "ERROR couldn't fsync file %s : %s", filename, strerror(errno) );
  }else{
    errno = 0;
    if ( close( fd ) == -1){
      fprintf(stderr, "ERROR couldn't close file %s : %s", filename, strerror(errno) );
    }
  }

  return 0;
} // End Main


/*
Emacs reads the stuff in this comment to set local variables
Local Variables:
compile-command: "make gen_file"
End:
*/

