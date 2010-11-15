#ifndef __SMPCACHE_H_
#define __SMPCACHE_H_
#include "CacheCore.h"
#include <vector>

class SMPCache{

public:
  unsigned long CPUId;
  
  //The actual SESC cache object
  CacheGeneric<StateGeneric<> > *cache;

  //A vector of all the caches in the multicachesim
  std::vector<SMPCache * > *allCaches;
  
  //Stats about the events the cache saw during execution
  int numReadHits;
  int numReadMisses;

  int numReadOnInvalidMisses;
  int numReadRequestsSent;
  int numReadMissesServicedByOthers;

  int numWriteHits;
  int numWriteMisses;

  int numWriteOnSharedMisses;
  int numWriteOnInvalidMisses;
  int numInvalidatesSent;
  
  SMPCache(int cpuid, std::vector<SMPCache * > * cacheVector);

  int getCPUId();
  std::vector<SMPCache * > *getCacheVector();
  //Readline performs a read, and uses readRemoteAction to 
  //check for data in other caches
  virtual void readLine(uint32_t rdPC, uint32_t addr) = 0;

  //Writeline performs a write, and uses writeRemoteAction
  //to check for data in other caches
  virtual void writeLine(uint32_t wrPC, uint32_t addr) = 0;
 
  //Fill line touches cache state, bringing addr's block in, and setting its state to mesi_state 
  virtual void fillLine(uint32_t addr, uint32_t mesi_state) = 0;

  //Dump the stats for this cache to outFile
  virtual void dumpStatsToFile(FILE* outFile);

};
#endif
