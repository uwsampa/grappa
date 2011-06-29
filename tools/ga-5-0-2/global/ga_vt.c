#if HAVE_CONFIG_H
#   include "config.h"
#endif

#include <stdio.h>
#include <VT.h>
#include <mpi.h>

/* define VT_DEBUG */

/* This file is compiled only when a Vampirtrace instrumented version of the 
   GA is required.
*/

int vampirtrace_level = 0;
int vampirtrace_init  = 0;
int vampirtrace_level_galinalg = 0;
double vampirtrace_timestamp = 0.0;

#if defined(VT_DEBUG)
#define MAX_TRACE_LEVEL 255
int vt_traces[MAX_TRACE_LEVEL];
extern int tcg_nodeid();
#endif

void vampir_symdef(int id, char *state, char *activity, 
                   char *file, int line)
{
    int ierr;

    ierr = VT_symdef(id,state,activity);
    if (0!=ierr) {
        printf("ERROR %d: vampir_symdef: failed to define %s. %s:%d\n",
               ierr,state,file,line);
        fflush(stdout);
        abort();
    };
}

void vampir_begin(int id, char *file, int line)
{
    int ierr;

    if (id <= 0 || id >= VT_MAX_USERCODE) {
        printf("Error: VT_begin failed, <id>(%d) out of range. %s:%d\n",
               id,file,line);
        fflush(stdout);
        abort();
    };
    if (vampirtrace_level < 0) {
        printf("Error: Improper nesting vampir_begin %d. %s:%d\n",
               vampirtrace_level,file,line);
        fflush(stdout);
        abort();
    };
    if (vampirtrace_level == 0) {
        ierr = VT_begin(id);
        if (0!=ierr) {
            printf("Error %d: VT_begin failed. %s:%d\n",ierr,file,line);
            fflush(stdout);
            abort();
        };
    };
#if defined(VT_DEBUG)
    if (vampirtrace_level < MAX_TRACE_LEVEL) {
        vt_traces[vampirtrace_level] = id;
    } 
    else {
        printf("%d: Error: MAX_TRACE_LEVEL exceeded\n",tcg_nodeid());
        fflush(stdout);
        abort();
    };
#endif
    vampirtrace_level++;
}

void vampir_end(int id, char *file, int line)
{
    int ierr;
    int nodeid;

    if (id <= 0 || id >= VT_MAX_USERCODE) {
        printf("Error: VT_end failed, <id>(%d) out of range. %s:%d\n",
               id,file,line);
        fflush(stdout);
        abort();
    };
    vampirtrace_level--;
    if (vampirtrace_level < 0) {
        printf("Error: Improper nesting vampir_end %d. %s:%d\n",
               vampirtrace_level,file,line);
        fflush(stdout);
        abort();
    };
#if defined(VT_DEBUG)
    nodeid=tcg_nodeid();
    if (vt_traces[vampirtrace_level] != id) {
        printf("%d: Error: Improper nesting vampir_end. %s:%d\n",
               nodeid,file,line);
        printf("%d: Error: vampir_begin called with %d.\n",
               nodeid,vt_traces[vampirtrace_level]);
        printf("%d: Error: vampir_end   called with %d.\n",
               nodeid,id);
        fflush(stdout);
        abort();
    };
#endif
    if (vampirtrace_level == 0) {
        ierr = VT_end(id);
        if (0!=ierr) {
            printf("Error %d: VT_end failed. %s:%d\n",ierr,file,line);
            fflush(stdout);
            abort();
        };
    };
}

void vampir_send(int me, int other, int length, int type)
{
#if VT_VERSION <= 2010
    (void) VT_log_sendmsg(me,other,length,type,0);
#elif VT_VERSION >= 2090
    (void) VT_log_sendmsg(other,length,type,VT_COMM_WORLD,VT_NOSCL);
#else
    Not sure what the correct interface is.
    Please look at VT.h and check which of the above is the most appropriate.
#endif
};

void vampir_recv(int me, int other, int length, int type)
{
#if VT_VERSION <= 2010
    (void) VT_log_recvmsg(me,other,length,type,0);
#elif VT_VERSION >= 2090
    (void) VT_log_recvmsg(other,length,type,VT_COMM_WORLD,VT_NOSCL);
#else
    Not sure what the correct interface is.
    Please look at VT.h and check which of the above is the most appropriate.
#endif
};

void vampir_start_comm(int source, int other, int length, int type)
{
#if VT_VERSION <= 2010
    (void) VT_log_sendmsg(source,other,length,type,0);
#elif VT_VERSION >= 2090
    (void) VT_begin_unordered();
    vampirtrace_timestamp = VT_timestamp();
#else
    Not sure what the correct interface is.
    Please look at VT.h and check which of the above is the most appropriate.
#endif
};

void vampir_end_comm(int source, int other, int length, int type)
{
#if VT_VERSION <= 2010
    (void) VT_log_recvmsg(other,source,length,type,0);
#elif VT_VERSION >= 2090
    (void) VT_log_msgevent(source,other,length,type,VT_COMM_WORLD,
           vampirtrace_timestamp,VT_NOSCL,VT_NOSCL);
    (void) VT_end_unordered();
#else
    Not sure what the correct interface is.
    Please look at VT.h and check which of the above is the most appropriate.
#endif
};

void vampir_begin_gop(int mynode, int nnodes, int length, int type)
{
#if VT_VERSION <= 2010
    if (vampirtrace_level == 1) {
        if (mynode < nnodes-1) {
           (void) VT_log_sendmsg(mynode,mynode+1,length,type,0);
        };
        if (mynode > 0) {
           (void) VT_log_sendmsg(mynode,mynode-1,length,type,0);
        };
    };
#elif VT_VERSION >= 2090
    if (vampirtrace_level == 1) {
       (void) VT_begin_unordered();
       vampirtrace_timestamp = VT_timestamp();
    };
#else
    Not sure what the correct interface is. 
    Please look at VT.h and check which of the above is the most appropriate.
#endif
}

void vampir_end_gop(int mynode, int nnodes, int length, int type)
{
#if VT_VERSION <= 2010
    if (vampirtrace_level == 1) {
        if (mynode < nnodes-1) {
           (void) VT_log_recvmsg(mynode,mynode+1,length,type,0);
        };
        if (mynode > 0) {
           (void) VT_log_recvmsg(mynode,mynode-1,length,type,0);
        };
    };
#elif VT_VERSION >= 2090
    if (vampirtrace_level == 1) {
       (void) VT_log_opevent(VT_OP_ALLTOALL,VT_COMM_WORLD,0,nnodes,
              &length,&length,&vampirtrace_timestamp,VT_NOSCL);
       (void) VT_end_unordered();
    };
#else
    Not sure what the correct interface is. 
    Please look at VT.h and check which of the above is the most appropriate.
#endif
}


void vampir_begin_galinalg(int id, char *file, int line)
{
    int ierr;

    if (id <= 0 || id >= VT_MAX_USERCODE) {
        printf("Error: VT_begin failed, <id>(%d) out of range. %s:%d\n",
               id,file,line);
        fflush(stdout);
        abort();
    };
    if (vampirtrace_level_galinalg < 0) {
        printf("Error: Improper nesting vampir_begin %d. %s:%d\n",
               vampirtrace_level_galinalg,file,line);
        fflush(stdout);
        abort();
    };
    if (vampirtrace_level_galinalg == 0) {
        ierr = VT_begin(id);
        if (0!=ierr) {
            printf("Error %d: VT_begin failed. %s:%d\n",ierr,file,line);
            fflush(stdout);
            abort();
        };
    };
    vampirtrace_level_galinalg++;
}

void vampir_end_galinalg(int id, char *file, int line)
{
    int ierr;

    if (id <= 0 || id >= VT_MAX_USERCODE) {
        printf("Error: VT_end failed, <id>(%d) out of range. %s:%d\n",
               id,file,line);
        fflush(stdout);
        abort();
    };
    vampirtrace_level_galinalg--;
    if (vampirtrace_level_galinalg < 0) {
        printf("Error: Improper nesting vampir_end %d. %s:%d\n",
               vampirtrace_level_galinalg,file,line);
        fflush(stdout);
        abort();
    };
    if (vampirtrace_level_galinalg == 0) {
        ierr = VT_end(id);
        if (0!=ierr) {
            printf("Error %d: VT_end failed. %s:%d\n",ierr,file,line);
            fflush(stdout);
            abort();
        };
    };
}

void vampir_begin_galinalg_(int id, char *file, int line)
{
    vampir_begin_galinalg(id,file,line);
}

void vampir_end_galinalg_(int id, char *file, int line)
{
    vampir_end_galinalg(id,file,line);
}

void vampir_init(int argc, char **argv, char *file, int line)
{
    int ierr;
    int length;
    char err_msg[MPI_MAX_ERROR_STRING];

#ifndef MPI
    if (vampirtrace_init == 0) {
       ierr = MPI_Init(&argc, &argv);
       if (ierr) {
           printf("ERROR %d: MPI_Init failed. %s:%d\n",ierr,file,line);
           fflush(stdout);
           MPI_Error_string(ierr,err_msg,&length);
           printf("ERROR %s\n",err_msg);
           fflush(stdout);
           abort();
       }
    };
#endif
    vampirtrace_init++;
}

void vampir_finalize(char *file, int line)
{
    int ierr;
    int length;
    char err_msg[MPI_MAX_ERROR_STRING];

    vampirtrace_init--;
#ifndef MPI
    /* Normally this routine gets called twice:
       1) from within ARMCI_Finalize within ga_terminate
       2) from ga_terminate itself
       So this check tends to be more of a nuisance than a help...
    if (vampirtrace_level != 0) {
        printf("Error: Improper nesting vampir_finalize %d. %s:%d\n",
               vampirtrace_level,file,line);
        fflush(stdout);
        abort();
    };
    */
    if (vampirtrace_init == 0) {
       ierr = MPI_Finalize();
       if (ierr) {
           printf("ERROR %d: MPI_Finalize failed. %s:%d\n",ierr,file,line);
           fflush(stdout);
           MPI_Error_string(ierr,err_msg,&length);
           printf("ERROR %s\n",err_msg);
           fflush(stdout);
           abort();
       };
    };
#endif
}
