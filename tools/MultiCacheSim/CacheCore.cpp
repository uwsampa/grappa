/* 
   SESC: Super ESCalar simulator
   Copyright (C) 2003 University of Illinois.

   Contributed by Jose Renau
                  Basilio Fraguela
                  James Tuck
                  Smruti Sarangi
                  Luis Ceze
                  Karin Strauss

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

#define CACHECORE_CPP

#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>

#include "CacheCore.h"
//#include "SescConf.h" 
//#include "EnergyMgr.h" //NEED TO GET RID OF THIS SOMEHOW

#define k_RANDOM     "RANDOM"
#define k_LRU        "LRU"
//
// Class CacheGeneric, the combinational logic of Cache
//
template<class State, class Addr_t, bool Energy>
CacheGeneric<State, Addr_t, Energy> *CacheGeneric<State, Addr_t, Energy>::create(int32_t size, int32_t assoc, int32_t bsize, int32_t addrUnit, const char *pStr, bool skew)
{
  CacheGeneric *cache;

  if (skew) {
    I(assoc=1); // Skew cache should be direct map
    cache = new CacheDMSkew<State, Addr_t, Energy>(size, bsize, addrUnit, pStr);
  }else if (assoc==1) {
    // Direct Map cache
    cache = new CacheDM<State, Addr_t, Energy>(size, bsize, addrUnit, pStr);
  }else if(size == (assoc * bsize)) {
    // TODO: Fully assoc can use STL container for speed
    cache = new CacheAssoc<State, Addr_t, Energy>(size, assoc, bsize, addrUnit, pStr);
  }else{
    // Associative Cache
    cache = new CacheAssoc<State, Addr_t, Energy>(size, assoc, bsize, addrUnit, pStr);
  }

  I(cache);
  return cache;
}

//template<class State, class Addr_t, bool Energy>
//PowerGroup CacheGeneric<State, Addr_t, Energy>::getRightStat(const char* type)
//{
//  if(strcasecmp(type,"icache")==0)
//    return FetchPower;
//
//  if(strcasecmp(type,"itlb")==0)
//    return FetchPower;
//
//  if(strcasecmp(type,"cache")==0)
//    return MemPower;
//
//  if(strcasecmp(type,"tlb")==0)
//    return MemPower;
//
//  if(strcasecmp(type,"dir")==0)
//    return MemPower;
//
//  if(strcasecmp(type,"revLVIDTable")==0)
//    return MemPower;
//
//  if(strcasecmp(type,"nicecache")==0)
//    return MemPower;
//
//  MSG("Unknown power group for [%s], add it to CacheCore", type);
//  I(0);
//
//  // default case
//  return MemPower;
//}

template<class State, class Addr_t, bool Energy>
void CacheGeneric<State, Addr_t, Energy>::createStats(const char *section, const char *name)
{
  // get the type
  //bool typeExists = SescConf->checkCharPtr(section, "deviceType");
  //const char *type = 0;
  //if (typeExists)
    //type = SescConf->getCharPtr(section, "deviceType");

  //int32_t procId = 0;
  //if ( name[0] == 'P' && name[1] == '(' ) {
    // This structure is assigned to an specific processor
    //const char *number = &name[2];
    //procId = atoi(number);
  //}

}

template<class State, class Addr_t, bool Energy>
CacheGeneric<State, Addr_t, Energy> *CacheGeneric<State, Addr_t, Energy>::create(const char *section, const char *append, const char *format, ...)
{
  /*We never use this constructor in MultiCacheSim*/
  CacheGeneric *cache=0;
  char size[STR_BUF_SIZE];
  char bsize[STR_BUF_SIZE];
  char addrUnit[STR_BUF_SIZE];
  char assoc[STR_BUF_SIZE];
  char repl[STR_BUF_SIZE];
  char skew[STR_BUF_SIZE];

  snprintf(size ,STR_BUF_SIZE,"%sSize" ,append);
  snprintf(bsize,STR_BUF_SIZE,"%sBsize",append);
  snprintf(addrUnit,STR_BUF_SIZE,"%sAddrUnit",append);
  snprintf(assoc,STR_BUF_SIZE,"%sAssoc",append);
  snprintf(repl ,STR_BUF_SIZE,"%sReplPolicy",append);
  snprintf(skew ,STR_BUF_SIZE,"%sSkew",append);

  int32_t s = CSIZE;
  int32_t a = CASSOC;
  int32_t b = CBLKSZ;
  bool sk = false;
  int32_t u;
    u = 1;
  const char *pStr = CREPLPOLICY;
    cache = create(s, a, b, u, pStr, sk);

  char cacheName[STR_BUF_SIZE];
  {
    va_list ap;
    
    va_start(ap, format);
    vsprintf(cacheName, format, ap);
    va_end(ap);
  }

  cache->createStats(section, cacheName);

  return cache;
}

/*********************************************************
 *  CacheAssoc
 *********************************************************/

template<class State, class Addr_t, bool Energy>
CacheAssoc<State, Addr_t, Energy>::CacheAssoc(int32_t size, int32_t assoc, int32_t blksize, int32_t addrUnit, const char *pStr) 
  : CacheGeneric<State, Addr_t, Energy>(size, assoc, blksize, addrUnit) 
{
  I(numLines>0);
  
  if (strcasecmp(pStr, k_RANDOM) == 0) 
    policy = RANDOM;
  else if (strcasecmp(pStr, k_LRU)    == 0) 
    policy = LRU;
  else {
    MSG("Invalid cache policy [%s]",pStr);
    exit(0);
  }

  mem     = new Line [numLines + 1];
  content = new Line* [numLines + 1];

  for(uint32_t i = 0; i < numLines; i++) {
    mem[i].initialize(this);
    mem[i].invalidate();
    content[i] = &mem[i];
  }
  
  irand = 0;
}


template<class State, class Addr_t, bool Energy>
typename CacheAssoc<State, Addr_t, Energy>::Line *CacheAssoc<State, Addr_t, Energy>::findLinePrivate(Addr_t addr)
{
  Addr_t tag = calcTag(addr);

  GI(Energy, goodInterface); // If modeling energy. Do not use this
                             // interface directly. use readLine and
                             // writeLine instead. If it is called
                             // inside debugging only use
                             // findLineDebug instead

  Line **theSet = &content[calcIndex4Tag(tag)];

  // Check most typical case
  if ((*theSet)->getTag() == tag) {
    //this assertion is not true for SMP; it is valid to return invalid line
#if !defined(SESC_SMP) && !defined(SESC_CRIT)
    I((*theSet)->isValid());  
#endif
    return *theSet;
  }

  Line **lineHit=0;
  Line **setEnd = theSet + assoc;

  // For sure that position 0 is not (short-cut)
  {
    Line **l = theSet + 1;
    while(l < setEnd) {
      if ((*l)->getTag() == tag) {
        lineHit = l;
        break;
      }
      l++;
    }
  }

  if (lineHit == 0)
    return 0;

  I((*lineHit)->isValid());

  // No matter what is the policy, move lineHit to the *theSet. This
  // increases locality
  Line *tmp = *lineHit;
  {
    Line **l = lineHit;
    while(l > theSet) {
      Line **prev = l - 1;
      *l = *prev;;
      l = prev;
    }
    *theSet = tmp;
  }

  return tmp;
}

template<class State, class Addr_t, bool Energy>
typename CacheAssoc<State, Addr_t, Energy>::Line 
*CacheAssoc<State, Addr_t, Energy>::findInvalidLine2Replace(Addr_t addr, bool ignoreLocked)
{ 
  Addr_t tag    = calcTag(addr);
  Line **theSet = &content[calcIndex4Tag(tag)];

  // Check most typical case
  if ((*theSet)->getTag() == tag ) {
    //GI(tag,(*theSet)->isValid());
    return *theSet;
  }

  Line **lineHit=0;
  Line **lineFree=0; // Order of preference, invalid, locked
  Line **setEnd = theSet + assoc;
  
  // Start in reverse order so that get the youngest invalid possible,
  // and the oldest isLocked possible (lineFree)
  {
    Line **l = setEnd -1;
    while(l >= theSet) {
      if ((*l)->getTag() == tag && !(*l)->isValid()) {
        lineHit = l;
        break;
      }
      if (!(*l)->isValid())
        lineFree = l;
      else if (lineFree == 0 && !(*l)->isLocked())
        lineFree = l;

      // If line is invalid, isLocked must be false
      GI(!(*l)->isValid(), !(*l)->isLocked()); 
      l--;
    }
  }
  GI(lineFree, !(*lineFree)->isValid() || !(*lineFree)->isLocked());

  if (lineHit)
    return *lineHit;
  
  I(lineHit==0);

  if(lineFree == 0 && !ignoreLocked)
    return 0;

  if (lineFree == 0) {
    I(ignoreLocked);
    if (policy == RANDOM) {
      lineFree = &theSet[irand];
      irand = (irand + 1) & maskAssoc;
    }else{
      I(policy == LRU);
      // Get the oldest line possible
      lineFree = setEnd-1;
    }
  }else if(ignoreLocked) {
    if (policy == RANDOM && (*lineFree)->isValid()) {
      lineFree = &theSet[irand];
      irand = (irand + 1) & maskAssoc;
    }else{
      //      I(policy == LRU);
      // Do nothing. lineFree is the oldest
    }
  }

  I(lineFree);
  GI(!ignoreLocked, !(*lineFree)->isValid() || !(*lineFree)->isLocked());

  if (lineFree == theSet)
    return *lineFree; // Hit in the first possition

  // No matter what is the policy, move lineHit to the *theSet. This
  // increases locality
  Line *tmp = *lineFree;
  {
    Line **l = lineFree;
    while(l > theSet) {
      Line **prev = l - 1;
      *l = *prev;;
      l = prev;
    }
    *theSet = tmp;
  }

  return tmp;
}


template<class State, class Addr_t, bool Energy>
typename CacheAssoc<State, Addr_t, Energy>::Line 
*CacheAssoc<State, Addr_t, Energy>::findLine2Replace(Addr_t addr, bool ignoreLocked)
{ 
  Addr_t tag    = calcTag(addr);
  Line **theSet = &content[calcIndex4Tag(tag)];

  // Check most typical case
  if ((*theSet)->getTag() == tag) {
    GI(tag,(*theSet)->isValid());
    return *theSet;
  }

  Line **lineHit=0;
  Line **lineFree=0; // Order of preference, invalid, locked
  Line **setEnd = theSet + assoc;
  
  // Start in reverse order so that get the youngest invalid possible,
  // and the oldest isLocked possible (lineFree)
  {
    Line **l = setEnd -1;
    while(l >= theSet) {
      if ((*l)->getTag() == tag) {
        lineHit = l;
        break;
      }
      if (!(*l)->isValid())
        lineFree = l;
      else if (lineFree == 0 && !(*l)->isLocked())
        lineFree = l;

      // If line is invalid, isLocked must be false
      GI(!(*l)->isValid(), !(*l)->isLocked()); 
      l--;
    }
  }
  GI(lineFree, !(*lineFree)->isValid() || !(*lineFree)->isLocked());

  if (lineHit)
    return *lineHit;
  
  I(lineHit==0);

  if(lineFree == 0 && !ignoreLocked)
    return 0;

  if (lineFree == 0) {
    I(ignoreLocked);
    if (policy == RANDOM) {
      lineFree = &theSet[irand];
      irand = (irand + 1) & maskAssoc;
    }else{
      I(policy == LRU);
      // Get the oldest line possible
      lineFree = setEnd-1;
    }
  }else if(ignoreLocked) {
    if (policy == RANDOM && (*lineFree)->isValid()) {
      lineFree = &theSet[irand];
      irand = (irand + 1) & maskAssoc;
    }else{
      //      I(policy == LRU);
      // Do nothing. lineFree is the oldest
    }
  }

  I(lineFree);
  GI(!ignoreLocked, !(*lineFree)->isValid() || !(*lineFree)->isLocked());

  if (lineFree == theSet)
    return *lineFree; // Hit in the first possition

  // No matter what is the policy, move lineHit to the *theSet. This
  // increases locality
  Line *tmp = *lineFree;
  {
    Line **l = lineFree;
    while(l > theSet) {
      Line **prev = l - 1;
      *l = *prev;;
      l = prev;
    }
    *theSet = tmp;
  }

  return tmp;
}

/*********************************************************
 *  CacheDM
 *********************************************************/

template<class State, class Addr_t, bool Energy>
CacheDM<State, Addr_t, Energy>::CacheDM(int32_t size, int32_t blksize, int32_t addrUnit, const char *pStr) 
  : CacheGeneric<State, Addr_t, Energy>(size, 1, blksize, addrUnit) 
{
  I(numLines>0);
  
  mem     = new Line[numLines + 1];
  content = new Line* [numLines + 1];

  for(uint32_t i = 0; i < numLines; i++) {
    mem[i].initialize(this);
    mem[i].invalidate();
    content[i] = &mem[i];
  }
}

template<class State, class Addr_t, bool Energy>
typename CacheDM<State, Addr_t, Energy>::Line *CacheDM<State, Addr_t, Energy>::findLinePrivate(Addr_t addr)
{
  Addr_t tag = calcTag(addr);

  GI(Energy, goodInterface); // If modeling energy. Do not use this
                             // interface directly. use readLine and
                             // writeLine instead. If it is called
                             // inside debugging only use
                             // findLineDebug instead

  Line *line = content[calcIndex4Tag(tag)];

  if (line->getTag() == tag) {
    I(line->isValid());
    return line;
  }

  return 0;
}

template<class State, class Addr_t, bool Energy>
typename CacheDM<State, Addr_t, Energy>::Line 
*CacheDM<State, Addr_t, Energy>::findLine2Replace(Addr_t addr, bool ignoreLocked)
{ 
  Addr_t tag = calcTag(addr);
  Line *line = content[calcIndex4Tag(tag)];

  if (ignoreLocked)
    return line;

  if (line->getTag() == tag) {
    GI(tag,line->isValid());
    return line;
  }

  if (line->isLocked())
    return 0;

  return line;
}

/*********************************************************
 *  CacheDMSkew
 *********************************************************/

template<class State, class Addr_t, bool Energy>
CacheDMSkew<State, Addr_t, Energy>::CacheDMSkew(int32_t size, int32_t blksize, int32_t addrUnit, const char *pStr) 
  : CacheGeneric<State, Addr_t, Energy>(size, 1, blksize, addrUnit) 
{
  I(numLines>0);
  
  mem     = new Line[numLines + 1];
  content = new Line* [numLines + 1];

  for(uint32_t i = 0; i < numLines; i++) {
    mem[i].initialize(this);
    mem[i].invalidate();
    content[i] = &mem[i];
  }
}

template<class State, class Addr_t, bool Energy>
typename CacheDMSkew<State, Addr_t, Energy>::Line *CacheDMSkew<State, Addr_t, Energy>::findLinePrivate(Addr_t addr)
{
  Addr_t tag = calcTag(addr);

  GI(Energy, goodInterface); // If modeling energy. Do not use this
                             // interface directly. use readLine and
                             // writeLine instead. If it is called
                             // inside debugging only use
                             // findLineDebug instead

  Line *line = content[calcIndex4Tag(tag)];

  if (line->getTag() == tag) {
    I(line->isValid());
    return line;
  }

  // BEGIN Skew cache
  Addr_t tag2 = calcTag(addr ^ (addr>>7));
  line = content[calcIndex4Tag(tag2)];

  if (line->getTag() == tag) {
    I(line->isValid());
    return line;
  }
  // END Skew cache

  return 0;
}

template<class State, class Addr_t, bool Energy>
typename CacheDMSkew<State, Addr_t, Energy>::Line 
*CacheDMSkew<State, Addr_t, Energy>::findLine2Replace(Addr_t addr, bool ignoreLocked)
{ 
  Addr_t tag = calcTag(addr);
  Line *line = content[calcIndex4Tag(tag)];

  if (ignoreLocked)
    return line;

  if (line->getTag() == tag) {
    GI(tag,line->isValid());
    return line;
  }

  if (line->isLocked())
    return 0;

  // BEGIN Skew cache
  Addr_t tag2 = calcTag(addr ^ (addr>>7));
  Line *line2 = content[calcIndex4Tag(tag2)];

  if (line2->getTag() == tag) {
    I(line2->isValid());
    return line2;
  }
  static int32_t rand_number =0;
  if (rand_number++ & 1)
    return line;
  else
    return line2;
  // END Skew cache
  
  return line;
}
