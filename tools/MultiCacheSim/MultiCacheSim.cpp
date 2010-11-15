#include "MultiCacheSim.h"
MultiCacheSim::MultiCacheSim(FILE *cachestats, int size, int assoc, int bsize){
    proto = PROTO_MSI;
    CacheStats = cachestats;
    num_caches = 0;
    cache_size = size;
    cache_assoc = assoc;
    cache_bsize = bsize; 
    #ifndef PIN
    pthread_mutex_init(&allCachesLock, NULL);
    #else
    InitLock(&allCachesLock);
    #endif
}

MultiCacheSim::MultiCacheSim(FILE *cachestats, int size, int assoc, int bsize, CoherenceProtocol p){
  proto = p;
  CacheStats = cachestats;
  num_caches = 0;
  cache_size = size;
  cache_assoc = assoc;
  cache_bsize = bsize; 
  #ifndef PIN
  pthread_mutex_init(&allCachesLock, NULL);
  #else
  InitLock(&allCachesLock);
  #endif
}

SMPCache *MultiCacheSim::findCacheByCPUId(int CPUid){
    std::vector<SMPCache *>::iterator cacheIter = allCaches.begin();
    std::vector<SMPCache *>::iterator cacheEndIter = allCaches.end();
    for(; cacheIter != cacheEndIter; cacheIter++){
      if((*cacheIter)->CPUId == CPUid){
        return (*cacheIter);
      }
    }
    return NULL;
} 
  
void MultiCacheSim::dumpStatsForAllCaches(){
    std::vector<SMPCache *>::iterator cacheIter = allCaches.begin();
    std::vector<SMPCache *>::iterator cacheEndIter = allCaches.end();
    for(; cacheIter != cacheEndIter; cacheIter++){
      (*cacheIter)->dumpStatsToFile(CacheStats);
    }
}

void MultiCacheSim::createNewCache(){
    #ifndef PIN
    pthread_mutex_lock(&allCachesLock);
    #else
    GetLock(&allCachesLock,1); 
    #endif

    SMPCache * newcache;

    if(proto == PROTO_MESI){ 
      newcache = new MESI_SMPCache(num_caches++, &allCaches, cache_size, cache_assoc, cache_bsize, 1, "LRU", false);
    }else if(proto == PROTO_MSI){ 
      newcache = new MSI_SMPCache(num_caches++, &allCaches, cache_size, cache_assoc, cache_bsize, 1, "LRU", false);
    }else{
      newcache = new MSI_SMPCache(num_caches++, &allCaches, cache_size, cache_assoc, cache_bsize, 1, "LRU", false);
    }
    allCaches.push_back(newcache);

    #ifndef PIN
    pthread_mutex_unlock(&allCachesLock);
    #else
    ReleaseLock(&allCachesLock); 
    #endif
}

void MultiCacheSim::readLine(unsigned long tid, unsigned long rdPC, unsigned long addr){
    #ifndef PIN
    pthread_mutex_lock(&allCachesLock);
    #else
    GetLock(&allCachesLock,1); 
    #endif
    SMPCache * cacheToRead = findCacheByCPUId(tidToCPUId(tid));
    if(!cacheToRead){
      return;
    }
    cacheToRead->readLine(rdPC,addr);
    #ifndef PIN
    pthread_mutex_unlock(&allCachesLock);
    #else
    ReleaseLock(&allCachesLock); 
    #endif
    return;
}
  
void MultiCacheSim::writeLine(unsigned long tid, unsigned long wrPC, unsigned long addr){
    #ifndef PIN
    pthread_mutex_lock(&allCachesLock);
    #else
    GetLock(&allCachesLock,1); 
    #endif
    SMPCache * cacheToWrite = findCacheByCPUId(tidToCPUId(tid));
    if(!cacheToWrite){
      return;
    }
    cacheToWrite->writeLine(wrPC,addr);
    #ifndef PIN
    pthread_mutex_unlock(&allCachesLock);
    #else
    ReleaseLock(&allCachesLock); 
    #endif
    return;
}


int MultiCacheSim::tidToCPUId(int tid){
    //simple for now, perhaps we want to be fancier
    return tid % num_caches; 
}
  
MultiCacheSim::~MultiCacheSim(){
    std::vector<SMPCache *>::iterator cacheIter = allCaches.begin();
    std::vector<SMPCache *>::iterator cacheEndIter = allCaches.end();
    for(; cacheIter != cacheEndIter; cacheIter++){
      delete (*cacheIter);
    }
}
