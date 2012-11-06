#include <stdint.h>
#include <stdio.h>

#include <mpp/shmem.h>

void check_shptrs(int rank, int npes, void *s, char *str ) {
  int                  i;
  uint64_t        my_ptr; // Pointer to shmalloced memory on PE 0.
  static uint64_t shptrs;
  
  /*
   * place pointer in variable depending on id
   */
  if( rank == 0 )
    my_ptr = (uint64_t)s;
  else
    shptrs = (uint64_t)s;
  
  /*
   * process 0 will check its pointer against all other
   * pointers
   */
  if( rank == 0 ) {
    /*
     * loop over all other processes
     */
    for( i = 1; i < npes; i++ ) {
      /*
       * grab pointer from pe i
       */
      shmem_get64( &shptrs, &shptrs, 1, i );
      
      /*
       * check pointer against pe 0's
       */
      if( my_ptr != shptrs ) {
	printf( "Failure in '%s' array shmallocation!\n", str );
	printf( "check_shptrs> ERROR: inconsistent shmem \n" );
	printf( "ptrs PE %u (%p) PE %u (%p)\n",
		rank, (void *)my_ptr, i, (void *)shptrs );
	printf( "exiting program...\n" );
      }
    }
  }
  return;
}
