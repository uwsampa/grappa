/* CVS info                     */
/* $Date: 2010/03/18 17:21:40 $ */
/* $Revision: 1.3 $             */
/* $RCSFile$                    */
/* $State: Stab $:               */


/*****************************************************\
*  These routines are use with parallel programming   *
* models like shmem, mpi, and upc. Each PE has a      *
* unique id. This id is useful for segregating PE     *
* input and output.                                   *
\*****************************************************/

#include "uqid.h"

#ifdef HAS_SHMEM
#  ifdef CRAY_SHMEM
#    include <mpp/shmem.h>
#  elif defined HP_SHMEM
#    include <shmem.h>
#  endif
#endif

#ifdef HAS_MPI
#  ifdef CRAY_MPI
#    include <mpi.h>
#  elif defined HP_SHMEM
#    include <mpi.h>
#  endif
#endif

#ifdef HAS_UPC
#  ifdef CRAY_UPC
#    include <upc.h>
#  elif defined HP_UPC
#    include <upc.h>
#  endif
#endif


#ifdef HAS_SHMEM
#include <stdlib.h>

void uqid_init(void){
  shmem_init();
}

void uqid(int* uid){
    *uid = shmem_my_pe();
  return;
}

void uqid_finalize(void){
#ifdef CRAY_SHMEM
  shmem_finalize();
#endif
}

void uqid_barrier(void){
  shmem_barrier_all();
}

void uqid_exit(int ec){
  exit(ec);
}
#endif


#ifdef HAS_MPI
// Following to get NULL
#include <stdio.h>
#include <mpi.h>
void uqid_init(void){
  (void) MPI_Init(NULL, NULL);
}

void uqid(int* uid){
  (void) MPI_Comm_rank(MPI_COMM_WORLD, uid);
  return;
}

void uqid_finalize(void){
  (void) MPI_Finalize();
}

void uqid_barrier(void){
  (void) MPI_Barrier(MPI_COMM_WORLD);
}
void uqid_exit(int ec){
  (void) MPI_Abort(MPI_COMM_WORLD, ec);
}
#endif

#ifdef HAS_UPC
void uqid_init(void){
}

void uqid(int* uid){
  *uid = MYTHREAD;
  return;
}

void uqid_finalize(void){
}

void uqid_barrier(void){
  upc_barrier;
}

void uqid_exit(int ec){
  upc_global_exit(ec);
}
#endif

#ifdef HAS_SLURM
#include <stdlib.h>

void uqid_init(void){
}

void uqid(int* uid){
char *tmp;
  tmp = getenv("SLURM_PROCID");
  *uid = atoi(tmp);
  return;
}

void uqid_finalize(void){
}

void uqid_barrier(void){
}

void uqid_exit(int ec){
 exit(ec);
}
#endif


#if ( (!defined(HAS_SHMEM)) && (!defined(HAS_MPI)) && (!defined(HAS_UPC)) && (!defined(HAS_SLURM)))
/* Normal serial code for workstations or farms */
#include <stdlib.h>
void uqid_init(void){
}

void uqid(int* uid){
  *uid = -1;
  return;
}

void uqid_finalize(void){
}

void uqid_barrier(void){
}

void uqid_exit(int ec){
  exit(ec);
}
#endif


#ifdef TEST_UQID
/*****************************************************\
Test the uquid code works with serial and parallel
code

cc uqid.c -DTEST_UQID -DHAS_SHMEM -DCRAY_SHMEM
cc uqid.c -DTEST_UQID -DHAS_MPI -DCRAY_MPI
cc -h upc  uqid.c -DTEST_UQID -DHAS_UPC -DCRAY_UPC
cc uqid.c -DTEST_UQID
\*****************************************************/

#include <stdio.h>
int main(int argc, char** argv){
  int my_uid;

  uqid_init();
  uqid(&my_uid);
  printf("my_uid = %d before barrier\n", my_uid);
  uqid_barrier();
  printf("after barrier\n");

  if (argc == 1){
    printf("using uqid_finalize\n");
    uqid_finalize();
  }else{
    printf("using uqid_exit(7)\n");
    uqid_exit(7);
  }
  return 0;
}

#endif



/*
Emacs reads the stuff in this comment to set local variables
Local Variables:
compile-command: "make uqid.o"
End:
*/



