#if HAVE_CONFIG_H
#   include "config.h"
#endif

#include <mpi.h>

extern void exit(int status);

#include "tcgmsgP.h"
#include "srftoc.h"
#ifdef USE_VAMPIR
#   include "tcgmsg_vampir.h"
#endif

char     tcgmsg_err_string[ERR_STR_LEN];
MPI_Comm TCGMSG_Comm;
int      _tcg_initialized=0;
Integer  DEBUG_;
int      SR_parallel; 
int      SR_single_cluster =1;

static int SR_initialized=0;


Integer TCGREADY_()
{
    return (Integer)SR_initialized;
}


/**
 * number of processes
 */
Integer NNODES_()
{
    int numprocs;

    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
#ifdef NXTVAL_SERVER
    if(SR_parallel) {
        return((Integer)numprocs-1);
    }
#endif
    return((Integer)numprocs);
}


/**
 * Get calling process id
 */
Integer NODEID_()
{
    int myid;

    MPI_Comm_rank(MPI_COMM_WORLD,&myid);
    return((Integer)myid);
}


void Error(char *string, Integer code)
{
    fprintf(stdout, FMT_INT ": %s " FMT_INT " (%#lx).\n",
            NODEID_(), string, code, (long unsigned int)code);
    fflush(stdout);
    fprintf(stderr, FMT_INT ": %s " FMT_INT " (%#lx).\n",
            NODEID_(), string, code, (long unsigned int)code);

    finalize_nxtval(); /* clean nxtval resources */
    MPI_Abort(MPI_COMM_WORLD,(int)code);
}


/**
 * this is based on the MPI Forum decision that MPI_COMM_WORLD is a C constant 
 */
void make_tcgmsg_comm()
{
    extern int single_cluster();

#ifdef NXTVAL_SERVER
    if( SR_parallel ){   
        /* data server for a single process */
        int server;
        MPI_Group MPI_GROUP_WORLD, tcgmsg_grp;

        MPI_Comm_size(MPI_COMM_WORLD, &server);
        server --; /* the highest numbered process will be excluded */
        MPI_Comm_group(MPI_COMM_WORLD, &MPI_GROUP_WORLD);
        MPI_Group_excl(MPI_GROUP_WORLD, 1, &server, &tcgmsg_grp); 
        MPI_Comm_create(MPI_COMM_WORLD, tcgmsg_grp, &TCGMSG_Comm); 
    }else
#endif
        TCGMSG_Comm = MPI_COMM_WORLD; 
}


/**
 * Alternative initialization for C programs
 * used to address argv/argc manipulation in MPI
 */
void tcgi_alt_pbegin(int *argc, char **argv[])
{
    int numprocs, myid;
    int init=0;

    if(SR_initialized) {
        Error("TCGMSG initialized already???",-1);
    } else {
        SR_initialized=1;
    }

    /* check if another library initialized MPI already */
    MPI_Initialized(&init);

    if(!init){ 
        /* nope */
#if defined(DCMF) 
        int desired = MPI_THREAD_MULTIPLE; 
        int provided; 
        MPI_Init_thread(argc, argv, desired, &provided); 
        MPI_Comm_rank(MPI_COMM_WORLD,&myid);  
        if ( myid == 0 ){ 
            if ( provided == MPI_THREAD_MULTIPLE ){ 
                printf("Using MPI_THREAD_MULTIPLE\n"); 
            } else if ( provided == MPI_THREAD_FUNNELED ){ 
                printf("Using MPI_THREAD_FUNNELED\n"); 
            } else if ( provided == MPI_THREAD_SERIALIZED ){ 
                printf("Using MPI_THREAD_SERIALIZED\n"); 
            } else if ( provided == MPI_THREAD_SINGLE ){ 
                printf("Using MPI_THREAD_SINGLE\n"); 
            } 
        } 
#else 
        MPI_Init(argc, argv);
#endif

#ifdef USE_VAMPIR
        tcgmsg_vampir_init(__FILE__,__LINE__);
#endif
        MPI_Errhandler_set(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
    }

#ifdef USE_VAMPIR
    vampir_begin(TCGMSG_PBEGINF,__FILE__,__LINE__);
#endif

    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);
    SR_parallel = numprocs > 1 ? 1 : 0;

    make_tcgmsg_comm();
    MPI_Barrier(MPI_COMM_WORLD);
    /* printf("%d:ready to go\n",NODEID_()); */
    install_nxtval(argc, argv);
#ifdef USE_VAMPIR
    vampir_end(TCGMSG_PBEGINF,__FILE__,__LINE__);
#endif
}


/**
 * Initialization for C programs
 */
void tcgi_pbegin(int argc, char* argv[])
{
    tcgi_alt_pbegin(&argc, &argv);
}


/**
 * shut down message-passing library
 */ 
void FATR PEND_()
{
#ifdef USE_VAMPIR
    vampir_begin(TCGMSG_PEND,__FILE__,__LINE__);
#endif
#ifdef NXTVAL_SERVER
    Integer zero=0;
    if( SR_parallel ) {
        (void) NXTVAL_(&zero);
    }
    MPI_Barrier(MPI_COMM_WORLD);
#endif
    finalize_nxtval();
#ifdef USE_VAMPIR
    vampir_end(TCGMSG_PEND,__FILE__,__LINE__);
#endif
    MPI_Finalize();
    exit(0);
}


double FATR TCGTIME_()
{
    static int first_call = 1;
    static double first_time, last_time, cur_time;
    double diff;

    if (first_call) {
        first_time = MPI_Wtime();
        first_call = 0;
        last_time  = -1e-9; 
    }

    cur_time = MPI_Wtime();
    diff = cur_time - first_time;

    /* address crappy MPI_Wtime: consectutive calls must be at least 1ns apart  */
    if(diff - last_time < 1e-9) {
        diff +=1e-9;
    }
    last_time = diff;

    return diff;                  /* Add logic here for clock wrap */
}


Integer MTIME_()
{
    return (Integer) (TCGTIME_()*100.0); /* time in centiseconds */
}



/**
 * Integererface from Fortran to C error routine
 */
void PARERR_(Integer *code)
{
    Error("User detected error in FORTRAN", *code);
}


void SETDBG_(Integer *onoff)
{
    DEBUG_ = *onoff;
}

void FATR STATS_()
{
    printf("STATS not implemented\n");
} 
