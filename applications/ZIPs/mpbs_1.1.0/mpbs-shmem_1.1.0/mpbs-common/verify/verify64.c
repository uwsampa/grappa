#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

/*
 * mpbs-verify
 *
 * Usage:
 *      mpbs-verify <sorted_file> <input files...>
 *
 * mpbs-verify uses the input data files and sorted output file from mpbs
 * to verify that the output file is sorted and that a checksum of the output
 * matches a checksum on all of the input data.
 *
 * In the first phase the PEs open an evenly distributed number of files and 
 * calculate checksums on them.  These local checksums are then reduced to a 
 * global checksum.  The sorted output file is then opened and read by the
 * PEs and another set of local checksums is calculated.  The test is successful
 * if the two global checksums match.
 *
 * The second phase does local checks of the sorted data by each PE, followed
 * by boundary sort verification between neighboring processors on the left
 * and right.  The test is successful if and only if sorted_file is actually
 * sorted.
 *
 */
#define BUFSIZE 8096

uint64_t file_checksum(char *fname, int *valid){
  uint64_t checksum = 0;
  uint64_t buff[BUFSIZE];

  uint64_t i = 0;
  uint64_t j;
  uint64_t k;
  int fd;

  uint64_t fsize;					
  uint64_t nblocks;

  int width = sizeof(uint64_t);
  int recs_per_block = BUFSIZE/width;
  
  ssize_t read_size;  
  
  fd = open(fname, O_RDONLY);
  fsize = lseek(fd, 0, SEEK_END);
  if(fsize == 0) {
    printf("ERROR: input file is empty!\n");
    if(valid) {
      *valid = 0;
    }
    return 0;
  }

  lseek(fd, 0, SEEK_SET);
  posix_fadvise(fd, 0, 0, POSIX_FADV_WILLNEED);
  
  nblocks = fsize / BUFSIZE;
  
  /*
   * Read whole blocks and calculate its checksum.
   */
  for( j = 0; j < nblocks; j++) {
    read_size = read(fd, buff, BUFSIZE);
    if( read_size > BUFSIZE ) {
      fprintf(stderr, "Error during file read.  %d bytes read.\n", read_size);
      if(valid) {
	*valid = 0;
      }
      return 0;
    }
    
    for(k = 0; k < recs_per_block; k++) 
      checksum ^= buff[k];
  }
  
  /*
   * Read the final partial block and calculate its checksum.
   */
  if(fsize % BUFSIZE) {
    read_size = read(fd, buff, BUFSIZE);
    recs_per_block = read_size / width;
    for(k = 0; k < recs_per_block; k++) 
      checksum ^= buff[k];
  }
  
  posix_fadvise(fd, 0, 0, POSIX_FADV_DONTNEED);
  close(fd);
  
  printf("%s: %lu\n", fname, checksum);

  if(valid) *valid = 1;
  
  return checksum;
}

int main(int argc, char *argv[]) {
  int id;
  int npes;

  // Size of the records in the input files.
  int width = sizeof(uint64_t);

  uint64_t indx;

  // Name and file descriptor of the sorted input file.
  char *fname_sorted;
  int fd;

  int isValid;

  // Size of the sorted input file and buffer where the data is stored.
  int64_t fsize;
  uint64_t buff[BUFSIZE];

  // Number of records in the sorted file this PE is responsible for checking.
  uint64_t nrecords;
  
  // Number of blocks this PE is responsible for checking.
  uint64_t nblocks;

  // Number of records in each block.
  int recs_per_block;

  // The offset in the file where this PE's data starts.
  uint64_t offset;
  
  uint64_t rem;
  uint64_t ii, j, k;
  
  // Number of input files.
  int gnumfiles;

  // Number of input files this PE is responsible for.
  int lnumfiles;

  // This PE's portion of the checksum.
  uint64_t lchecksum = 0;    

  // The global checksum calculated from the unsorted files (valid only on rank 0)
  uint64_t gchecksum = 0;

  // The global checksum calculated from the sorted file (valid only on rank 0)
  uint64_t schecksum = 0;

  // Minimum and maximum record in this PE's portion of the sorted file. 
  uint64_t min;
  uint64_t max;

  // Local and global sorting flags to indicate a sorting verificaiton problem.
  int lsortflag = 0;
  int gsortflag = 0;
 
  ssize_t bytesread;  

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &id);
  MPI_Comm_size(MPI_COMM_WORLD, &npes);

  if(argc < 2) {
    printf("Wrong number of arguments.  Usage:\n\t",
	   "mpbs-verify <sorted_file> <input_files...>\n");
    MPI_Finalize();
    exit(1);
  }

  fname_sorted = argv[1];

  /*
   * Calculate this PE's set of input files and starting index in the argv
   * array where they begin.
   */
  gnumfiles = argc - 2;
  lnumfiles = gnumfiles / npes;
  rem = gnumfiles - lnumfiles * npes;

  indx = id * lnumfiles + 2;  
  if(id >= rem)
    indx += rem;
  else if(id < rem) {
    lnumfiles++;
    indx += id;
  }

  /*
   * Read each input file and calculate a local checksum.
   */
  if(id == 0)
    printf("Calculating unsorted data file checksums...\n");
 
  for( ii = 0; ii < lnumfiles; ii++ ) {
    lchecksum ^= file_checksum(argv[indx + ii], &isValid); 
  
    if(!isValid) {
      printf("Checksum error.\n");
      MPI_Finalize();
      exit(2);
    }
  }
  
  MPI_Reduce(&lchecksum, &gchecksum, 1, MPI_UNSIGNED_LONG, MPI_BXOR, 0, MPI_COMM_WORLD);

  if(id == 0)
    printf("Unsorted data checksum: %lu\n", gchecksum);
    
  fd = open(fname_sorted, O_RDONLY);
  if(fd < 0) {
    perror("Couldn't open the sorted input file");
    MPI_Finalize();
    exit(2);
  }

  /*
   * Read the sorted file, verify the sort, and calculate a global checksum.
   */
  fsize = lseek(fd, 0, SEEK_END);
  if(fsize <= 0) {
    fprintf(stderr, "The sorted file is empty!\n");
    MPI_Finalize();
    exit(1);
  }
  lseek(fd, 0, SEEK_SET);
  
  /*
   * Divide up work on the sorted file to each of the PEs.
   */
  nrecords = fsize / (width * npes);

  rem = (fsize / width) - nrecords * npes;
  offset = id * nrecords * width;
  if(id >= rem) 
    offset += rem * width;
  else if(id < rem) {
    nrecords++;
    offset += id * width;
  }

  nblocks = (nrecords * width) / BUFSIZE;
  recs_per_block = BUFSIZE / width;
    
  /*
   * Seek to the start of the sorted file section this PE is responsible for 
   * and read in nblocks whole chunks to checksum.
   */
#ifdef DEBUG
  printf("(%d) Checking offset %lu to %lu\n", id, offset, offset + nrecords * width);
#endif
  
  if( lseek(fd, offset, SEEK_SET) < 0 ) {
    perror("Sorted file lseek.");
    fprintf(stderr, "Could not set offset to %lu!\n", offset);
    MPI_Finalize();
    exit(1);
  }

  /*
   * Grab the first element for the sorting verification.
   */
  read(fd, &min, sizeof(uint64_t));
  lseek(fd, offset, SEEK_SET);
  
  if(id == 0)
    printf("Calculating the sorted file checksum...\n");

  lchecksum = 0;
  for( j = 0; j < nblocks; j++) {
    if(id == 0 && (j % 10000 == 0))
      printf("Block %d of %d\n", j, nblocks);

    bytesread = read(fd, buff, BUFSIZE);
    if( bytesread > BUFSIZE ) {
      fprintf(stderr, "Error during sorted file read.  %d bytes read.\n", bytesread);
      MPI_Finalize();
      exit(1);
    }
    
    for(k = 0; k < recs_per_block; k++) 
      lchecksum ^= buff[k];

    /*
     * Local sort verification. Don't break here because we still might want to know
     * the checksums are OK, even if the data isn't sorted.
     */
    for(k = 0; k < recs_per_block; k++) {
      if(!buff[k]) {
	printf("(%d) Found a zero record. This is almost certainly an error.\n", id);
      }
      if(buff[k] < min) {
	printf("(%d) Found local sort error: min = %lu, buff[k] = %lu\n", id, min, buff[k]);
	lsortflag = 1;
	break;
      }
    }
    max = buff[recs_per_block - 1];
  }

  /*
   * Read in the final partial block (taking care not to read into the next PE's chunk).
   */
  rem = (nrecords * width) - nblocks * BUFSIZE;
  if (rem) {
    bytesread = read(fd, buff, rem);
    recs_per_block = bytesread / width;
    for(k = 0; k < recs_per_block; k++) 
      lchecksum ^= buff[k];
    for(k = 0; k < recs_per_block; k++) {
      if(buff[k] < min) {
	lsortflag = 1;
	break;
      }
    }
    max = buff[recs_per_block - 1];
  }

  MPI_Reduce(&lchecksum, &schecksum, 1, MPI_UNSIGNED_LONG, MPI_BXOR, 0, MPI_COMM_WORLD);

  if(id == 0) {
    printf("Sorted checksum: %lu\n", schecksum);
    if(gchecksum != schecksum)
      printf("Checksum verification... FAILED\n");
    else
      printf("Checksum verification... PASSED\n");
  }
  

  /*
   * Global verification of sort.  Each PE has already done a local sort verification above,
   * so each PE just checks with their left neighbor to verify its position in the total ordering.
   */
  if(id == 0) {
    printf("Sorting verification...  ");
    fflush(stdin);
  }
  
  /*
   * Verify the sort with your left neighbor. 
   */
  uint64_t temp64;

  MPI_Status status;
  if(id != 0) {
    MPI_Send(&min, 1, MPI_UNSIGNED_LONG, id-1, 0, MPI_COMM_WORLD);
  }
  if(id != npes - 1){
    MPI_Recv(&temp64, 1, MPI_UNSIGNED_LONG, id+1, 0, MPI_COMM_WORLD, &status);
  }

  MPI_Barrier(MPI_COMM_WORLD);

  if(id != npes - 1) {
    /*
     * If my max element is larger than the min element of my right neighbor, then 
     * the PE's are not in the correct order.
     */
    if(max > temp64) 
      lsortflag = 1; 
  }
  
  MPI_Reduce(&lsortflag, &gsortflag, 1, MPI_INT, MPI_LOR, 0, MPI_COMM_WORLD);  

  if(id == 0) {
    if(gsortflag)
      printf("FAILED\n");
    else
      printf("PASSED\n");
  }

  MPI_Finalize();

  return 0;
}
