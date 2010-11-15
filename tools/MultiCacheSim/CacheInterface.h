#ifndef _CACHEINTERFACE_H_
#define _CACHEINTERFACE_H_
class CacheInterface{
public:
  virtual void readLine(unsigned long tid, unsigned long rdPC, unsigned long addr) = 0;
  virtual void writeLine(unsigned long tid, unsigned long rdPC, unsigned long addr) = 0;
  virtual void dumpStatsForAllCaches() = 0;
};
#endif
