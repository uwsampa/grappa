#if HAVE_CONFIG_H
#   include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <unistd.h>

#include <mpi.h>
#define MP_BARRIER()      MPI_Barrier(MPI_COMM_WORLD)
#define MP_FINALIZE()     MPI_Finalize()
#define MP_INIT(arc,argv) MPI_Init(&(argc),&(argv))
#define MP_MYID(pid)      MPI_Comm_rank(MPI_COMM_WORLD, (pid))
#define MP_PROCS(pproc)   MPI_Comm_size(MPI_COMM_WORLD, (pproc));

#include "armci.h"

int me, nproc;
extern void armcill_lock(int,int);
extern void armcill_unlock(int,int);


void test_lock()
{
int i,mut;
    if(me==0)printf("\n");
    for(mut=0;mut<16;mut++)
      for(i=0;i<nproc;i++){ 
        armcill_lock(mut,i);
        armcill_unlock(mut,i);
        MP_BARRIER();
        if(me==0){printf(".");fflush(stdout);}
        MP_BARRIER();
      }
}


int main(int argc, char* argv[])
{
    int ndim;

    MP_INIT(argc, argv);
    MP_PROCS(&nproc);
    MP_MYID(&me);

    if(me==0){
       printf("ARMCI test program for lock(%d processes)\n",nproc); 
       fflush(stdout);
       sleep(1);
    }
    
    ARMCI_Init();

    test_lock();

    MP_BARRIER();
    if(me==0){printf("test passed\n"); fflush(stdout);}
    sleep(2);

    MP_BARRIER();
    ARMCI_Finalize();
    MP_FINALIZE();
    return(0);
}
