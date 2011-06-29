#if HAVE_CONFIG_H
#   include "config.h"
#endif

#if HAVE_STDLIB_H
#   include <stdlib.h>
#endif

#include "mpi.h"
#include "macdecls.h"
#include "ga.h"
#include "armci.h"

int main(int argc, char **argv)
{
    int desired = MPI_THREAD_MULTIPLE;
    int provided;
    //printf("using MPI_Init_thread\n");
    MPI_Init_thread(&argc, &argv, desired, &provided);

    int me;
    int nproc;
    MPI_Comm_rank(MPI_COMM_WORLD,&me);
    MPI_Comm_size(MPI_COMM_WORLD,&nproc);

    printf("%d: Hello world!\n",me);

    if ( me == 0 )
    {
        switch (provided)
        {
            case MPI_THREAD_MULTIPLE:
                printf("%d: provided = MPI_THREAD_MULTIPLE\n",me);
                break;

            case MPI_THREAD_SERIALIZED:
                printf("%d: provided = MPI_THREAD_SERIALIZED\n",me);
                break;

            case MPI_THREAD_FUNNELED:
                printf("%d: provided = MPI_THREAD_FUNNELED\n",me);
                break;

            case MPI_THREAD_SINGLE:
                printf("%d: provided = MPI_THREAD_SINGLE\n",me);
                break;

            default:
                printf("%d: MPI_Init_thread returned an invalid value of <provided>.\n",me);
                return(provided);
                
        }
    }

    if (me==0) printf("%d: GA_Initialize\n",me);
    GA_Initialize();
    //if (me==0) printf("%d: ARMCI_Init\n",me);
    //ARMCI_Init();
    //if (me==0) printf("%d: MA_Init\n",me);
    //MA_init(MT_DBL, 8*1024*1024, 2*1024*1024);

    const int ndim=1;
    int status;

    if (me==0) printf("%d: GA_Create_handle\n",me);
    int g_a = GA_Create_handle();

    if (me==0) printf("%d: GA_Set_array_name\n",me);
    GA_Set_array_name(g_a,"test array A");

    int dims[ndim];
    dims[0] = 30;
    if (me==0) printf("%d: GA_Set_data\n",me);
    GA_Set_data(g_a,ndim,dims,MT_DBL);

    int chunk[ndim];
    chunk[0] = -1;
    if (me==0) printf("%d: GA_Set_chunk\n",me);
    GA_Set_chunk(g_a,chunk);

    if (me==0) printf("%d: GA_Pgroup_get_world\n",me);
    int pg_world = GA_Pgroup_get_world();
    if (me==0) printf("%d: GA_Set_pgroup\n",me);
    GA_Set_pgroup(g_a,pg_world);

    if (me==0) printf("%d: GA_Allocate\n",me);
    status = GA_Allocate(g_a);
    if(0 == status) MPI_Abort(MPI_COMM_WORLD,100);

    if (me==0) printf("%d: GA_Zero\n",me);
    GA_Zero(g_a);

    if (me==0) printf("%d: GA_Sync\n",me);
    GA_Sync();

    size_t num = 10;
    double* p1 = malloc(num*sizeof(double));
    //double* p1 = ARMCI_Malloc_local(num*sizeof(double));
    if (p1==NULL) MPI_Abort(MPI_COMM_WORLD,1000);
    double* p2 = malloc(num*sizeof(double));
    //double* p2 = ARMCI_Malloc_local(num*sizeof(double));
    if (p2==NULL) MPI_Abort(MPI_COMM_WORLD,2000);

    size_t i;
    for ( i=0 ; i<num ; i++ ) p1[i] = 7.0;
    for ( i=0 ; i<num ; i++ ) p2[i] = 3.0;

    int num_mutex = 17;
    status = GA_Create_mutexes(num_mutex);
    if (me==0) printf("%d: GA_Create_mutexes = %d\n",me,status);

    int lo[1];
    int hi[1];
    int ld[1]={1};
/***************************************************************/
    if (me==0) {
        printf("%d: before GA_Lock\n",me);
        GA_Lock(0);
        lo[0] = 0;
        hi[0] = num-1;
        GA_Init_fence();
        NGA_Put(g_a,lo,hi,p1,ld);
        GA_Fence();
        GA_Unlock(0);
        printf("%d: after GA_Unlock\n",me);
    } 
    GA_Print(g_a);
    if (me==1) {
        printf("%d: before GA_Lock\n",me);
        GA_Lock(0);
        lo[0] = 0;
        hi[0] = num-1;
        GA_Init_fence();
        NGA_Get(g_a,lo,hi,p2,ld);
        GA_Fence();
        GA_Unlock(0);
        printf("%d: after GA_Unlock\n",me);
        for ( i=0 ; i<num ; i++ ) printf("p2[%2lu] = %20.10f\n",
                (long unsigned)i,p2[i]);
    }
/***************************************************************/



    status = GA_Destroy_mutexes();
    if (me==0) printf("%d: GA_Destroy_mutexes = %d\n",me,status);

    //ARMCI_Free(p2);
    //ARMCI_Free(p1);
    free(p2);
    free(p1);

    if (me==0) printf("%d: GA_Destroy\n",me);
    GA_Destroy(g_a);

    //if (me==0) printf("%d: ARMCI_Finalize\n",me);
    //ARMCI_Finalize();
    if (me==0) printf("%d: GA_Terminate\n",me);
    GA_Terminate();
    if (me==0) printf("%d: MPI_Finalize\n",me);
    MPI_Finalize();

    return(0);
}
