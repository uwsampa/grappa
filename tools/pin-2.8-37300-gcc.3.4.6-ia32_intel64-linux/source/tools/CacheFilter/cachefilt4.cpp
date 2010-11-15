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
/* cachefilt4.cpp: 
 * This program implements Puzak-style cache trace filtering for quick
 * multi-cache simulation. This adds instruction cache to the mix.
 *
 *  Author: Mark Charney
 *
 *  $Id: cachefilt4.cpp,v 1.4 2003/05/07 20:57:25 mjcharne Exp $
 *
 *
 *
 *
 * CLASSES:
 * 
 *    CACHE_CONFIG_T: describes cache geometry
 *
 *    ASSOC_CACHE_T
 *    MULTI_CACHE_T : collection of ASSOC_CACHE_T's -- the "back end" caches
 *
 *    IPEA_T: instruction pointers and effective addresses, accumulated by
 *            instrumentation.
 *
 *    DIRMAP_CACHE_T
 *    FILTER_CACHES_T: collection of DIRMAP_CACHE_T's -- the "front end" caches
 *
 */

#include <time.h>
#include <stdlib.h>
#include <libgen.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <bitset>
#include <cassert>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "pin.H"
#include "instlib.H"

using namespace std;
using namespace INSTLIB;

#include "my-types.H"


///////////////////////////////////////////////////////////////////////
// PROTOTYPES 
///////////////////////////////////////////////////////////////////////
static void do_filter_data(void);
static void do_filter_inst(void);
static void do_no_filter_data(void);
static void do_no_filter_inst(void);

static void interim_report(void); 

static ofstream* dump_stats_file(ostringstream& os,
                                 const char* type,
                                 UINT64 icount,
                                 time_t t1); 

static void report(ofstream* mylog,
                   UINT64 icount,
                   time_t t1);


static void fini(int exit_val, void* v);
static void reset(void);

//////////////////////////////////////////////////////////////////////////
// DEFINES
//////////////////////////////////////////////////////////////////////////

#define MAX_MEMOPS           10000
#define MIN_MEMOPS_THRESHOLD  1000

//////////////////////////////////////////////////////////////////////////
// GLOBALS (too many globals)
//////////////////////////////////////////////////////////////////////////
static const UINT million   = 1000000;
static const UINT d_million = 200*million;
static const UINT c_million = 200*million;

static ofstream* debug = 0;     // "debug"  file
static ofstream* pplog = 0;     // pin point stats file
static ofstream* mylog = 0;     // "cfilt" stats file

static char*  prog_name       = 0; // for error msgs
static pid_t  pid             = 0; // for uniqifying the output file name
UINT64 my_alloc               = 0; // count of bytes allocated on heap
static char*  fn_prefix       = "cachefilt"; // default prefix for stats files
static bool   do_trace_file            = false; 
static bool   do_instruction_cache_sim = false;
static bool   use_filtering_caches     = true;
static bool   flush_directory_at_sample_boundary = false;
static bool   run_all_refs = true;

//static SAMPLING_CONTROLLER_BASE_T*  ppc = 0;

// Contains knobs and instrumentation to recognize start/stop points
static CONTROL control;

// Track the number of instructions executed
ICOUNT icount;

#include "caches.H"


static FILTER_CACHES_T* d_filter_caches     = 0;
static FILTER_CACHES_T* i_filter_caches     = 0;
static MULTI_CACHE_T*   d_multi_cache       = 0;
static MULTI_CACHE_T*   i_multi_cache       = 0;

static UINT64           last_icount         = 0;
#ifdef INTERIM_REPORTING
static UINT64           last_icount2        = 0;
#endif
static UINT64           instrumentations    = 0;
static time_t           t1                  = 0; // program start time
static UINT64           sample_start_icount = 0;
static time_t           pp_t1               = 0; // start time of sample

static UINT imemops = 0;
static UINT memops  = 0;
static ADDRINT iaddr[MAX_MEMOPS];
static MISS_DATA_T memdata[MAX_MEMOPS];

// set defaults for the cache sizes
static CACHE_CONFIG_T last_multi_cache   = { 4096*1024, 8, 256};
static CACHE_CONFIG_T first_multi_cache  = { 8*1024, 1, 32};
static CACHE_CONFIG_T last_filter_cache  = { 4*1024, 1, 256};
static CACHE_CONFIG_T first_filter_cache = { 4*1024, 1, 32};
static ADDRINT  min_line_size_mask = 0;


UINT
ilog(UINT arg)
{
    UINT i=0;
    UINT a=arg;
    assert(a > 0);
    while(a)
    {
        i++;
        a = a >> 1;
    }
    return i-1;
}
        
bool
is_power_of_2(UINT x)
{
    return ( (1U << ilog(x))  == x );
}






//////////////////////////////////////////////////////////////////////

static void
set_min_line_size_mask(void)
{
    min_line_size_mask = ~((UINT64)(first_multi_cache.linesz  - 1));
}

///////////////////////////////////////////////////////////////////////////



static void
brief_status(ofstream* out,
             UINT64 sicount,
             bool newlines,
             time_t t1)
{
    time_t t2 = time(0);
    UINT64 seconds = t2 - t1;
    *out << "Global_icount = " << icount.Count();
    if (newlines) { *out << endl; } else { *out << " "; }
    *out << "Sample_icount = " << sicount;
    if (newlines) { *out << endl; } else { *out << " "; }
    *out << "Seconds = " << seconds;
    if (newlines) { *out << endl; } else { *out << " "; }
    *out << "Insts_per_second = " << 1.0*sicount/seconds;
    if (newlines) { *out << endl; } else { *out << " "; }
    *out << "Instrumentations = " << instrumentations << endl;
}

static void
watch_icount(void)
{
    static time_t local_t1 = 0;

    // dump an icount line every ~1 million insts
    if (icount.Count() > last_icount + d_million)
    {
        const bool newlines = false;
        brief_status(debug, icount.Count(), newlines, local_t1);
        last_icount = icount.Count();
        local_t1 = time(0);
        
#ifdef INTERIM_REPORTING
        // dump a full stats report every ~200 million insts
        if (icount > last_icount2 + c_million)
        {
            interim_report();
            last_icount2 = icount.Count();
        }
#endif
    }
}


//////////////////////////////////////////////////////////////////////

static void
reset(void)
{
    if (d_filter_caches)
    {
        d_filter_caches->reset(flush_directory_at_sample_boundary);
    }
    if (i_filter_caches)
    {
        i_filter_caches->reset(flush_directory_at_sample_boundary);
    }
    d_multi_cache->reset(flush_directory_at_sample_boundary);
    if (i_multi_cache)
    {
        i_multi_cache->reset(flush_directory_at_sample_boundary);
    }
}


static void
process_accumulated_data_records(void)
{
    /*
     * When we are finishing, we have a bunch of
     * data that we may not have processed yet.
     * This function does one more pass on the accumlated
     * data and sends it to the caches.
     */
    if (use_filtering_caches)
    {
        do_filter_data();
        do_filter_inst();
    }
    else
    {
        do_no_filter_data();
        do_no_filter_inst();
    }
    imemops = 0;
    memops  = 0;
}
    
static void
finish_pinpoint(UINT64 sample_icount, time_t pp_t1)
{
    process_accumulated_data_records();

    UINT64 c;
    if ( sample_icount == 0) {
        c = icount.Count(); // use the global icount when not sampling
    }
    else {
        c = sample_icount;
    }
    if (control.PinPointsActive())
    {
        UINT32 pp = control.NumPp();

        *debug << "Dumping pin-point " << pp <<  endl;
        
        *pplog << "PinPoint " << pp << endl;
        report(pplog, c, pp_t1);
    }
}

//////////////////////////////////////////////////////////////////////
VOID Handler(CONTROL_EVENT ev, VOID * v, CONTEXT * ctxt, VOID * ip, VOID * tid)
{
    std::cout << "ip: " << ip << " Instructions: "  << icount.Count() << " ";

    switch(ev)
    {
      case CONTROL_START:
        std::cout << "Start" << endl;
        if(control.PinPointsActive())
        {
            std::cout << "PinPoint: " << control.CurrentPp() << endl;
            //UINT32 pp = control.CurrentPp();
            //ASSERTX( pp <= MAXPP);
            //ppinfo.ppstats[pp].ppWeightTimesThousand = control.CurrentPpWeightTimesThousand();
            //ppinfo.currentpp = pp; 
            sample_start_icount = icount.Count();
            reset();
            pp_t1 = time(0);
        }
        break;

      case CONTROL_STOP:
        std::cout << "Stop" << endl;
        if(control.PinPointsActive())
        {
            std::cout << "PinPoint: " << control.CurrentPp() << endl;
            //UINT32 pp = ppinfo.currentpp;
            //ppinfo.ppstats[pp].ppMispredicts = mispredicts;
            //ppinfo.ppstats[pp].ppInstructions = instructions;

            /* emit stats for the last segment */
            UINT64 sample_length = icount.Count() - sample_start_icount;
            finish_pinpoint(sample_length, pp_t1);
            sample_start_icount = 0;
        }
        break;

      default:
        ASSERTX(false);
        break;
    }
}



///////////////////////////////////////////////////////////////////

static void
do_filter_data(void)
{
    // we are tracing
    for(UINT i=0;i<memops;i++)
    {
        const MISS_DATA_T* p = memdata + i;
        const ADDRINT ip = p->ip;
        const ADDRINT ea = p->ea;
        const REF_CODE_ENUM t = p->t;
        if (d_filter_caches->ref(ip,ea,t)) {
            d_multi_cache->ref(ip,ea,t);
        }
    }
}



static void
do_no_filter_data(void)
{
    // we are tracing
    for(UINT i=0;i<memops;i++)
    {
        const MISS_DATA_T* p = memdata + i;
        const ADDRINT ip = p->ip;
        const ADDRINT ea = p->ea;
        const REF_CODE_ENUM t = p->t;
        d_multi_cache->ref(ip,ea,t);
    }
}

///////////////////////////////////////////////////////////////////

static void
do_filter_inst(void)
{
    for(UINT i=0;i<imemops;i++)
    {
        ADDRINT ip = iaddr[i];
        if (i_filter_caches->ref(ip,ip,IFETCH_CODE)) {
            i_multi_cache->ref(ip,ip,IFETCH_CODE);
        }
    }
}

static void
do_no_filter_inst(void)
{
    for(UINT i=0;i<imemops;i++)
    {
        ADDRINT ip = iaddr[i];
        i_multi_cache->ref(ip,ip,IFETCH_CODE);
    }
}

////////////////////////////////////////////////////////////////////

static void
do_filter()
{
    // called from PIN instrumentation when we leave a sequence
    // bogus after-the-fact check -- better than nothing
    assert(memops < MAX_MEMOPS);
    assert(imemops < MAX_MEMOPS);

    bool do_work = run_all_refs;
    if (do_work)
    {
        if (memops > MIN_MEMOPS_THRESHOLD) 
        {    // wait until we have enough to do
            do_filter_data();
            memops = 0;
        }
        if (i_multi_cache)
        {
            if (imemops > MIN_MEMOPS_THRESHOLD)
            {  // wait until we have enough to do
                do_filter_inst();
                imemops = 0;
            }
        }
    }
    else // not tracing -- just toss data by zeroing array indices
    {
        imemops = 0;
        memops = 0;
    }
    watch_icount();
}


static void
do_no_filter()
{
    // bogus after-the-fact check -- better than nothing
    assert(memops < MAX_MEMOPS);
    assert(imemops < MAX_MEMOPS);

    bool do_work = run_all_refs;
    if (do_work)
    {
        if (memops > MIN_MEMOPS_THRESHOLD)
        {    // wait until we have enough to do
            do_no_filter_data();
            memops = 0;
        }
        if (i_multi_cache)
        {
            if (imemops > MIN_MEMOPS_THRESHOLD)
            {  // wait until we have enough to do
                do_no_filter_inst();
                imemops = 0;
            }
        }
    }
    else // not tracing -- just toss data by zeroing array indices
    {
        imemops = 0;
        memops = 0;
    }
    watch_icount();
}

///////////////////////////////////////////////////////////////////
// THIS SECTION IS FOR THE FUNCTIONS INSERTED BY PIN INTO THE
//   PROGRAM WE ARE STUDYING/TRACING/SIMULATING.
//
//     THESE ARE THE INSTRUMENTATION FUNCTIONS
//
///////////////////////////////////////////////////////////////////

static void
icache_record(ADDRINT ip)
{
    iaddr[imemops++] = ip; // FIXME:  unchecked bounds -- danger danger
}

static void
dofilter_load(UINT64 qpval, ADDRINT ip, ADDRINT ea)
{
    // qpval is the qualifying predicate value

    // if qpval is zero, we don't use the last thing
    // we store here.

    // PIN won't inline if I use a conditional, so I'm
    // doing integer math and hoping it'll use predication.

    MISS_DATA_T* const p = memdata + memops;// FIXME:  unchecked bounds 
    p->ip = ip;
    p->ea = ea;
    p->t = LOAD_CODE;
    memops = memops + (1*qpval);
}

static void
dofilter_store(UINT64 qpval, ADDRINT ip, ADDRINT ea)
{
    // see comment in dofilter_load
    MISS_DATA_T* const p = memdata + memops;// FIXME:  unchecked bounds 
    p->ip = ip;
    p->ea = ea;
    p->t = STORE_CODE;
    memops = memops + (1*qpval);
}

static void
dofilter_prefetch(UINT64 qpval, ADDRINT ip, ADDRINT ea)
{
    // see comment in dofilter_load
    MISS_DATA_T* const p = memdata + memops;// FIXME:  unchecked bounds 
    p->ip = ip;
    p->ea = ea;
    p->t = PREFETCH_CODE;
    memops = memops + (1*qpval);
}
static void
dofilter_atomic(UINT64 qpval, ADDRINT ip, ADDRINT ea)
{
    // see comment in dofilter_load
    MISS_DATA_T* const p = memdata + memops;// FIXME:  unchecked bounds 
    p->ip = ip;
    p->ea = ea;
    p->t =  ATOMIC_CODE;
    memops = memops + (1*qpval);
}

//
// END OF INSTRUMENTATION FUNCTIONS
//
///////////////////////////////////////////////////////////////////

static AFUNPTR
pick_cleanup_fn(void)
{
    if (use_filtering_caches)
    {
        return (AFUNPTR)do_filter;
    }
    return (AFUNPTR)do_no_filter;
}


class instrument_trace_info_t
{
  public:
    INS last;
    BOOL last_is_valid;
    AFUNPTR cleanup_fn;
    ADDRINT last_line_addr;
    instrument_trace_info_t()    
    {
        zero();
    }

    void zero()
    {
        last_is_valid = false;

        cleanup_fn = pick_cleanup_fn();
        last_line_addr = 0xDEADBEEF; //sentinel
    }
};

static void
InstrumentInstructions(INS ins, instrument_trace_info_t* tinfo)
{
    /*
     * This function, called by the pin infrastructure, adds the
     * instrumentation functions to the application.
     */

    ADDRINT ia = INS_Address(ins);
    
    if (i_multi_cache)
    {
        ADDRINT line_addr  = ia & min_line_size_mask;
        
        if (line_addr != tinfo->last_line_addr)
        {
            INS_InsertCall(ins,IPOINT_BEFORE,
                           (AFUNPTR) icache_record,
                           IARG_ADDRINT,
                           line_addr,
                           IARG_END);
        }
        tinfo->last_line_addr = line_addr;
    }
    
    if (INS_IsBranch(ins))
    {        // TYPE_CAT_BRANCH: TYPE_CAT_CBRANCH: TYPE_CAT_CHECK:
        // process the accumulated memops on taken branches
        instrumentations++;
        INS_InsertCall(ins,IPOINT_TAKEN_BRANCH, 
                       tinfo->cleanup_fn,
                       IARG_END);
    }
    else if (INS_IsCall(ins) || INS_IsSyscall(ins) || INS_IsRet(ins) || INS_IsInterrupt(ins))
    {
        // TYPE_CAT_JUMP TYPE_CAT_CJUMP TYPE_CAT_BREAK:
        // process the accumulated memops on these kinds of jumps
        instrumentations++;
        INS_InsertCall(ins,IPOINT_BEFORE,
                       tinfo->cleanup_fn,
                       IARG_END);
    }
    else if (INS_IsMemoryWrite(ins))
    {
        // TYPE_CAT_STORE:
        instrumentations++;
        INS_InsertCall(ins,IPOINT_BEFORE, 
                       (AFUNPTR)dofilter_store,
                       IARG_PREDICATE,
                       IARG_INST_PTR,
                       IARG_MEMORYWRITE_EA,
                       IARG_END);
    }
    else if (INS_IsMemoryRead(ins))
    {
        // TYPE_CAT_LOAD:
        instrumentations++;
        INS_InsertCall(ins,IPOINT_BEFORE, 
                       (AFUNPTR)dofilter_load,
                       IARG_PREDICATE,
                       IARG_INST_PTR,
                       IARG_MEMORYREAD_EA,
                       IARG_END);
    }
    else if (INS_HasMemoryRead2(ins))
    {
        instrumentations++;
        INS_InsertCall(ins,IPOINT_BEFORE, 
                       (AFUNPTR)dofilter_load,
                       IARG_PREDICATE,
                       IARG_INST_PTR,
                       IARG_MEMORYREAD2_EA,
                       IARG_END);
    }
#if 0 //FIXME: iprefetch missing from PIN API
    else if (INS_IsInstructionPrefetch(ins))
    {
        // TYPE_CAT_FETCH:
        instrumentations++;
        INS_InsertCall(ins,IPOINT_BEFORE, 
                       (AFUNPTR)dofilter_prefetch,
                       IARG_PREDICATE,
                       IARG_INST_PTR,
                       IARG_MEMORYREAD_EA,
                       IARG_END);
    }
#endif
    else if (INS_IsAtomicUpdate(ins))
    {       // TYPE_CAT_FETCHADD: TYPE_CAT_CMPXCHG: TYPE_CAT_XCHG:
        instrumentations++;
        INS_InsertCall(ins,IPOINT_BEFORE, 
                       (AFUNPTR)dofilter_atomic,
                       IARG_PREDICATE,
                       IARG_INST_PTR,
                       IARG_MEMORYREAD_EA,
                       IARG_END);
    }

    tinfo->last = ins;
    tinfo->last_is_valid = true;
}

static void
alloc_filter_caches(const char* file_name)
{
    ////////////////////////
    // not currently used //
    ////////////////////////
    const UINT n = 4;
    const UINT cap = 4*1024;
    UINT linesz = 32;

    d_filter_caches = new FILTER_CACHES_T(n, file_name);
    my_alloc += sizeof(FILTER_CACHES_T);

    for(UINT i=0; i<n;i++) {
        d_filter_caches->init(i,cap,linesz);
        linesz *= 2;
    }
}

static void
validate_filter_cache_params(void)
{
    
    if (first_filter_cache.capacity != last_filter_cache.capacity)
    {
        cerr << "Filter caches must all be the same capacity" << endl;
        exit(1);
    }
    if (first_filter_cache.assoc != last_filter_cache.assoc)
    {
        cerr << "Filter caches must all be the same associativity." << endl;
        exit(1);
    }
    if (first_filter_cache.assoc != 1)
    {
        cerr << "Filter caches must all be direct mapped." << endl;
        exit(1);
    }
    if (first_filter_cache.linesz >  last_filter_cache.linesz)
    {
        cerr << "First filter cache must have a linesize <= " <<
            "that of the last filter cache." << endl;
        exit(1);
    }

    if (!is_power_of_2(first_filter_cache.capacity))
    {
        cerr << "Filter caches capacity must be power of 2." << endl;
        exit(1);
    }
    if (!is_power_of_2(first_filter_cache.linesz))
    {
        cerr << "Filter caches line-size must be power of 2." << endl;
        exit(1);
    }
    if (!is_power_of_2(last_filter_cache.linesz))
    {
        cerr << "Filter caches line-size must be power of 2." << endl;
        exit(1);
    }
}

static FILTER_CACHES_T* 
alloc_filter_caches_param(const char* file_name)
{
    FILTER_CACHES_T* fc = 0;

    if (use_filtering_caches)
    {
        validate_filter_cache_params();
        
        UINT n = 1;
        CACHE_CONFIG_T config = first_filter_cache;
        while(config.linesz < last_filter_cache.linesz)
        {
            n++;
            config.linesz *= 2;
        }
        
        
        fc = new FILTER_CACHES_T(n, file_name);
        my_alloc += sizeof(FILTER_CACHES_T);
        
        config = first_filter_cache;
        for(UINT i=0; i<n;i++) {
            fc->init(i,config.capacity,config.linesz);
            config.linesz *= 2;
        }
    }
    return fc;
}

static void
alloc_multi_cache()
{
    ////////////////////////
    // not currently used //
    ////////////////////////

    /* this routine allocates a fixed set of caches (described below) */

    const UINT ncap = 10;
    const UINT nassoc = 4;
    const UINT nlinesz = 4;


    d_multi_cache = new MULTI_CACHE_T(ncap*nassoc*nlinesz);
    my_alloc += sizeof(MULTI_CACHE_T);

    // capacities: 8 16 32 64 128 256 512 1024 2048 4096
    // associativities: 1 2 4 8
    // linesizes: 32 64 128 256

    UINT m = 0;
    UINT cap = 8*1024;
    for(UINT i=0; i<ncap;i++) {
        UINT assoc = 1;
        for(UINT j=0; j<nassoc;j++) {
            UINT linesz = 32;
            for(UINT k=0; k<nlinesz;k++) {
                
                d_multi_cache->init(m++,cap,assoc,linesz);
                linesz *= 2;
            }
            assoc *= 2;
        }   
        cap *= 2;
    }   
}

static void
validate_multi_cache_params(void)
{
    
    if (first_multi_cache.capacity > last_multi_cache.capacity)
    {
        cerr << "First cache\'s capacity must be <= that of the last cache."
             << endl;
        exit(1);
    }
    if (first_multi_cache.assoc > last_multi_cache.assoc)
    {
        cerr << "First cache\'s associativity must be <= that of the last cache."
             << endl;
        exit(1);
    }
    if (first_multi_cache.linesz >  last_multi_cache.linesz)
    {
        cerr << "First cache\'s have a linesize <= " <<
            "that of the last cache." << endl;
        exit(1);
    }
    
    if (!is_power_of_2(first_multi_cache.capacity))
    {
        cerr << "First cache\'s capacity must be power of 2." << endl;
        exit(1);
    }
    if (!is_power_of_2(last_multi_cache.capacity))
    {
        cerr << "Last cache\'s capacity must be power of 2." << endl;
        exit(1);
    }
    if (!is_power_of_2(first_multi_cache.linesz))
    {
        cerr << "First cache\'s line-size must be power of 2." << endl;
        exit(1);
    }
    if (!is_power_of_2(last_multi_cache.linesz))
    {
        cerr << "Last cache\'s line-size must be power of 2." << endl;
        exit(1);
    }

    if (!is_power_of_2(first_multi_cache.assoc))
    {
        cerr << "First cache\'s associativity must be power of 2." << endl;
        exit(1);
    }
    if (!is_power_of_2(last_multi_cache.assoc))
    {
        cerr << "Last cache\'s associativity must be power of 2." << endl;
        exit(1);
    }
}

static MULTI_CACHE_T*
alloc_multi_cache_param()
{
    MULTI_CACHE_T* c = 0;
    validate_multi_cache_params();

    
    // compute the number of caches  = ncap*nassoc*nlinesz
    UINT ncap = 1;
    UINT nassoc = 1;
    UINT nlinesz = 1;

    CACHE_CONFIG_T config = first_multi_cache;
    while(config.linesz < last_multi_cache.linesz)
    {
        nlinesz++;
        config.linesz *= 2;
    }

    while(config.capacity < last_multi_cache.capacity)
    {
        ncap++;
        config.capacity *= 2;
    }

    while(config.assoc < last_multi_cache.assoc )
    {
        nassoc++;
        config.assoc *= 2;
    }


    /* allocate the caches */

    c = new MULTI_CACHE_T(ncap*nassoc*nlinesz);
    my_alloc += sizeof(MULTI_CACHE_T);

    UINT m = 0;
    UINT cap = first_multi_cache.capacity;
    for(UINT i=0; i<ncap;i++) {
        UINT assoc = first_multi_cache.assoc;
        for(UINT j=0; j<nassoc;j++) {
            UINT linesz = first_multi_cache.linesz;
            for(UINT k=0; k<nlinesz;k++) {
                
                c->init(m++,cap,assoc,linesz);
                linesz *= 2;
            }
            assoc *= 2;
        }   
        cap *= 2;
    }   
    return c;
}


static void
alloc_caches(const char* file_name)
{
    d_multi_cache = alloc_multi_cache_param();    
    d_filter_caches = alloc_filter_caches_param(file_name);

    if (do_instruction_cache_sim)
    {
        i_multi_cache = alloc_multi_cache_param();    
        i_filter_caches = alloc_filter_caches_param(file_name);
    }
    
    *debug << "Allocated " << my_alloc << " bytes." << endl;
}


//////////////////////////////////////////////////////////////////////

static void
report(ofstream* mylog, UINT64 icount, time_t t1)
{
    bool newlines = true;
    brief_status(mylog,icount,newlines, t1);

    /* data caches */
    if (d_filter_caches)
    {
        d_filter_caches->report(mylog, icount, "data-filter");
    }
    d_multi_cache->report(mylog, icount, "data-multi");

    /* instruction caches */

    if (i_filter_caches)
    {
        i_filter_caches->report(mylog, icount, "inst-filter");
    }
    if (i_multi_cache)
    {
        i_multi_cache->report(mylog, icount, "inst-multi");
    }
}

static ofstream*
open_output_file(const char* fn, const char* type)
{
    ofstream* o = new ofstream(fn);
    if (!o)
    {
        cerr << prog_name << " error: could not open << " << type
             << " file: [" << fn << "]" << endl;
        exit(1);
    }
    return o;
}

static ofstream*
open_output_file(ostringstream& ofn, const char* type)
{
    const char* s = ofn.str().c_str();
    return open_output_file(s,type);
}

static ofstream*
dump_stats_file(ostringstream& os,
                const char* type,
                UINT64 icount,
                time_t t1)
{
    const char* s = os.str().c_str();
    ofstream* o = open_output_file(s,type);
    report(o, icount, t1);
    return o;
}


static void
interim_report(void)
{
    static UINT i=0;
    ostringstream fn;
    //*debug << "interim_report. fn_prefix=[" << fn_prefix << "]" <<  endl;
    fn << fn_prefix << ".pid." << pid << "." << i << ".tlog" << ends;

    i = (1-i); // alternate 0, 1, 0, 1, ...

    ofstream* o = dump_stats_file(fn, "tlog", icount.Count(), t1);
    o->close();
}

static void
fini(int exit_val, void* v)
{
    /*
     * This function is called by the pin infrastructure or this analayzer
     * when the program is ending.
     */
    *debug << "In fini()" << endl;
    if (sample_start_icount)
    {
        UINT64 sample_length = icount.Count() - sample_start_icount;
        finish_pinpoint(sample_length, pp_t1);
    }
    report(mylog, icount.Count(), t1);
    time_t t2 = time(0);
    *debug << " Total time = " << t2 - t1  << " seconds." << endl;
    *debug << " Closing log file." << endl;
    mylog->close();
    pplog->close();
    exit(exit_val);
}

////////////////////////////////////////////////////////////////////////////

static VOID 
InstrumentTrace(TRACE trace, VOID *v)
{
    instrument_trace_info_t tinfo;

    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
    {
        for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins))
        {
            InstrumentInstructions(ins, &tinfo);
        }
    }

    // we fell off the end of the trace -- instrument the exit
    if (tinfo.last_is_valid)
    {
        instrumentations++;
        INS_InsertCall(tinfo.last,IPOINT_AFTER,
                       tinfo.cleanup_fn,
                       IARG_END);
    }
}
    
///////////////////////////////////////////////////////////////////////

static CACHE_CONFIG_T
parse_cache(char* cache_config)
{
    CACHE_CONFIG_T x;
    UINT r;
    if (!cache_config) {
        cerr << "Null cache config given to parse_cache()" << endl;
        exit(1);
    }
    r = sscanf(cache_config,"%d/%d/%d",
               &x.capacity,
               &x.assoc,
               &x.linesz);

    x.capacity *= 1024; // convert to KB

    if (r != 3)
    {
        cerr << "Could not parse cache config string ["
             << cache_config << "]" << endl;
        cerr << "It should be of the form "
             << " CapcityKB/Associativity/LineSizeBytes" << endl;
        exit(1);
    }

    return x;
}

static const char* usage_msg[] = {
    "cachefilt options are: ",
    "-f             Dump a trace file of filtered referneces",
    "-fc  config   Set first and last filter cache config (KB/assoc/linesz)",
    "-fc1 config   First filter cache config (KB/assoc/linesz)",
    "-fc2 config   Last filter cache config (KB/assoc/linesz)",
    "-nf           no filtering caches",
    "-i            do instruciton cache simulation",
    "-c   config   Set first and last cache config (KB/assoc/linesz)",
    "-c1  config   First cache config (KB/assoc/linesz)",
    "-c2  config   Last fcache config (KB/assoc/linesz)",
    "-prefix string    File name prefix for the stats files",
    "-cold             Flush the cache directories at sample boundaries",
    "-- pin-client-app-and-args  (must be last)",
    "-p path-to-pin-binary (must be just before --)",
    "-h             Help. (This message)",
    "",
    "Notes:",
    "  1. The filter caches must be direct mapped and have the same capacity.",
    "  2. \".cfilt\" files are dumped at the end of the run",
    "  3. \".tlog\" files are dumped periodically -- there are two [0,1]",
    "  4. \".pp\" files hold the data for all pin points",
    "  5. \"debug\" files hold the recent icount",

    "",
    0
};

static void
usage(char* prog_name)
{
    cerr << "Usage: " << prog_name
         << " [cachefilt-options] [-p path-to-pin] "
         << "-- client-program-and-args"
         << endl
         << endl;

    UINT i=0;
    while(usage_msg[i]) {
        cerr << "  " << usage_msg[i] << endl;
        i++;
    }

    cerr << KNOB_BASE::StringKnobSummary() << endl;
}

static void
process_args(UINT argc, char **argv)
{

    UINT have_filter_cache_specs=0;
    UINT have_multi_cache_specs=0;
    bool have_random_seed = false;


    for(UINT i=1;i<argc;i++) {
        char* s = argv[i];
        if (strcmp(s,"-f") == 0) {
            do_trace_file = true;
        }
        else if (strcmp(s,"-prefix")==0 ||
                 strcmp(s,"--prefix")==0) {
            i++;
            fn_prefix = strdup(argv[i]);
        }
        else if (strcmp(s,"-i")==0) {
            do_instruction_cache_sim = true;
        }
        else if (strcmp(s,"-fc")==0) {
            i++;
            first_filter_cache = parse_cache(argv[i]);
            have_filter_cache_specs |= 1;
            last_filter_cache = parse_cache(argv[i]);
            have_filter_cache_specs |= 2;
        }
        else if (strcmp(s,"-fc1")==0) {
            i++;
            first_filter_cache = parse_cache(argv[i]);
            have_filter_cache_specs |= 1;
        }
        else if (strcmp(s,"-fc2")==0) {
            i++;
            last_filter_cache = parse_cache(argv[i]);
            have_filter_cache_specs |= 2;
        }
        else if (strcmp(s,"-nf")==0) {
            use_filtering_caches = false;
        }
        else if (strcmp(s,"-cold")==0) {
            flush_directory_at_sample_boundary = true;
        }
        else if (strcmp(s,"-c")==0) {
            i++;
            first_multi_cache = parse_cache(argv[i]);
            have_multi_cache_specs |= 1;
            last_multi_cache = parse_cache(argv[i]);
            have_multi_cache_specs |= 2;
        }
        else if (strcmp(s,"-c1")==0) {
            i++;
            first_multi_cache = parse_cache(argv[i]);
            have_multi_cache_specs |= 1;
        }
        else if (strcmp(s,"-c2")==0) {
            i++;
            last_multi_cache = parse_cache(argv[i]);
            have_multi_cache_specs |= 2;
        }
        else if (strcmp(s,"-h")==0 ||
                 strcmp(s,"-help")==0 ||
                 strcmp(s,"--help")==0 ||
                 strcmp(s,"-?")==0)
        {
            i++;
            usage(prog_name);
            exit(1);
        }
        else if (strcmp(s,"-p") == 0 ||
                 strcmp(s,"--") == 0) {
            // hit the pin args 
            break;
        }
        else {
            cerr << "Error: unknown option [" << s << "]" << endl;
            usage(prog_name);
            exit(1);
        }
    }
    if (have_filter_cache_specs && have_filter_cache_specs != 3) {
        cerr << "Error: You must specify two filter caches "
             << "(-fc1 & -fc2)" << endl;
        exit(1);
    }

    if (have_multi_cache_specs && have_multi_cache_specs != 3) {
        cerr << "Error: You must specify two caches (-c1 & -c2)" << endl;
        exit(1);
    }
    
}

int
main(int argc, char **argv)
{
    const UINT uargc = argc;
    prog_name = strdup(argv[0]);

    debug = open_output_file("debug", "debug");
    pid = PIN_GetPid();
    t1 = time(0);

    ostringstream trace_fn;
    trace_fn << "cachefilter.trace." << pid << ends;

    if (PIN_Init(argc,argv))
    {
        usage(argv[0]);
        exit(1);
    }

    process_args(uargc, argv);

    ostringstream log_fn;
    log_fn <<  fn_prefix << ".pid." << pid << ".cfilt" << ends;
    mylog = open_output_file(log_fn, "stats log");

    ostringstream pp_fn;
    pp_fn << fn_prefix << ".pid." << pid << ".pp" << ends;
    pplog = open_output_file(pp_fn, "pin point log file");

    // save args and PID
    *mylog<< pid << " ";
    for(UINT i=0;i<uargc;i++) {
        *mylog << argv[i] << " ";
    }
    *mylog << endl;



    alloc_caches( do_trace_file ? trace_fn.str().c_str() : 0);

    set_min_line_size_mask();
    assert(min_line_size_mask != 0);

    // Register Instruction to be called to instrument instructions
    TRACE_AddInstrumentFunction(InstrumentTrace, 0);
    ///FIXME:PORT    PIN_AddInstrumentSequenceFunction(sequence,0);

    PIN_AddFiniFunction(fini,0);

    icount.Activate();

    // Activate alarm, must be done before PIN_StartProgram
    control.CheckKnobs(Handler, 0);

    *debug << "Calling PIN_StartProgram." << endl;
    PIN_StartProgram();
    /* notreached */
    return 0;
}

