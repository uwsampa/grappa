#if HAVE_CONFIG_H
#   include "config.h"
#endif

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <portals/portals3.h>
#include <portals/nal.h>
#include <mpi.h>

#define FORK_BEFORE_NI_INIT
#ifndef FORK_BEFORE_NI_INIT
#define FORK_AFTER_NI_INIT
#endif

int
main(int argc, char **argv, char **envp)
{
         int ret;
         int num_interfaces;
         ptl_handle_ni_t nih;
         ptl_handle_eq_t eqh;
         ptl_ni_limits_t ptl_limits;
         pid_t child;

         if ((ret=PtlInit(&num_interfaces)) != PTL_OK) {
                 printf("%s: PtlInit failed: %d\n", __FUNCTION__, ret);
                 exit(1);
         }
         printf("%s: PtlInit succeeds (%d)\n", __FUNCTION__, ret);

#ifdef FORK_BEFORE_NI_INIT
         child = fork();
#endif
         if ((ret=PtlNIInit(
                 IFACE_FROM_BRIDGE_AND_NALID(PTL_BRIDGE_UK, PTL_IFACE_SS),
                 PTL_PID_ANY, NULL, &ptl_limits, &nih)) != PTL_OK) {
                 printf("%s: PtlNIInit failed: %d\n", __FUNCTION__, ret);
                 exit(1);
         }
         printf("%s: PtlNIInit succeeds (%d)\n", __FUNCTION__, ret);

#ifdef FORK_AFTER_NI_INIT
         child = fork();
#endif
         if ((ret=PtlEQAlloc(nih, 4096, NULL, &eqh)) != PTL_OK) {
                 printf("%s: PtlEQAlloc failed: %d(%d)\n",
                         __FUNCTION__, ret, child);
                 exit(1);
         }
         printf("%s: PtlEQAlloc succeeds (%d:%d)\n", __FUNCTION__, child, ret);
         if(child){
                 MPI_Init(&argc,&argv);
                 MPI_Finalize();
         }
}

