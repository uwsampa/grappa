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
// caches.H
// Mark Charney   <mark.charney@intel.com>
// $Id: caches.H,v 1.2 2003/05/06 20:05:40 rscohn1 Exp $


#ifndef _CACHES_H_
# define _CACHES_H_

#include <cassert>
#include <iomanip>
#include "my-types.H"
#include "log-util.H"
#include "globals.H"
////////////////////////////////////////////////////////////////////////////
using namespace std;
class ASSOC_CACHE_T
{
    UINT    capacity;
    UINT    assoc;
    UINT    linesz;
    UINT    rows;
    UINT    congruence_classes;
    UINT    cc_mask;
    UINT    line_shift;
    UINT    cc_shift;
    UINT    tag_shift;
    UINT64* tags;
    UINT64* lru;
    UINT64  hits;
    UINT64  misses;
public:
    ASSOC_CACHE_T(UINT arg_capacity,
                  UINT arg_assoc,
                  UINT arg_linesz)  
        : capacity(arg_capacity),
          assoc(arg_assoc),
          linesz(arg_linesz),
          hits(0),
          misses(0)
    {
        assert(is_power_of_2(linesz));
        assert(is_power_of_2(capacity));
        rows = capacity / linesz;
        congruence_classes = rows / assoc;
        line_shift = ilog(linesz);
        cc_mask = congruence_classes - 1;
        cc_shift = ilog(congruence_classes);
        tag_shift = cc_shift + line_shift;
        tags = new UINT64[rows];
        lru = new UINT64[rows];
        my_alloc += 2*sizeof(UINT64)*rows;
        wipe_directory();
    }
    ~ASSOC_CACHE_T()
    {
        delete [] tags;
        delete [] lru;
    }
    void wipe_directory(void)
    {
        for(UINT i=0;i<congruence_classes;i++)
        {
            for(UINT j=0;j<assoc;j++)
            {
                UINT idx = i*assoc+j;
                tags[idx] = 0;
                lru[idx] = j;
            }
        }
    }

    void reset(bool flush) {
        if (flush) {
            wipe_directory();
        }
        hits = 0;
        misses = 0;
    }

    UINT64 get_tag(UINT64 ea) const {
        UINT64 tag = (ea >> tag_shift);
        return tag;
    }
    UINT get_cc(UINT64 ea) const {
        UINT cc = (ea >> line_shift) & cc_mask;
        return cc;
    }

    void report(ofstream* log, UINT i, UINT64 insts, char* prefix) {
        char* s = " multi_cache_";
        *log << prefix << s << i << "  Capacity = " <<  capacity << endl;
        *log << prefix << s << i << "  Assoc  = " <<  assoc << endl;
        *log << prefix << s << i << "  Linesz = " <<  linesz<< endl;
        *log << prefix << s << i << "  Congruence_classes = "
             <<  congruence_classes << endl;
        *log << prefix << s << i << "  Rows = " <<  rows << endl;
        *log << prefix << s << i << "  Hits = " <<  hits << endl;
        *log << prefix << s << i << "  Misses = " <<  misses << endl;
        *log << prefix << s << i << "  Refs = " <<  hits+misses << endl;
        *log << prefix << s << i << "  Misses_per_inst = "
             << setprecision(4)
             <<  1.0*misses/insts << endl;
    }


    void ref(UINT64 ip, UINT64 ea, REF_CODE_ENUM type)
    {
        //FIXME: ignoring type
        const UINT cc = get_cc(ea);
        const UINT64 tag = get_tag(ea);
        const UINT base_idx =  cc*assoc;
        UINT64 last_lru = lru[base_idx];
        UINT lru_idx = base_idx;
        for(UINT a=0;a<assoc;a++) {
            const UINT idx = base_idx+a;
            assert(idx < rows);
            if (tags[idx] == tag) {
                hits++;
                lru[idx] = icount.Count();
                return;
            }
            else if (lru[idx] < last_lru) {
                last_lru = lru[idx];
                lru_idx = idx;
            }
        }
        // missed... find LRU
        tags[lru_idx] = tag;
        lru[lru_idx] = icount.Count();
        misses++;
    }

};

/**********************************************************************/

class MULTI_CACHE_T
{
    UINT ncaches;
    ASSOC_CACHE_T** cache;
  public:
    MULTI_CACHE_T(UINT arg_ncaches) // CONS
        : ncaches(arg_ncaches)
    {
        cache = new ASSOC_CACHE_T*[ncaches];
        my_alloc += sizeof(ASSOC_CACHE_T*)*ncaches;
        for(UINT i=0;i<ncaches;i++) {
            cache[i] = 0;
        }
    }
    ~MULTI_CACHE_T()
    {
        for(UINT i=0;i<ncaches;i++) {
            if (cache[i]) {
                delete cache[i];
            }
        }
        delete [] cache;
    }

    void init(UINT i, UINT capacity, UINT assoc, UINT linesz)
    {
        assert(i < ncaches);
        assert(cache[i] == 0);
        cache[i] = new ASSOC_CACHE_T(capacity,assoc,linesz);
        my_alloc += sizeof(ASSOC_CACHE_T);
    }

    void report(ofstream* log, UINT64 icount, char* prefix)
    {
        for(UINT i=0;i<ncaches;i++)
        {
            cache[i]->report(log, i, icount, prefix);
        }
    }

    void reset(bool flush)
    {
        for(UINT i=0;i<ncaches;i++)
        {
            cache[i]->reset(flush);
        }
    }


    void ref(UINT64 ip, UINT64 ea, REF_CODE_ENUM type)
    {
        for(UINT i=0;i<ncaches;i++)
        {
            cache[i]->ref(ip,ea,type);
        }
    }
};

/**********************************************************************/

class DIRMAP_CACHE_T
{
    UINT    capacity;
    UINT    linesz;
    UINT    rows;
    UINT    row_mask;
    UINT    line_shift;
    UINT64* tags;
    UINT64  hits;
    UINT64  misses;
  public:
    DIRMAP_CACHE_T(UINT arg_capacity, UINT arg_linesz) // CONS
        : capacity(arg_capacity), linesz(arg_linesz),
          hits(0), misses(0)
    {
        assert( linesz <= capacity);
/*
        UINT x = 1<<ilog(linesz);
        cerr << " ilog(linesz) " << ilog(linesz) << endl;
        cerr << " linesz " << linesz << endl;
        cerr << " 1<< ilog(linesz)" << x << endl;
*/
        assert(is_power_of_2(linesz));
        assert(is_power_of_2(capacity));
        rows = capacity / linesz;
        line_shift = ilog(linesz);
        row_mask = rows - 1;
        tags = new UINT64[rows];
        my_alloc += sizeof(UINT64)*rows;
        wipe_directory();
    }
    ~DIRMAP_CACHE_T()
    {
        delete [] tags;
    }
    
    void reset(bool flush) {
        if (flush) {
            wipe_directory();
        }
        hits = 0;
        misses = 0;
    }

    void wipe_directory(void)
    {
        for(UINT i=0;i<rows;i++)
        {
            tags[i] = 0;
        }
    }

    UINT64 get_tag(UINT64 ea) const {
        /* I'm leaving the row bits in the tag -- it doesn't matter */
        UINT tag = (ea >> line_shift);
        return tag;
    }
    UINT get_row(UINT64 ea) const {
        UINT row = (ea >> line_shift) & row_mask;
        return row;
    }

    void report(ofstream* log, UINT i, UINT64 insts, char* prefix) {
        char* s = " filter_cache_";
        *log << prefix << s << i << "  Capacity = " <<  capacity << endl;
        *log << prefix << s << i << "  Linesz = " <<  linesz<< endl;
        *log << prefix << s << i << "  Rows = " <<  rows << endl;
        *log << prefix << s << i << "  Hits = " <<  hits << endl;
        *log << prefix << s << i << "  Misses = " <<  misses << endl;
        *log << prefix << s << i << "  Refs = " <<  hits+misses << endl;
        *log << prefix << s << i << "  Misses_per_inst = "
             << setprecision(4)
             <<  1.0*misses/insts << endl;
    }

    bool ref(UINT64 ip, UINT64 ea, REF_CODE_ENUM t)
    {
        UINT row = get_row(ea);
        UINT64 tag = get_tag(ea);
        if (tags[row] == tag)
        {
            hits++;
            return false;
        }
        misses++;
        tags[row] = tag;
        return true;
    }

};

/**********************************************************************/




class FILTER_CACHES_T
{
    UINT             ncaches;
    DIRMAP_CACHE_T** cache;
    UINT64           misses[LAST_REF_CODE];
    UINT64           refs[LAST_REF_CODE];
    int              fd;
#define NRECS 1024
    IPEA_T           ipea[NRECS]; // buffer for optional trace
    UINT             p;

    void trace_write(UINT64 ip, UINT64 ea)
    {
        ipea[p].ip   = ip;
        ipea[p++].ea = ea;
        if (p == NRECS) {
            trace_dump();
        }
    }
    void trace_dump(void)
    {
        write(fd, &ipea, p*sizeof(IPEA_T));
        p = 0;
    }
  public:
    FILTER_CACHES_T(UINT n,
                    const char* file) // CONS
        : ncaches(n),
          fd(0), p(0)
    {
        if (file) {
            fd = open(file,O_CREAT|O_WRONLY|O_TRUNC, S_IWRITE|S_IREAD);
            if (fd == -1) {
                cerr << "Could not open file " << file << endl;
                exit(1);
            }
        }
        for(UINT i=0;i<LAST_REF_CODE;i++) {
            refs[i] = 0;
            misses[i] = 0;
        }
        cache = new DIRMAP_CACHE_T*[ncaches];
        my_alloc += sizeof(DIRMAP_CACHE_T*)*ncaches;
        for(UINT i=0;i<ncaches;i++) {
            cache[i] = 0;
        }
    }
    ~FILTER_CACHES_T()
    {
        for(UINT i=0;i<ncaches;i++) {
            if (cache[i]) {
                delete cache[i];
            }
        }
        delete [] cache;
    }
    void report(ofstream* log, UINT64 insts, char* prefix)
    {
        if (p) {
            trace_dump();
        }
        if (fd) {
            close(fd);
        }

        for(UINT i=0;i<ncaches;i++)
        {
            cache[i]->report(log, i,insts, prefix);
        }

        UINT64 all_data_misses = 0;
        for(UINT i=0;i<LAST_REF_CODE;i++) {
            if (i != IFETCH_CODE)
            {
                all_data_misses += misses[i];
            }
        }


        if (all_data_misses)
        {
            *log << prefix << " Loads = " << refs[LOAD_CODE]<< endl;
            *log << prefix << " Stores = " << refs[STORE_CODE] << endl;
            *log << prefix << " Atomics = " << refs[ATOMIC_CODE] << endl;
            *log << prefix << " Prefetches = " << refs[PREFETCH_CODE] << endl;
            
            *log << prefix << " Code_misses = " << misses[IFETCH_CODE] << endl;
            *log << prefix << " Load_misses = " << misses[LOAD_CODE] << endl;
            *log << prefix << " Store_misses = " << misses[STORE_CODE] << endl;
            *log << prefix << " Atomic_misses = " << misses[ATOMIC_CODE]
                 << endl;
            *log << prefix << " Prefetch_misses = " << misses[PREFETCH_CODE]
                 << endl;
            
            *log << prefix << " Data_Misses = " << all_data_misses << endl;
            *log << prefix << " Data_Misses_per_inst = "
                 << setprecision(4)
                 << 1.0*all_data_misses/insts << endl;
        }

        if ( refs[IFETCH_CODE] )
        {
            *log << prefix << " Code_fetches = " << refs[IFETCH_CODE]<< endl;
            *log << prefix << " Inst_Misses = " << misses[IFETCH_CODE] << endl;
            *log << prefix << " Inst_Misses_per_inst = "
                 << setprecision(4)
                 << 1.0*misses[IFETCH_CODE]/insts
                 << endl;
        }
    }

    void init(UINT i, UINT capacity, UINT linesz)
    {
        assert(i<ncaches);
        assert(cache[i] == 0);
        
        cache[i] = new DIRMAP_CACHE_T(capacity, linesz);
        my_alloc += sizeof(DIRMAP_CACHE_T);
    }

        
    void reset(bool flush)
    {
        for(UINT i=0;i<ncaches;i++)
        {
            cache[i]->reset(flush);
        }
    }

    bool ref(UINT64 ip, UINT64 ea, REF_CODE_ENUM t)
    {
        bool miss = false;
        refs[t]++;
        for(UINT i=0;i<ncaches;i++)
        {
            miss |= cache[i]->ref(ip,ea,t);
        }
        if ( miss ) {
            // emit the miss into the filtered miss stream
            if (fd) {
                UINT64 nea = (ea & ~0x3ULL) | t;
                trace_write(ip,nea);
            }
            misses[t]++;
            return true;
        }
        return false;
    }

};



////////////////////////////////////////////////////////////////////////////
#endif
