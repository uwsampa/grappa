#include "MultiCacheSim.h"
#include <stdlib.h>
#include <ctime>

#define NUM_CACHES 16 

MultiCacheSim * c;

void *concurrent_accesses(void*np){  
  unsigned long tid = *((unsigned long*)(np));
  for(int i = 0; i < 100000; i++){
    unsigned long addr = 1; 
    unsigned long pc = rand() % 0xdeadbeff + 0xdeadbeef; 
    unsigned long type = rand() % 2;
    if(type == 0){
      c->readLine(tid, pc, addr);
    }else{
      c->writeLine(tid, pc, addr);
    }
  }
}


int main(int argc, char** argv){
  srand(time(NULL));
  pthread_t tasks[NUM_CACHES];

  char *pc;
  CoherenceProtocol p = PROTO_MSI;
  if((pc = getenv("MCS_PROTO"))){
    if(!strcmp(pc,"MESI")){
      p = PROTO_MESI;
    }
  }

  c = new MultiCacheSim(stdout, 32767, 8, 32, p);

  c->createNewCache();//CPU 1
  c->createNewCache();//CPU 2
  c->createNewCache();//CPU 3
  c->createNewCache();//CPU 4

  for(int i = 0; i < NUM_CACHES; i++){
    pthread_create(&(tasks[i]), NULL, concurrent_accesses, (void*)(new int(i)));
  }
  
  for(int i = 0; i < NUM_CACHES; i++){
    pthread_join(tasks[i], NULL);
  }

  c->dumpStatsForAllCaches();
}


