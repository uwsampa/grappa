#ifdef SPRNG_MPI
#include <mpi.h>
#endif
#include <time.h>
#include "sprng_interface.h"


#ifdef __STDC__
void get_proc_info_mpi(int *myid, int *nprocs)
#else
void get_proc_info_mpi(myid, nprocs)
int *myid, *nprocs;
#endif
{
#ifdef SPRNG_MPI
  MPI_Comm_rank(MPI_COMM_WORLD, myid);
  MPI_Comm_size(MPI_COMM_WORLD, nprocs);
#else
  *myid = 0;
  *nprocs = 1;
#endif
}


#ifdef __STDC__
int make_new_seed_mpi(void)
#else
int make_new_seed_mpi()
#endif
{
#ifdef SPRNG_MPI
  unsigned int temp2;
  int myid, nprocs;
  MPI_Comm newcomm;
  
  MPI_Comm_dup(MPI_COMM_WORLD, &newcomm); /* create a temporary communicator */
  
  MPI_Comm_rank(newcomm, &myid);
  MPI_Comm_size(newcomm, &nprocs);

  if(myid == 0)
    temp2 = make_new_seed();
  
  MPI_Bcast(&temp2,1,MPI_UNSIGNED,0,newcomm);

  MPI_Comm_free(&newcomm);
  
  return temp2;
#else
  return make_new_seed();
#endif
}


#if 0
main()
{
  printf("%u\n", make_new_seed());
  printf("%u\n", make_new_seed());
}
#endif
