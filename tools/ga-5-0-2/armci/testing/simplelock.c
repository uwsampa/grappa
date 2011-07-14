#if HAVE_CONFIG_H
#   include "config.h"
#endif

#if HAVE_STDIO_H
#   include <stdio.h>
#endif
#if HAVE_STDLIB_H
#   include <stdlib.h>
#endif
#if HAVE_ASSERT_H
#   include <assert.h>
#endif
#if HAVE_UNISTD_H
#   include <unistd.h>
#endif

#include "mp3.h"
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
#if FIXME_THESE_ARE_NOT_DEFINED_FOR_PORTALS
        armcill_lock(mut,i);
        armcill_unlock(mut,i);
#endif
        MP_BARRIER();
        if(me==0){printf(".");fflush(stdout);}
        MP_BARRIER();
      }
}


int main(int argc, char* argv[])
{
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
