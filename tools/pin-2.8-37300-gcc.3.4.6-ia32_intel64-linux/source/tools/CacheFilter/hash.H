/*BEGIN_LEGAL 
Intel Open Source License 

Copyright (c) 2002-2010 Intel Corporation. All rights reserved.
 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */
// hash.H
// Mark Charney   <mark.charney@intel.com>
// $Id: hash.H,v 1.106 2003/05/09 15:02:09 rscohn1 Exp $


#ifndef _HASH_H_
# define _HASH_H_

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
//#include "types_base.H"
#include <map>
#include <list>

using namespace std;

typedef enum {
    PCTL_START,
    PCTL_STOP,
    PCTL_IGNORE,
} PCTL_ENUM_T;

#define MAX_HASH_BINS (16*1024)
class PCTL_HASH_BIN_T
{
  public:
    PCTL_HASH_BIN_T(ADDRINT arg_pc=0,               //CONS
                  UINT64 arg_target_count=0,
                  bool arg_start=false,
                  UINT32 arg_pp=0)
        : pc(arg_pc),
          target_count(arg_target_count),
          count(0),
          start(arg_start),
          pp(arg_pp)
    {
        // nada
    }

    ADDRINT pc;
    UINT64 target_count;
    UINT64 count;
    bool start; // true = start , false = stop
    UINT32 pp;
};

typedef list<PCTL_HASH_BIN_T*> BIN_LIST_T;

class PC_HASH_T
{
    map<ADDRINT,BIN_LIST_T*> pc_hash_bins;
    
  public:
    PC_HASH_T() //CONS
    {
        //nada
    }
    ~PC_HASH_T()
    {
        //FIXME
    }

    inline void
    insert(ADDRINT pc,
           UINT64 target_count,
           bool start,
           UINT32 pp)
    {
        PCTL_HASH_BIN_T* p = new PCTL_HASH_BIN_T(pc,target_count,start,pp);
        if (pc_hash_bins.find(pc) == pc_hash_bins.end())
        {
            BIN_LIST_T* plist = new BIN_LIST_T;
            plist->push_front(p);
            pc_hash_bins[pc] = plist;
        }
        else
        {
            BIN_LIST_T* plist = pc_hash_bins[pc];
            plist->push_front(p);
        }
    }



    inline PCTL_ENUM_T
    check_inc(ADDRINT pc, UINT32 pp)
    {
        PCTL_ENUM_T retval = PCTL_IGNORE;
        if (pc_hash_bins.find(pc) != pc_hash_bins.end())
        {
            BIN_LIST_T* plist = pc_hash_bins[pc];
            assert(plist != 0);
            list< PCTL_HASH_BIN_T* >::iterator i = plist->begin();
            while( i != plist->end() )
            {
                PCTL_HASH_BIN_T* p = *i;
                assert(p!= 0);
                PCTL_ENUM_T x = sub_inc(p,pp);
                i++;
                if (x != PCTL_IGNORE)
                {
                    assert(retval == PCTL_IGNORE);
                    retval = x;
                }
            }
        }
        return retval;
    }
        
    

    inline UINT64
    get_count (ADDRINT pc, UINT32 pp) 
    {
        if (pc_hash_bins.find(pc) != pc_hash_bins.end())
        {
            BIN_LIST_T* plist = pc_hash_bins[pc];
            assert(plist != 0);
            list< PCTL_HASH_BIN_T* >::iterator i = plist->begin();
            while( i != plist->end() )
            {
                PCTL_HASH_BIN_T* p = *i;
                assert(p != 0);
                if (p->pp == pp)
                {
                    return p->count;
                }
                i++;
            }
        }
        return 0;
    }


    inline bool
    check_start_stop(ADDRINT pc) const
    {
        return pc_hash_bins.find(pc) != pc_hash_bins.end();
    }

  private:

    inline PCTL_ENUM_T
    sub_inc(PCTL_HASH_BIN_T* p, UINT32 pp)
    {
        p->count++;
#if 0
        if (p->pc == 0x40000000000b69b2)
        {
            if (p->count >= 802000 && p->count <= 802010)
            {  
                cerr << "x[b69b2]= " << p->count
                     << " targ= " << p->target_count
                     << " pp= " << p->pp 
                     << " start= " << p->start 
                     << endl;
            }
        }
#endif
        if (p->count == p->target_count && p->pp == pp)
        {
            if (p->start)
            {
#if 0
                cerr << "START " << pp
                     << " at " << hex << p->pc << dec
                     << " tgt = " << p->target_count
                     << " count= " << p->count
                     << endl;
#endif
                return PCTL_START;
            }
#if 0
            cerr << "STOP " << pp
                 << " at " << hex << p->pc << dec
                 << " tgt = " << p->target_count
                 << " count= " << p->count
                 << endl;
#endif
            return PCTL_STOP;
        }
        return PCTL_IGNORE;
    }
};

#endif
