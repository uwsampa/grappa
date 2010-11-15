/* 
   SESC: Super ESCalar simulator
   Copyright (C) 2003 University of Illinois.

   Contributed by Jose Renau
                  Basilio Fraguela
                  James Tuck
                  Milos Prvulovic
                  Smruti Sarangi

This file is part of SESC.

SESC is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2, or (at your option) any later version.

SESC is    distributed in the  hope that  it will  be  useful, but  WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should  have received a copy of  the GNU General  Public License along with
SESC; see the file COPYING.  If not, write to the  Free Software Foundation, 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef CACHECORE_H
#define CACHECORE_H

//#include "GEnergy.h" //NEED TO GET RID OF THIS SOMEHOW
#include "nanassert.h"
#include "Snippets.h"
//#include "GStats.h" //NEED TO GET RID OF THIS SOMEHOW
#define CBLKSZ 32
#define CASSOC 8
#define CSIZE 32767
#define CREPLPOLICY "RANDOM" 

enum    ReplacementPolicy  {LRU, RANDOM};

#ifdef SESC_ENERGY
template<class State, class Addr_t = uint32_t, bool Energy=true>
#else
  template<class State, class Addr_t = uint32_t, bool Energy=false>
#endif
  class CacheGeneric {
  private:
  static const int32_t STR_BUF_SIZE=1024;
 
  //static PowerGroup getRightStat(const char* type);

  protected:
  const uint32_t  size;
  const uint32_t  lineSize;
  const uint32_t  addrUnit; //Addressable unit: for most caches = 1 byte
  const uint32_t  assoc;
  const uint32_t  log2Assoc;
  const uint64_t  log2AddrLs;
  const uint64_t  maskAssoc;
  const uint32_t  sets;
  const uint32_t  maskSets;
  const uint32_t  numLines;

  //GStatsEnergy *rdEnergy[2]; // 0 hit, 1 miss
  //GStatsEnergy *wrEnergy[2]; // 0 hit, 1 miss

  bool goodInterface;

  public:
  class CacheLine : public State {
  public:
    // Pure virtual class defines interface
    //
    // Tag included in state. Accessed through:
    //
    // Addr_t getTag() const;
    // void setTag(Addr_t a);
    // void clearTag();
    // 
    //
    // bool isValid() const;
    // void invalidate();
    //
    // bool isLocked() const;
  };

  // findLine returns a cache line that has tag == addr, NULL otherwise
  virtual CacheLine *findLinePrivate(Addr_t addr)=0;
  protected:

  CacheGeneric(uint32_t s, uint32_t a, uint32_t b, uint32_t u)
  : size(s)
  ,lineSize(b)
  ,addrUnit(u)
  ,assoc(a)
  ,log2Assoc(log2i(a))
  ,log2AddrLs(log2i(b/u))
  ,maskAssoc(a-1)
  ,sets((s/b)/a)
  ,maskSets(sets-1)
  ,numLines(s/b)
  {
    // TODO : assoc and sets must be a power of 2
  }

  virtual ~CacheGeneric() {}

  //GStatsEnergy *getEnergy(const char *section, PowerGroup grp, const char *format, const char *name);
  void createStats(const char *section, const char *name);

  public:
  // Do not use this interface, use other create
  static CacheGeneric<State, Addr_t, Energy> *create(int32_t size, int32_t assoc, int32_t blksize, int32_t addrUnit, const char *pStr, bool skew);
  static CacheGeneric<State, Addr_t, Energy> *create(const char *section, const char *append, const char *format, ...);
  void destroy() {
    delete this;
  }

  // If there are not free lines, it would return an existing cache line unless
  // all the possible cache lines are locked State must implement isLocked API
  //
  // when locked parameter is false, it would try to remove even locked lines

  virtual CacheLine *findLine2Replace(Addr_t addr, bool ignoreLocked=false)=0;

 
  virtual CacheLine *findInvalidLine2Replace(Addr_t addr, bool ignoreLocked=false){
    return findLine2Replace(addr,ignoreLocked);
  }

  // TO DELETE if flush from Cache.cpp is cleared.  At least it should have a
  // cleaner interface so that Cache.cpp does not touch the internals.
  //
  // Access the line directly without checking TAG
  virtual CacheLine *getPLine(uint32_t l) = 0;

  //ALL USERS OF THIS CLASS PLEASE READ:
  //
  //readLine and writeLine MUST have the same functionality as findLine. The only
  //difference is that readLine and writeLine update power consumption
  //statistics. So, only use these functions when you want to model a physical
  //read or write operation.

  // Use this is for debug checks. Otherwise, a bad interface can be detected
  CacheLine *findLineDebug(Addr_t addr) {
    IS(goodInterface=true);
    CacheLine *line = findLine(addr);
    IS(goodInterface=false);
    return line;
  }

  // Use this when you need to change the line state but
  // do not want to account for energy
  CacheLine *findLineNoEffect(Addr_t addr) {
    IS(goodInterface=true);
    CacheLine *line = findLine(addr);
    IS(goodInterface=false);
    return line;
  }

  CacheLine *findLine(Addr_t addr) {
    return findLinePrivate(addr);
  }

  CacheLine *readLine(Addr_t addr) {

    IS(goodInterface=true);
    CacheLine *line = findLine(addr);
    IS(goodInterface=false);

    return line;

    //rdEnergy[line != 0 ? 0 : 1]->inc();
  }

  CacheLine *writeLine(Addr_t addr) {

    IS(goodInterface=true);
    CacheLine *line = findLine(addr);
    IS(goodInterface=false);

    if(!Energy)
      return line;
    
    //wrEnergy[line != 0 ? 0 : 1]->inc();

    return line;
  }

  CacheLine *fillLine(Addr_t addr) {
    CacheLine *l = findLine2Replace(addr);
    if (l==0)
      return 0;
    
    l->setTag(calcTag(addr));
    
    return l;
  }

  CacheLine *fillLine(Addr_t addr, Addr_t &rplcAddr, bool ignoreLocked=false) {
    CacheLine *l = findLine2Replace(addr, ignoreLocked);
    rplcAddr = 0;
    if (l==0)
      return 0;
    
    Addr_t newTag = calcTag(addr);
    if (l->isValid()) {
      Addr_t curTag = l->getTag();
      if (curTag != newTag) {
        rplcAddr = calcAddr4Tag(curTag);
      }
    }
    
    l->setTag(newTag);
    
    return l;
  }

  uint32_t  getLineSize() const   { return lineSize;    }
  uint32_t  getAssoc() const      { return assoc;       }
  uint32_t  getLog2AddrLs() const { return log2AddrLs;  }
  uint32_t  getLog2Assoc() const  { return log2Assoc;   }
  uint32_t  getMaskSets() const   { return maskSets;    }
  uint32_t  getNumLines() const   { return numLines;    }
  uint32_t  getNumSets() const    { return sets;        }

  Addr_t calcTag(Addr_t addr)       const { return (addr >> log2AddrLs);              }

  uint32_t calcSet4Tag(Addr_t tag)     const { return (tag & maskSets);                  }
  uint32_t calcSet4Addr(Addr_t addr)   const { return calcSet4Tag(calcTag(addr));        }

  uint32_t calcIndex4Set(uint32_t set)    const { return (set << log2Assoc);                }
  uint32_t calcIndex4Tag(uint32_t tag)    const { return calcIndex4Set(calcSet4Tag(tag));   }
  uint32_t calcIndex4Addr(Addr_t addr) const { return calcIndex4Set(calcSet4Addr(addr)); }

  Addr_t calcAddr4Tag(Addr_t tag)   const { return (tag << log2AddrLs);                   }
};

#ifdef SESC_ENERGY
template<class State, class Addr_t = uint32_t, bool Energy=true>
#else
template<class State, class Addr_t = uint32_t, bool Energy=false>
#endif
class CacheAssoc : public CacheGeneric<State, Addr_t, Energy> {
  using CacheGeneric<State, Addr_t, Energy>::numLines;
  using CacheGeneric<State, Addr_t, Energy>::assoc;
  using CacheGeneric<State, Addr_t, Energy>::maskAssoc;
  using CacheGeneric<State, Addr_t, Energy>::goodInterface;

private:
public:
  typedef typename CacheGeneric<State, Addr_t, Energy>::CacheLine Line;

protected:
 
  Line *mem;
  Line **content;
  ushort irand;
  ReplacementPolicy policy;

  friend class CacheGeneric<State, Addr_t, Energy>;
  CacheAssoc(int32_t size, int32_t assoc, int32_t blksize, int32_t addrUnit, const char *pStr);

  Line *findLinePrivate(Addr_t addr);
public:
  virtual ~CacheAssoc() {
    delete [] content;
    delete [] mem;
  }

  // TODO: do an iterator. not this junk!!
  Line *getPLine(uint32_t l) {
    // Lines [l..l+assoc] belong to the same set
    I(l<numLines);
    return content[l];
  }

  Line *findLine2Replace(Addr_t addr, bool ignoreLocked=false);
  Line *findInvalidLine2Replace(Addr_t addr, bool ignoreLocked=false);
};

#ifdef SESC_ENERGY
template<class State, class Addr_t = uint32_t, bool Energy=true>
#else
template<class State, class Addr_t = uint32_t, bool Energy=false>
#endif
class CacheDM : public CacheGeneric<State, Addr_t, Energy> {
  using CacheGeneric<State, Addr_t, Energy>::numLines;
  using CacheGeneric<State, Addr_t, Energy>::goodInterface;

private:
public:
  typedef typename CacheGeneric<State, Addr_t, Energy>::CacheLine Line;

protected:
  
  Line *mem;
  Line **content;

  friend class CacheGeneric<State, Addr_t, Energy>;
  CacheDM(int32_t size, int32_t blksize, int32_t addrUnit, const char *pStr);

  Line *findLinePrivate(Addr_t addr);
public:
  virtual ~CacheDM() {
    delete [] content;
    delete [] mem;
  };

  // TODO: do an iterator. not this junk!!
  Line *getPLine(uint32_t l) {
    // Lines [l..l+assoc] belong to the same set
    I(l<numLines);
    return content[l];
  }

  Line *findLine2Replace(Addr_t addr, bool ignoreLocked=false);
};

#ifdef SESC_ENERGY
template<class State, class Addr_t = uint32_t, bool Energy=true>
#else
template<class State, class Addr_t = uint32_t, bool Energy=false>
#endif
class CacheDMSkew : public CacheGeneric<State, Addr_t, Energy> {
  using CacheGeneric<State, Addr_t, Energy>::numLines;
  using CacheGeneric<State, Addr_t, Energy>::goodInterface;

private:
public:
  typedef typename CacheGeneric<State, Addr_t, Energy>::CacheLine Line;

protected:
  
  Line *mem;
  Line **content;

  friend class CacheGeneric<State, Addr_t, Energy>;
  CacheDMSkew(int32_t size, int32_t blksize, int32_t addrUnit, const char *pStr);

  Line *findLinePrivate(Addr_t addr);
public:
  virtual ~CacheDMSkew() {
    delete [] content;
    delete [] mem;
  };

  // TODO: do an iterator. not this junk!!
  Line *getPLine(uint32_t l) {
    // Lines [l..l+assoc] belong to the same set
    I(l<numLines);
    return content[l];
  }

  Line *findLine2Replace(Addr_t addr, bool ignoreLocked=false);
};


template<class Addr_t=uint32_t>
class StateGeneric {
private:
  Addr_t tag;

public:
  virtual ~StateGeneric() {
    tag = 0;
  }
 
 Addr_t getTag() const { return tag; }
 void setTag(Addr_t a) {
   I(a);
   tag = a; 
 }
 void clearTag() { tag = 0; }
 void initialize(void *c) { 
   clearTag(); 
 }

 virtual bool isValid() const { return tag; }

 virtual void invalidate() { clearTag(); }

 virtual bool isLocked() const {
   return false;
 }

 virtual void dump(const char *str) {
 }
};

#ifndef CACHECORE_CPP
#include "CacheCore.cpp"
#endif

#endif // CACHECORE_H
