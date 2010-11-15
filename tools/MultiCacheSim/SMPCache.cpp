#include "SMPCache.h"

SMPCache::SMPCache(int cpuid, std::vector<SMPCache * > * cacheVector){

  CPUId = cpuid;
  allCaches = cacheVector;

  numReadHits = 0;
  numReadMisses = 0;
  numReadOnInvalidMisses = 0;
  numReadRequestsSent = 0;
  numReadMissesServicedByOthers = 0;

  numWriteHits = 0;
  numWriteMisses = 0;
  numWriteOnSharedMisses = 0;
  numWriteOnInvalidMisses = 0;
  numInvalidatesSent = 0;

}

void SMPCache::dumpStatsToFile(FILE* outFile){
  fprintf(outFile, "-----Cache %d-----\n",CPUId);

  fprintf(outFile, "Read Hits:                   %d\n",numReadHits);
  fprintf(outFile, "Read Misses:                 %d\n",numReadMisses);
  fprintf(outFile, "Read-On-Invalid Misses:      %d\n",numReadOnInvalidMisses);
  fprintf(outFile, "Read Requests Sent:          %d\n",numReadRequestsSent);
  fprintf(outFile, "Rd Misses Serviced Remotely: %d\n",numReadMissesServicedByOthers);

  fprintf(outFile, "Write Hits:                  %d\n",numWriteHits);
  fprintf(outFile, "Write Misses:                %d\n",numWriteMisses);
  fprintf(outFile, "Write-On-Shared Misses:      %d\n",numWriteOnSharedMisses);
  fprintf(outFile, "Write-On-Invalid Misses:     %d\n",numWriteOnInvalidMisses);
  fprintf(outFile, "Invalidates Sent:            %d\n",numInvalidatesSent);

}

int SMPCache::getCPUId(){
  return CPUId;
}

std::vector<SMPCache * > *SMPCache::getCacheVector(){
  return allCaches;
}
