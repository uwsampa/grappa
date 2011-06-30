/* $Id:  */
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <fstream>
using namespace std;

#if HAVE_UNISTD_H
#   include <unistd.h>
#elif HAVE_WINDOWS_H
#   include <windows.h>
#   define sleep(x) Sleep(1000*(x))
#endif

#include "mp3.h"
#include "armci.h"
#define ARMCI_ENABLE_GPC_CALLS
#include "gpc.h"

#include "Hash_common.h"
#include "DistHashmap.h"
int me, nproc;


void test_distHashmap(istream &infile)
{
  string str;

  // create a distributed hashmap
  if(me==0) { printf("Creating a distributed hashmap\n"); fflush(stdout);}
  DistHashmap *dist_hashmap = new DistHashmap();
  dist_hashmap->create();
  if(me==0) { printf("Distributed hashmap created. O.K.\n"); fflush(stdout);}

  // reads a word from the file and inserts it into the hashmap
  while(!infile.eof()) {
    infile >> str;
    dist_hashmap->insert(str);
    // if(me==0) { printf("%s\n", str.c_str()); fflush(stdout);}
  }
  dist_hashmap->commit();
  
  dist_hashmap->print();  fflush(stdout);
  MP_BARRIER();
  
  dist_hashmap->print2();  fflush(stdout);
  MP_BARRIER();

  // delete the distributed hashmap
  dist_hashmap->destroy();
  if(me==0) { printf("Distributed hashmap deleted. O.K.\n"); fflush(stdout);}
}


int main(int argc, char* argv[])
{
    MP_INIT(argc, argv);
    MP_PROCS(&nproc);
    MP_MYID(&me);
    ifstream infile;
    
    if(me==0){
      printf("ARMCI Distributed Hashmap test program (%d processes)\n",nproc); 
      fflush(stdout);
      sleep(1);
    }
    
    if(me==0){
      printf("\nDistributed Hashmap using ARMCI's GPC calls\n");
      fflush(stdout);
    }

    if (argc == 1) {
        // look for file "sample.txt" in current directory
        infile.open("sample.txt");
        if (!infile) {
            if (0 == me) {
                printf("Could not open 'sample.txt'\n");
            }
            MP_FINALIZE();
        }
    } else if (argc == 2) {
        // attempt to open command-line argument
        infile.open(argv[1]);
        if (!infile) {
            if (0 == me) {
                printf("Could not open '%s'\n", argv[1]);
            }
            MP_FINALIZE();
        }
    }
    
    ARMCI_Init();
    
    MP_BARRIER();
    
    test_distHashmap(infile);
  
    infile.close();

    ARMCI_AllFence();
    MP_BARRIER();
    
    
    ARMCI_Finalize();
    MP_FINALIZE();
    return(0);
}
