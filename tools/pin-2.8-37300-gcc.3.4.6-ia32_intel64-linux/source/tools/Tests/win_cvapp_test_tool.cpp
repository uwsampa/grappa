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
/* ===================================================================== */
/*! @file This file contains a static and dynamic opcode/ISA extension/ISA
 *  category mix profiler
 *
 * This is derived from mix.cpp. Handles an arbitrary number of threads 
 * using TLS for data storage and avoids locking, except during I/O.
 */

#include "pin.H"
#include "instlib.H"
#include "portability.H"
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <cstdlib>
#include <map>
#if defined(TARGET_IPF)
# include "ipf_opcode.H"
#endif
using namespace INSTLIB;

// key for accessing TLS storage in the threads. initialized once in main()
static  TLS_KEY tls_key;

typedef UINT32 stat_index_t;

#if defined(TARGET_IA32) || defined(TARGET_IA32E)
static string disassemble(UINT64 start, UINT64 stop);
#endif
/* ===================================================================== */
/* Commandline Switches */
/* ===================================================================== */

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE,         "pintool",
    "o", "mix.out", "specify profile file name");
KNOB<UINT32> KnobTopBlocks(KNOB_MODE_WRITEONCE,         "pintool",
    "top_blocks", "20", "specify a maximal number of top blocks for which icounts are printed");
#if defined(TARGET_IA32) || defined(TARGET_IA32E)
KNOB<BOOL>   KnobShowDisassembly(KNOB_MODE_WRITEONCE,                "pintool",
    "disas", "0", "Show disassembly for top blocks");
#endif
KNOB<BOOL>   KnobPid(KNOB_MODE_WRITEONCE,                "pintool",
    "i", "0", "append pid to output");
KNOB<BOOL>   KnobProfilePredicated(KNOB_MODE_WRITEONCE,  "pintool",
    "p", "0", "enable accurate profiling for predicated instructions");
KNOB<BOOL>   KnobProfileStaticOnly(KNOB_MODE_WRITEONCE,  "pintool",
    "s", "0", "terminate after collection of static profile for main image");
#ifndef TARGET_WINDOWS
KNOB<BOOL>   KnobProfileDynamicOnly(KNOB_MODE_WRITEONCE, "pintool",
    "d", "0", "Only collect dynamic profile");
#else
KNOB<BOOL>   KnobProfileDynamicOnly(KNOB_MODE_WRITEONCE, "pintool",
    "d", "1", "Only collect dynamic profile");
#endif
KNOB<BOOL>   KnobNoSharedLibs(KNOB_MODE_WRITEONCE,       "pintool",
    "no_shared_libs", "0", "do not instrument shared libraries");

KNOB<BOOL> KnobInstructionLengthMix(KNOB_MODE_WRITEONCE,  "pintool","ilen", "0", "Compute instruction length mix");
KNOB<BOOL> KnobCategoryMix(KNOB_MODE_WRITEONCE, "pintool", "category", "0", "Compute ISA category mix");
KNOB<BOOL> KnobIformMix(KNOB_MODE_WRITEONCE, "pintool", "iform", "0", "Compute ISA iform mix");
KNOB<BOOL> KnobMapToFile(KNOB_MODE_WRITEONCE, "pintool", "mapaddr", "0", "Map Addresses to File/Line information");

typedef enum { measure_opcode=0, measure_category=1, measure_ilen=2, measure_iform=3 } measurement_t;
measurement_t measurement = measure_opcode;

/* ===================================================================== */

INT32 Usage()
{
    cerr << "This pin tool computes a static and dynamic opcode, "
         << "instruction form, instruction length, extension or category mix profile\n\n";
    cerr << KNOB_BASE::StringKnobSummary();
    cerr << endl;
    cerr << "The default is to do opcode and ISA extension profileing" << endl;
    cerr << "At most one of -iform, -ilen or  -category is allowed" << endl;
    cerr << endl;
    return -1;
}

/* ===================================================================== */
/* INDEX HELPERS */
/* ===================================================================== */

const UINT32 INDEX_SPECIAL =  3000;
const UINT32 MAX_MEM_SIZE = 520;
const UINT32 MAX_EXTENSION = 50;

const UINT32 INDEX_TOTAL =          INDEX_SPECIAL + 0;
const UINT32 INDEX_MEM_ATOMIC =     INDEX_SPECIAL + 1;
const UINT32 INDEX_STACK_READ =     INDEX_SPECIAL + 2;
const UINT32 INDEX_STACK_WRITE =    INDEX_SPECIAL + 3;
const UINT32 INDEX_IPREL_READ =     INDEX_SPECIAL + 4;
const UINT32 INDEX_IPREL_WRITE =    INDEX_SPECIAL + 5;
const UINT32 INDEX_MEM_READ_SIZE =  INDEX_SPECIAL + 6;
const UINT32 INDEX_MEM_WRITE_SIZE = INDEX_SPECIAL + 6 + MAX_MEM_SIZE;

const UINT32 INDEX_EXTENSION   = INDEX_SPECIAL + 6 + 2*MAX_MEM_SIZE;

const UINT32 INDEX_FMA_BASE   = INDEX_EXTENSION + MAX_EXTENSION;
const UINT32 INDEX_FMA        = INDEX_FMA_BASE + 1;
const UINT32 INDEX_FMA_ADD    = INDEX_FMA_BASE + 2;
const UINT32 INDEX_FMA_MUL    = INDEX_FMA_BASE + 3;
const UINT32 INDEX_FMA_S      = INDEX_FMA_BASE + 4;
const UINT32 INDEX_FMA_S_ADD  = INDEX_FMA_BASE + 5; // NOTE: skipped 6. does not matter
const UINT32 INDEX_FMA_S_MUL  = INDEX_FMA_BASE + 7;
const UINT32 INDEX_FMA_D      = INDEX_FMA_BASE + 8;
const UINT32 INDEX_FMA_D_ADD  = INDEX_FMA_BASE + 9;
const UINT32 INDEX_FMA_D_MUL  = INDEX_FMA_BASE + 10;
const UINT32 INDEX_FPMA       = INDEX_FMA_BASE + 11;
const UINT32 INDEX_FPMA_ADD   = INDEX_FMA_BASE + 12;
const UINT32 INDEX_FPMA_MUL   = INDEX_FMA_BASE + 13;
const UINT32 INDEX_FMS        = INDEX_FMA_BASE + 14;
const UINT32 INDEX_FMS_SUB    = INDEX_FMA_BASE + 15;
const UINT32 INDEX_FMS_MUL    = INDEX_FMA_BASE + 16;
const UINT32 INDEX_FMS_S      = INDEX_FMA_BASE + 17;
const UINT32 INDEX_FMS_S_SUB  = INDEX_FMA_BASE + 18;
const UINT32 INDEX_FMS_S_MUL  = INDEX_FMA_BASE + 19;
const UINT32 INDEX_FMS_D      = INDEX_FMA_BASE + 20;
const UINT32 INDEX_FMS_D_SUB  = INDEX_FMA_BASE + 21;
const UINT32 INDEX_FMS_D_MUL  = INDEX_FMA_BASE + 22;
const UINT32 INDEX_FPMS       = INDEX_FMA_BASE + 23;
const UINT32 INDEX_FPMS_SUB   = INDEX_FMA_BASE + 24;
const UINT32 INDEX_FPMS_MUL   = INDEX_FMA_BASE + 25;
const UINT32 INDEX_FNMA       = INDEX_FMA_BASE + 26;
const UINT32 INDEX_FNMA_ADD   = INDEX_FMA_BASE + 27;
const UINT32 INDEX_FNMA_MUL   = INDEX_FMA_BASE + 28;
const UINT32 INDEX_FNMA_S     = INDEX_FMA_BASE + 29;
const UINT32 INDEX_FNMA_S_ADD = INDEX_FMA_BASE + 30;
const UINT32 INDEX_FNMA_S_MUL = INDEX_FMA_BASE + 31;
const UINT32 INDEX_FNMA_D     = INDEX_FMA_BASE + 32;
const UINT32 INDEX_FNMA_D_ADD = INDEX_FMA_BASE + 33;
const UINT32 INDEX_FNMA_D_MUL = INDEX_FMA_BASE + 34;
const UINT32 INDEX_FPNMA      = INDEX_FMA_BASE + 35;
const UINT32 INDEX_FPNMA_ADD  = INDEX_FMA_BASE + 36;
const UINT32 INDEX_FPNMA_MUL  = INDEX_FMA_BASE + 37;

const UINT32 INDEX_SPECIAL_END   =  INDEX_FMA_BASE + 38;

BOOL IsMemReadIndex(UINT32 i)
{
    return (INDEX_MEM_READ_SIZE <= i && i < INDEX_MEM_READ_SIZE + MAX_MEM_SIZE );
}

BOOL IsMemWriteIndex(UINT32 i)
{
    return (INDEX_MEM_WRITE_SIZE <= i && i < INDEX_MEM_WRITE_SIZE + MAX_MEM_SIZE );
}


/* ===================================================================== */
LOCALFUN UINT32 INS_GetIndex(INS ins)
{
    UINT32 index = 0;
    switch(measurement) {
      case measure_opcode:
        index = INS_Opcode(ins);
        break;
      case measure_ilen:
        index = INS_Size(ins);
        break;
      case measure_category:
        index = INS_Category(ins);
        break;
      case measure_iform: 
        {
#if defined(TARGET_IA32) || defined(TARGET_IA32E)
            xed_decoded_inst_t* xedd = INS_XedDec(ins);
            xed_iform_enum_t iform = xed_decoded_inst_get_iform_enum(xedd);
            index = static_cast<UINT32>(iform);
#endif
        }
        break;
    }
    return index;
}

/* ===================================================================== */

LOCALFUN BOOL INS_IsFMA(INS ins)
{
#if defined(TARGET_IPF)
    IA64OPCODES opcode = static_cast<IA64OPCODES>(INS_Opcode(ins));
    switch(opcode) {
      case IA64OP_FMA_S0:
      case IA64OP_FMA_S1:
      case IA64OP_FMA_S2:
      case IA64OP_FMA_S3:

      case IA64OP_FMA_S_S0:
      case IA64OP_FMA_S_S1:
      case IA64OP_FMA_S_S2:
      case IA64OP_FMA_S_S3:

      case IA64OP_FMA_D_S0:
      case IA64OP_FMA_D_S2:
      case IA64OP_FMA_D_S1:
      case IA64OP_FMA_D_S3:

      case IA64OP_FPMA_S0:
      case IA64OP_FPMA_S1:
      case IA64OP_FPMA_S2:
      case IA64OP_FPMA_S3:

      case IA64OP_FMS_S0:
      case IA64OP_FMS_S1:
      case IA64OP_FMS_S2:
      case IA64OP_FMS_S3:

      case IA64OP_FMS_S_S0:
      case IA64OP_FMS_S_S1:
      case IA64OP_FMS_S_S2:
      case IA64OP_FMS_S_S3:

      case IA64OP_FMS_D_S0:
      case IA64OP_FMS_D_S1:
      case IA64OP_FMS_D_S2:
      case IA64OP_FMS_D_S3:

      case IA64OP_FPMS_S0:
      case IA64OP_FPMS_S1:
      case IA64OP_FPMS_S2:
      case IA64OP_FPMS_S3:

      case IA64OP_FNMA_S0:
      case IA64OP_FNMA_S1:
      case IA64OP_FNMA_S2:
      case IA64OP_FNMA_S3:

      case IA64OP_FNMA_S_S0:
      case IA64OP_FNMA_S_S1:
      case IA64OP_FNMA_S_S2:
      case IA64OP_FNMA_S_S3:

      case IA64OP_FNMA_D_S0:
      case IA64OP_FNMA_D_S1:
      case IA64OP_FNMA_D_S2:
      case IA64OP_FNMA_D_S3:

      case IA64OP_FPNMA_S0:
      case IA64OP_FPNMA_S1:
      case IA64OP_FPNMA_S2:
      case IA64OP_FPNMA_S3:
        return TRUE;

      default:
        break;
    }
#endif
    return FALSE;
}

/* ===================================================================== */

LOCALFUN  UINT32 IndexStringLength(BBL bbl, BOOL memory_access_profile)
{
    UINT32 count = 0;

    for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins))
    {
        count++; // one for the ins 
        if (measurement != measure_iform)
            count++;  // one for the ISA extension.

        if( measurement == measure_opcode && memory_access_profile )
        {
            if( INS_IsMemoryRead(ins) ) count++;   // for size

            if( INS_IsStackRead(ins) ) count++;

            if( INS_IsIpRelRead(ins) ) count++;

            
            if( INS_IsMemoryWrite(ins) ) count++; // for size

            if( INS_IsStackWrite(ins) ) count++;

            if( INS_IsIpRelWrite(ins) ) count++;

            if( INS_IsAtomicUpdate(ins) ) count++;

            if( INS_IsFMA(ins) ) count++;
        }
    }
    
    return count;
}


/* ===================================================================== */
LOCALFUN UINT32 MemsizeToIndex(UINT32 size, BOOL write)
{
    return (write ? INDEX_MEM_WRITE_SIZE : INDEX_MEM_READ_SIZE ) + size;
}


LOCALFUN stat_index_t *INS_GenerateIndexFMA(INS ins, stat_index_t *stats)
{
#if defined(TARGET_IPF)

    const UINT32 fma[] = { INDEX_FMA, INDEX_FMA_ADD, INDEX_FMA_MUL };
    const UINT32 fma_s[] = { INDEX_FMA_S, INDEX_FMA_S_ADD, INDEX_FMA_S_MUL };
    const UINT32 fma_d[] = { INDEX_FMA_D, INDEX_FMA_D_ADD, INDEX_FMA_D_MUL };
    const UINT32 fpma[] = { INDEX_FPMA, INDEX_FPMA_ADD, INDEX_FPMA_MUL };
    const UINT32 fms[] = { INDEX_FMS, INDEX_FMS_SUB, INDEX_FMS_MUL };
    const UINT32 fms_s[] = { INDEX_FMS_S, INDEX_FMS_S_SUB, INDEX_FMS_S_MUL };
    const UINT32 fms_d[] = { INDEX_FMS_D, INDEX_FMS_D_SUB, INDEX_FMS_D_MUL };
    const UINT32 fpms[] = { INDEX_FPMS, INDEX_FPMS_SUB, INDEX_FPMS_MUL };
    const UINT32 fnma[] = { INDEX_FNMA, INDEX_FNMA_ADD, INDEX_FNMA_MUL };
    const UINT32 fnma_s[] = { INDEX_FNMA_S, INDEX_FNMA_S_ADD, INDEX_FNMA_S_MUL };
    const UINT32 fnma_d[] = { INDEX_FNMA_D, INDEX_FNMA_D_ADD, INDEX_FNMA_D_MUL };
    const UINT32 fpnma[] = { INDEX_FPNMA, INDEX_FPNMA_ADD, INDEX_FPNMA_MUL };
    
    const UINT32* p = 0;
    
    IA64OPCODES opcode = static_cast<IA64OPCODES>(INS_Opcode(ins));
    switch(opcode) {
      case IA64OP_FMA_S0:
      case IA64OP_FMA_S1:
      case IA64OP_FMA_S2:
      case IA64OP_FMA_S3:
        p = fma;
        break;

      case IA64OP_FMA_S_S0:
      case IA64OP_FMA_S_S1:
      case IA64OP_FMA_S_S2:
      case IA64OP_FMA_S_S3:
        p = fma_s;
        break;

      case IA64OP_FMA_D_S0:
      case IA64OP_FMA_D_S2:
      case IA64OP_FMA_D_S1:
      case IA64OP_FMA_D_S3:
        p = fma_d;
        break;

      case IA64OP_FPMA_S0:
      case IA64OP_FPMA_S1:
      case IA64OP_FPMA_S2:
      case IA64OP_FPMA_S3:
        p = fpma;
        break;

      case IA64OP_FMS_S0:
      case IA64OP_FMS_S1:
      case IA64OP_FMS_S2:
      case IA64OP_FMS_S3:
        p = fms;
        break;

      case IA64OP_FMS_S_S0:
      case IA64OP_FMS_S_S1:
      case IA64OP_FMS_S_S2:
      case IA64OP_FMS_S_S3:
        p = fms_s;
        break;

      case IA64OP_FMS_D_S0:
      case IA64OP_FMS_D_S1:
      case IA64OP_FMS_D_S2:
      case IA64OP_FMS_D_S3:
        p = fms_d;
        break;

      case IA64OP_FPMS_S0:
      case IA64OP_FPMS_S1:
      case IA64OP_FPMS_S2:
      case IA64OP_FPMS_S3:
        p = fpms;
        break;

      case IA64OP_FNMA_S0:
      case IA64OP_FNMA_S1:
      case IA64OP_FNMA_S2:
      case IA64OP_FNMA_S3:
        p = fnma;
        break;


      case IA64OP_FNMA_S_S0:
      case IA64OP_FNMA_S_S1:
      case IA64OP_FNMA_S_S2:
      case IA64OP_FNMA_S_S3:
        p = fnma_s;
        break;

      case IA64OP_FNMA_D_S0:
      case IA64OP_FNMA_D_S1:
      case IA64OP_FNMA_D_S2:
      case IA64OP_FNMA_D_S3:
        p = fnma_d;
        break;

      case IA64OP_FPNMA_S0:
      case IA64OP_FPNMA_S1:
      case IA64OP_FPNMA_S2:
      case IA64OP_FPNMA_S3:
        p = fpnma;
        break;
      default:
        break;
    }

    if (p) 
    {
        //cout << INS_Disassemble(ins);
        int index = 0;
        //ASSERTX(INS_OperandCount(ins) == 4);
        REG src1=INS_RegR(ins,1); // summand
        REG src2=INS_RegR(ins,2); // multiplier
        REG src3=INS_RegR(ins,3); // multiplier
        // look for f0 as src1 -> mul
        if (src1 == REG_FZERO) {
            //cout << " ---> MUL ";
            index = 2;
        }
        // look for f1 as a src2 or src3 -> add
        //cout << " " << REG_StringShort(src1) << " " << REG_StringShort(src2) 
        //     << " " << REG_StringShort(src3);
        if (src2 == REG_FONE || src3 == REG_FONE) {
            //cout << " ---> ADD ";
            index = 1;
        }
        //cout << endl;
        *stats++ = p[index];
    }
#endif
    return stats;
}
/* ===================================================================== */
LOCALFUN stat_index_t* INS_GenerateIndexString(INS ins, stat_index_t *stats, BOOL memory_access_profile)
{
    *stats++ = INS_GetIndex(ins);
    if (measurement != measure_iform)
        *stats++ = INS_Extension(ins) + INDEX_EXTENSION;

    if( measurement == measure_opcode && memory_access_profile )
    {
        if( INS_IsMemoryRead(ins) )  *stats++ = MemsizeToIndex( INS_MemoryReadSize(ins), 0 );
        if( INS_IsMemoryWrite(ins) ) *stats++ = MemsizeToIndex( INS_MemoryWriteSize(ins), 1 );
        
        if( INS_IsAtomicUpdate(ins) ) *stats++ = INDEX_MEM_ATOMIC;
        
        if( INS_IsStackRead(ins) ) *stats++ = INDEX_STACK_READ;
        if( INS_IsStackWrite(ins) ) *stats++ = INDEX_STACK_WRITE;
        
        if( INS_IsIpRelRead(ins) ) *stats++ = INDEX_IPREL_READ;
        if( INS_IsIpRelWrite(ins) ) *stats++ = INDEX_IPREL_WRITE;
    }

    return stats;
}


/* ===================================================================== */

LOCALFUN string IndexToString( UINT32 index )
{
    if (measurement == measure_iform) 
    {
#if defined(TARGET_IA32) || defined(TARGET_IA32E)
        return xed_iform_enum_t2str(static_cast<xed_iform_enum_t>(index));
#else
        return "???";
#endif
    }

    if( INDEX_SPECIAL <= index  && index < INDEX_SPECIAL_END)
    {
        if( index == INDEX_TOTAL )            return  "*total";
        else if( IsMemReadIndex(index) )      return  "*mem-read-" + decstr( index - INDEX_MEM_READ_SIZE );
        else if( IsMemWriteIndex(index))      return  "*mem-write-" + decstr( index - INDEX_MEM_WRITE_SIZE );
        else if( index == INDEX_MEM_ATOMIC )  return  "*mem-atomic";
        else if( index == INDEX_STACK_READ )  return  "*stack-read";
        else if( index == INDEX_STACK_WRITE ) return  "*stack-write";
        else if( index == INDEX_IPREL_READ )  return  "*iprel-read";
        else if( index == INDEX_IPREL_WRITE ) return  "*iprel-write";

        else if (index >= INDEX_EXTENSION && index < INDEX_EXTENSION + MAX_EXTENSION) 
            return "*isa-ext-" + EXTENSION_StringShort(index - INDEX_EXTENSION);

        else if ( index == INDEX_FMA         ) return "*FMA";
        else if ( index == INDEX_FMA_ADD     ) return "*FMA_ADD";
        else if ( index == INDEX_FMA_MUL     ) return "*FMA_MUL";
        else if ( index == INDEX_FMA_S       ) return "*FMA_S";
        else if ( index == INDEX_FMA_S_ADD   ) return "*FMA_S_ADD";
        else if ( index == INDEX_FMA_S_MUL   ) return "*FMA_S_MUL";
        else if ( index == INDEX_FMA_D       ) return "*FMA_D";
        else if ( index == INDEX_FMA_D_ADD   ) return "*FMA_D_ADD";
        else if ( index == INDEX_FMA_D_MUL   ) return "*FMA_D_MUL";
        else if ( index == INDEX_FPMA        ) return "*FPMA";
        else if ( index == INDEX_FPMA_ADD    ) return "*FPMA_ADD";
        else if ( index == INDEX_FPMA_MUL    ) return "*FPMA_MUL";
        else if ( index == INDEX_FMS         ) return "*FMS";
        else if ( index == INDEX_FMS_SUB     ) return "*FMS_SUB";
        else if ( index == INDEX_FMS_MUL     ) return "*FMS_MUL";
        else if ( index == INDEX_FMS_S       ) return "*FMS_S";
        else if ( index == INDEX_FMS_S_SUB   ) return "*FMS_S_SUB";
        else if ( index == INDEX_FMS_S_MUL   ) return "*FMS_S_MUL";
        else if ( index == INDEX_FMS_D       ) return "*FMS_D";
        else if ( index == INDEX_FMS_D_SUB   ) return "*FMS_D_SUB";
        else if ( index == INDEX_FMS_D_MUL   ) return "*FMS_D_MUL";
        else if ( index == INDEX_FPMS        ) return "*FPMS";
        else if ( index == INDEX_FPMS_SUB    ) return "*FPMS_SUB";
        else if ( index == INDEX_FPMS_MUL    ) return "*FPMS_MUL";
        else if ( index == INDEX_FNMA        ) return "*FNMA";
        else if ( index == INDEX_FNMA_ADD    ) return "*FNMA_ADD";
        else if ( index == INDEX_FNMA_MUL    ) return "*FNMA_MUL";
        else if ( index == INDEX_FNMA_S      ) return "*FNMA_S";
        else if ( index == INDEX_FNMA_S_ADD  ) return "*FNMA_S_ADD";
        else if ( index == INDEX_FNMA_S_MUL  ) return "*FNMA_S_MUL";
        else if ( index == INDEX_FNMA_D      ) return "*FNMA_D";
        else if ( index == INDEX_FNMA_D_ADD  ) return "*FNMA_D_ADD";
        else if ( index == INDEX_FNMA_D_MUL  ) return "*FNMA_D_MUL";
        else if ( index == INDEX_FPNMA       ) return "*FPNMA";
        else if ( index == INDEX_FPNMA_ADD   ) return "*FPNMA_ADD";
        else if ( index == INDEX_FPNMA_MUL   ) return "*FPNMA_MUL";

        else
        {
            ASSERTX(0);
            return "";
        }
    }
    else if (measurement == measure_ilen)
    {
        ostringstream s;
        s << "ILEN-" << index;
        return s.str();
    }
    else if (measurement == measure_opcode)
    {
        return OPCODE_StringShort(index);
    }
    else if (measurement == measure_category)
    {
        return CATEGORY_StringShort(index);
    }
    ASSERTX(0);
    return "";
    
}

/* ===================================================================== */
/* ===================================================================== */
typedef UINT64 COUNTER;


/* zero initialized */

typedef map<UINT32,COUNTER> stat_map_t;

class CSTATS
{
  public:
    CSTATS()
    {
        clear();
    }

    stat_map_t unpredicated;
    stat_map_t predicated;
    stat_map_t predicated_true;

    VOID clear()
    {
        unpredicated.erase(unpredicated.begin(),unpredicated.end());
        predicated.erase(predicated.begin(),predicated.end());
        predicated_true.erase(predicated_true.begin(),predicated_true.end());
    }
};

class BBL_SORT_STATS
{
  public:
    ADDRINT _pc;
    UINT64 _icount;
    UINT64 _executions;
    UINT64 _nbytes; 
};

CSTATS GlobalStatsStatic;  // summary stats for static analysis

class BBLSTATS
{
    // Our first pass sets up the types of stats we need to update for this
    // block. We have one stat per instruction in the block. The _stats
    // array is null terminated.
  public:
    const stat_index_t* const _stats;
    const ADDRINT _pc; // start PC of the block
    const UINT32 _ninst; // # of instructions
    const UINT32 _nbytes; // # of bytes in the block
    BBLSTATS(stat_index_t* stats, ADDRINT pc, UINT32 ninst, UINT32 nbytes) : _stats(stats), _pc(pc), 
                                                                       _ninst(ninst), _nbytes(nbytes) {    };
};

LOCALVAR vector<BBLSTATS*> statsList;

/* ===================================================================== */

#if defined(__GNUC__)
#  if defined(TARGET_MAC) || defined(TARGET_WINDOWS) 
     // MAC XCODE2.4.1 gcc and Cgywin gcc 3.4.x only allow for 16b
     // alignment! So we need to pad!
#    define ALIGN_LOCK __attribute__ ((aligned(16)))
#    define NEED_TO_PAD
#  else
#    define ALIGN_LOCK __attribute__ ((aligned(64)))
#  endif
#else
# define ALIGN_LOCK __declspec(align(64))
#endif

#if defined(NEED_TO_PAD)
LOCALVAR char pad0[48];
#endif
LOCALVAR PIN_LOCK  ALIGN_LOCK lock;
#if defined(NEED_TO_PAD)
LOCALVAR char pad1[48];
#endif
LOCALVAR PIN_LOCK  ALIGN_LOCK bbl_list_lock;
#if defined(NEED_TO_PAD)
LOCALVAR char pad2[48];
#endif

static std::ofstream* out;

class thread_data_t
{
  public:
    thread_data_t()
        : enabled(0)
    {
    }
    CSTATS cstats;
    UINT32 enabled;

    vector<COUNTER> block_counts;
    
    UINT32 size() 
    {
        UINT32 limit;
        limit = block_counts.size();
        return limit;
    }

    void resize(UINT32 n)
    {
        if (size() < n)
            block_counts.resize(2*n);
    }

};

thread_data_t* get_tls(THREADID tid)
{
    thread_data_t* tdata = 
          static_cast<thread_data_t*>(PIN_GetThreadData(tls_key, tid));
    return tdata;
}

VOID activate_counting(THREADID tid)
{
    thread_data_t* tdata = get_tls(tid);
    tdata->enabled = 1;
}
VOID deactivate_counting(THREADID tid)
{
    thread_data_t* tdata = get_tls(tid);
    tdata->enabled = 0;
}

UINT32 numThreads = 0;

VOID ThreadStart(THREADID tid, CONTEXT *ctxt, INT32 flags, VOID *v)
{
    // This function is locked no need for a Pin Lock here
    numThreads++;
    GetLock(&lock, tid+1); // for output
    *out << "# Starting tid " << tid << endl;
    ReleaseLock(&lock);

    thread_data_t* tdata = new thread_data_t;
    // remember my pointer for later
    PIN_SetThreadData(tls_key, tdata, tid);

    // make sure the thread is counting stuff.  

    // FIXME: The controller should start all threads if no trigger
    // conditions are specified, but currently it only starts
    // TID0. Starting here is wrong if the controller has a nontrivial
    // starting condition, but this is what most people want. They can
    // always stop the controller and zero the stats using markers as a
    // workaround.

    if (tid) 
        activate_counting(tid);
}




VOID emit_stats(THREADID tid); //forward prototype
VOID emit_pc_stats(THREADID tid); //forward prototype
VOID zero_stats(THREADID tid); //forward prototype

VOID emit_bbl_stats_sorted(THREADID tid);
LOCALVAR CONTROL control;
LOCALVAR CONTROL_STATS control_stats;



LOCALFUN VOID Handler(CONTROL_EVENT ev, VOID *val, CONTEXT *ctxt, VOID *ip, THREADID tid)
{
    switch(ev)
    {
      case CONTROL_START:
        GetLock(&lock, tid+1); // for output
        *out << "# Start counting for tid " << tid << endl;
        ReleaseLock(&lock);
        activate_counting(tid);
        break;
      case CONTROL_STOP:
        GetLock(&lock, tid+1); // for output
        *out << "# Stop counting for tid "  << tid << endl;
        if (control.PinPointsActive()) {
            UINT32 pp = control.CurrentPp(tid);
            UINT32 phase  = control.CurrentPhase(tid);
            *out << "# PinPointNumber " << pp << endl;
            *out << "# PinPointPhase " << phase << endl;
        }
        ReleaseLock(&lock);
        deactivate_counting(tid);
        if (control.PinPointsActive()) {
            // when doing pinpoints "mixes" we want to emit and then zero the stats when we stop a region.
            emit_stats(tid);
            emit_bbl_stats_sorted(tid);
            zero_stats(tid);
        }
        break;
      default:
        ASSERTX(false);
    }
}




LOCALFUN VOID HandlerStats(CONTROL_STATS_EVENT ev, VOID *val, CONTEXT* dummy_context, VOID *ip, THREADID tid)
{
    switch(ev)
    {
      case CONTROL_STATS_EMIT:
        GetLock(&lock, tid+1); // for output
        *out << "# Emit stats for tid " << tid << endl;
        ReleaseLock(&lock);
        emit_stats(tid);
        break;
      case CONTROL_STATS_RESET:
        GetLock(&lock, tid+1); // for output
        *out << "# Reset stats for tid " << tid << endl;
        ReleaseLock(&lock);
        zero_stats(tid);
        break;
      default:
        ASSERTX(false);
    }
}



/* ===================================================================== */
VOID validate_bbl_count(THREADID tid, ADDRINT block_count_for_trace)
{
    thread_data_t* tdata = get_tls(tid);
    tdata->resize(block_count_for_trace+1);
}

VOID PIN_FAST_ANALYSIS_CALL docount_bbl(ADDRINT block_id, THREADID tid)
{
    thread_data_t* tdata = get_tls(tid);
    //ASSERTX(tdata->size() > block_id);
    tdata->block_counts[block_id] += tdata->enabled;
}


VOID docount_predicated_true(UINT32 index, THREADID tid)
{
    thread_data_t* tdata = get_tls(tid);
    if (tdata->enabled) {
        stat_map_t::iterator i = tdata->cstats.predicated_true.find(index);
        if (i == tdata->cstats.predicated_true.end())
            tdata->cstats.predicated_true[index] = 1;
        else
            i->second += 1;
    }
}

/* ===================================================================== */

VOID zero_stats(THREADID tid)
{
    thread_data_t* tdata = get_tls(tid);
    tdata->cstats.clear();
    UINT32 limit =  tdata->size();
    for(UINT32 i=0;i< limit;i++)
        tdata->block_counts[i]=0;
}
/* ===================================================================== */

VOID CheckForSpecialMarkers(INS ins, ADDRINT pc, unsigned int instruction_size)
{
    // This checks for single instances of special 3B NOPs.  
    // 0F1FF3 - start
    // 0F1FF4 - stop
    // 0F1FF5 - emit stats
    // 0F1FF6 - zero stats

    // FIXME: if there are collisions with existing instructions, we can
    // change them here.

    //FIXME: Ideally this would be integrated in to the control.H so file
    //so that anything can use it.
    if (instruction_size != 3)
        return;

    UINT8* pc_ptr = reinterpret_cast<UINT8*>(pc);
    if (pc_ptr[0] == 0x0F &&
        pc_ptr[1] == 0x1F)
    {
        switch(pc_ptr[2])
        {
          case 0xF3: // start
            INS_InsertCall(ins, 
                           IPOINT_BEFORE, 
                           (AFUNPTR)activate_counting,
                           IARG_THREAD_ID,
                           IARG_END);
            break;
          case 0xF4: // stop
            INS_InsertCall(ins, 
                           IPOINT_BEFORE, 
                           (AFUNPTR)deactivate_counting,
                           IARG_THREAD_ID,
                           IARG_END);
            break;
          case 0xF5: // emit
            INS_InsertCall(ins, 
                           IPOINT_BEFORE, 
                           (AFUNPTR)emit_stats,
                           IARG_THREAD_ID,
                           IARG_END);
            break;
          case 0xF6: // zero
            INS_InsertCall(ins, 
                           IPOINT_BEFORE, 
                           (AFUNPTR)zero_stats,
                           IARG_THREAD_ID,
                           IARG_END);
            break;
          default:
            break;
        }
    }
}

/* ===================================================================== */

VOID Trace(TRACE trace, VOID *v)
{
    static UINT32 basic_blocks = 0;


    
    const BOOL accurate_handling_of_predicates = KnobProfilePredicated.Value();
    ADDRINT pc = TRACE_Address(trace);
    ADDRINT start_pc = pc;

    UINT32 new_blocks = 0;
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
    {
        const INS head = BBL_InsHead(bbl);
        if (! INS_Valid(head)) continue;
        new_blocks++;
    }


    TRACE_InsertCall(trace,
                     IPOINT_BEFORE,
                     AFUNPTR(validate_bbl_count), 
                     IARG_THREAD_ID,
                     IARG_UINT32,
                     basic_blocks+new_blocks,
                     IARG_END);

    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
    {
        const INS head = BBL_InsHead(bbl);
        if (! INS_Valid(head)) continue;

        // Summarize the stats for the bbl in a 0 terminated list
        // This is done at instrumentation time
        const UINT32 n = IndexStringLength(bbl, 1);

        // stats is an array of index types. We later multiply it by the
        // dynamic count for a block.
        stat_index_t *const stats = new stat_index_t[ n + 1];
        stat_index_t *const stats_end = stats + (n + 1);
        stat_index_t *curr = stats;
        UINT32 ninsts = 0;
        for (INS ins = head; INS_Valid(ins); ins = INS_Next(ins))
        {
            unsigned int instruction_size = INS_Size(ins);
#if !defined(TARGET_IPF)
            // This checks for x86-specific opcodes and fails on some IA-64 architecture
            // machines where the itext is not readable.
            CheckForSpecialMarkers(ins, pc, instruction_size);
#endif

            // Count the number of times a predicated instruction is actually executed
            // this is expensive and hence disabled by default
            if( INS_IsPredicated(ins) && accurate_handling_of_predicates )
            {
                INS_InsertPredicatedCall(ins,
                                         IPOINT_BEFORE,
                                         AFUNPTR(docount_predicated_true),
                                         IARG_UINT32,
                                         INS_GetIndex(ins),
                                         IARG_THREAD_ID,
                                         IARG_END);    
            }


            if (KnobMapToFile) {
                INT32 line;
                string filename;
                PIN_GetSourceLocation(pc, NULL, &line, &filename);
                if (!filename.empty())
                    *out << "MAPADDR 0x" << hex << pc << " " << dec << line << " " << filename << endl;
            }
            curr = INS_GenerateIndexString(ins,curr,1);
            if (measurement == measure_opcode)
                curr = INS_GenerateIndexFMA(ins,curr);
            pc = pc + instruction_size;
            ninsts++;
        }
        
        // stats terminator
        *curr++ = 0;
        ASSERTX( curr == stats_end );

        // Insert instrumentation to count the number of times the bbl is executed
        BBLSTATS * bblstats = new BBLSTATS(stats, start_pc, ninsts, pc-start_pc);
        INS_InsertCall(head,
                       IPOINT_BEFORE, 
                       AFUNPTR(docount_bbl), 
                       IARG_FAST_ANALYSIS_CALL,
                       IARG_UINT32,
                       basic_blocks,
                       IARG_THREAD_ID,
                       IARG_END);

        // Remember the counter and stats so we can compute a summary at the end
        basic_blocks++;
        GetLock(&bbl_list_lock,1);
        statsList.push_back(bblstats);
        ReleaseLock(&bbl_list_lock);
    }

}

/* ===================================================================== */
VOID DumpStats(ofstream& out,
               CSTATS& stats, 
               BOOL predicated_true,  
               const string& title, 
               THREADID tid)
{
    out << "#\n# " << title << "\n#\n";
    if (tid  != INVALID_THREADID)
        out << "# TID " << tid << "\n";
    out << "#       ";
    if (measurement == measure_opcode)
        out << "opcode";
    else if (measurement == measure_ilen)
        out << "inslen";
    else if (measurement == measure_category)
        out << "catgry";
    else if (measurement == measure_iform)
        out << "iform ";
    out<< "                 count-unpredicated    count-predicated";

    if( predicated_true )
        out << "    count-predicated-true";
    out << "\n#\n";

    // Compute the "total" bin. Stop at the INDEX_TOTAL for all histograms
    // except the iform. Iforms donot use the special rows, so we count everything.
    
    // build a map of the valid stats index values for all 3 tables.
    map<UINT32, bool> m;

    UINT32 tu=0, tp=0, tpt=0;
    for(stat_map_t::iterator it = stats.unpredicated.begin() ;  it != stats.unpredicated.end() ; it++) {
        if (measurement == measure_iform || it->first < INDEX_TOTAL)
            tu += it->second;
        m[it->first]=true;
    }
    for(stat_map_t::iterator it = stats.predicated.begin() ;  it != stats.predicated.end() ; it++) {
        if (measurement == measure_iform || it->first < INDEX_TOTAL)
            tp += it->second;
        m[it->first]=true;
    }
    for(stat_map_t::iterator it=stats.predicated_true.begin();it != stats.predicated_true.end() ; it++) {
        if (measurement == measure_iform || it->first < INDEX_TOTAL)
            tpt += it->second;
        m[it->first]=true;
    }

    for(map<UINT32,bool>::iterator it = m.begin(); it != m.end(); it++) {
        stat_map_t::iterator s;
        COUNTER up=0,pr=0,prt=0;
        UINT32 indx = it->first;

        s = stats.unpredicated.find(indx);
        if (s !=  stats.unpredicated.end())
            up = s->second;

        s = stats.predicated.find(indx);
        if (s !=  stats.predicated.end())
            pr = s->second;

        if (up == 0 && pr == 0)
            continue;

        out << setw(6) << indx << " " 
            << ljstr(IndexToString(indx),25) << " " 
            << setw(16) << up << " "
            << setw(16) << pr;
        if( predicated_true ) {
            s = stats.predicated_true.find(indx);
            prt = 0;
            if (s !=  stats.predicated_true.end())
                prt = s->second;
            out << " " << setw(16) << prt;
        }
        out << endl;
    }

    // print the totals
    out << setw(6) << "000000" << " " 
        << ljstr("*total",25) << " " 
        << setw(16) << tu << " "
        << setw(16) << tp;
    if( predicated_true ) 
        out << " " << setw(16) << tpt;
    out << endl;


}



/* ===================================================================== */
static UINT32 stat_dump_count = 0;


VOID emit_bbl_stats(THREADID tid)
{
    thread_data_t* tdata = get_tls(tid);
    // dynamic Counts 

    // Need to lock here because we might be resize (and thus reallocing)
    // the statsList when we do a push_back in the instrumentation.
    GetLock(&bbl_list_lock,tid+1);
    UINT32 limit = tdata->size();
    if ( limit  > statsList.size() )
        limit = statsList.size();
    for(UINT32 i=0;i< limit ; i++)
    {
        UINT32 bcount = tdata->block_counts[i];
        BBLSTATS* b = statsList[i];
        if (b && b->_stats)
            for (const stat_index_t* stats = b->_stats; *stats; stats++)
                tdata->cstats.unpredicated[*stats] += bcount;
    }
    ReleaseLock(&bbl_list_lock);

    GetLock(&lock, tid+1); // for output
    stat_dump_count++;
    *out << "# EMIT_STATS " << stat_dump_count << endl;
    DumpStats(*out, tdata->cstats, KnobProfilePredicated, "$dynamic-counts",tid);
    *out << "# END_STATS" <<  endl;
    ReleaseLock(&lock);
}

int qsort_compare_fn(const void *a, const void *b) 
{
    const BBL_SORT_STATS* ba = static_cast<const BBL_SORT_STATS*>(a);
    const BBL_SORT_STATS* bb = static_cast<const BBL_SORT_STATS*>(b);
    return (bb->_icount - ba->_icount); // descending sort
}

VOID emit_bbl_stats_sorted(THREADID tid)
{
    thread_data_t* tdata = get_tls(tid);
    // dynamic Counts 

    // Need to lock here because we might be resize (and thus reallocing)
    // the statsList when we do a push_back in the instrumentation.
    GetLock(&bbl_list_lock,tid+1);
    UINT32 limit = tdata->size();
    if ( limit  > statsList.size() )
        limit = statsList.size();
    BBL_SORT_STATS* icounts = new BBL_SORT_STATS[limit];
    UINT64 thread_total = 0;
    for(UINT32 i=0;i< limit ; i++)
    {
        BBLSTATS* b = statsList[i];
        if (b) {
            UINT32 bcount = tdata->block_counts[i];
            icounts[i]._icount = bcount * b->_ninst;
            icounts[i]._pc = b->_pc;
            icounts[i]._executions = bcount;
            icounts[i]._nbytes = b->_nbytes;
            thread_total += icounts[i]._icount;
        }
    }
    ReleaseLock(&bbl_list_lock);

    qsort(icounts, limit, sizeof(BBL_SORT_STATS), qsort_compare_fn);

    GetLock(&lock, tid+1); // for output
    *out << "# EMIT_STATS TOP BLOCKS " << stat_dump_count 
         << " FOR TID " << tid
         << endl;
    if (limit > KnobTopBlocks.Value())
        limit = KnobTopBlocks.Value(); 
    UINT64 t =0; 
    for(UINT32 i=0;i<limit;i++) {
        t+= icounts[i]._icount;
        *out << "BLOCK: " << setw(5) << i
             << "   PC: "
             << hex
             << setfill('0')
             << setw(sizeof(ADDRINT)*2) << icounts[i]._pc
             << setfill(' ')
             << dec
             << "   ICOUNT: "
             << setw(9) << icounts[i]._icount
             << "   EXECUTIONS: "
             << setw(9) << icounts[i]._executions
             << "   #BYTES: "
             << setw(2) << icounts[i]._nbytes
             << "   %: "
             << setw(5) << setprecision(3) << 100.0*icounts[i]._icount/thread_total
             << "   cumltv%: "
             << setw(5) << setprecision(3) << 100.0*t/thread_total
             << endl;
#if defined(TARGET_IA32) || defined(TARGET_IA32E)
        if (KnobShowDisassembly) {
            string s = disassemble(icounts[i]._pc, icounts[i]._pc + icounts[i]._nbytes);
            *out << s << endl;
        }
#endif

    }
    
    *out << "# END_STATS" <<  endl;
    ReleaseLock(&lock);
    delete [] icounts;
}

VOID emit_static_stats()
{
    *out << "# EMIT_STATIC_STATS " << stat_dump_count << endl;
    DumpStats(*out, GlobalStatsStatic, false, "$static-counts",INVALID_THREADID);
    *out << endl << "# END_STATIC_STATS" <<  endl;
}

VOID emit_pc_stats(THREADID tid)
{
    thread_data_t* tdata = get_tls(tid);
    // dynamic Counts 

    // Need to lock here because we might be resize (and thus reallocing)
    // the statsList when we do a push_back in the instrumentation.

    GetLock(&lock, tid+1); // for output
    *out << "# EMIT_PC_STATS for TID "  << tid << endl;
    GetLock(&bbl_list_lock,tid+1);
    UINT32 limit = tdata->size();
    if ( limit  > statsList.size() )
        limit = statsList.size();
    for(UINT32 i=0;i< limit ; i++)
    {
        UINT32 bcount = tdata->block_counts[i];
        BBLSTATS* b = statsList[i];
        if (bcount && b && b->_stats) 
            *out << "BLOCKCOUNT 0x" << hex << b->_pc  << " " << dec << (bcount * b->_ninst ) << endl;
    }
    ReleaseLock(&bbl_list_lock);
    *out << "# END_EMIT_PC_STATS for TID "  << tid << endl;
    ReleaseLock(&lock);
}
VOID emit_stats(THREADID tid)
{
    emit_bbl_stats(tid);
    if (KnobMapToFile)
        emit_pc_stats(tid);
}

/* ===================================================================== */

void combine_dynamic_stats(unsigned int numThreads)
{
    // combine all the rows from each thread in to the total variable.
    CSTATS total;
    for (THREADID i=0;i<numThreads; i++)
    {
        thread_data_t* tdata = get_tls(i);

        for(stat_map_t::iterator it = tdata->cstats.unpredicated.begin(); it != tdata->cstats.unpredicated.end() ; it++) {
            stat_map_t::iterator x = total.unpredicated.find(it->first);
            if (x == total.unpredicated.end())
                total.unpredicated[it->first] = it->second;
            else
                x->second += it->second;
        }

        for(stat_map_t::iterator it = tdata->cstats.predicated.begin(); it != tdata->cstats.predicated.end() ; it++) {
            stat_map_t::iterator x = total.predicated.find(it->first);
            if (x == total.predicated.end())
                total.predicated[it->first] = it->second;
            else
                x->second += it->second;
        }


        for(stat_map_t::iterator it = tdata->cstats.predicated_true.begin(); it != tdata->cstats.predicated_true.end() ; it++) {
            stat_map_t::iterator x = total.predicated_true.find(it->first);
            if (x == total.predicated_true.end())
                total.predicated_true[it->first] = it->second;
            else
                x->second += it->second;
        }
    }

    *out << "# EMIT_GLOBAL_DYNAMIC_STATS " << stat_dump_count << endl;
    DumpStats(*out, total, false, "$global-dynamic-counts",INVALID_THREADID);
    *out << endl << "# END_GLOBAL_DYNAMIC_STATS" <<  endl;
    
}

VOID Fini(int, VOID * v) // only runs once for the application
{
    *out << "# FINI: end of program" << endl;
    for(unsigned int i=0;i<numThreads;i++) {
        emit_stats(i);
        emit_bbl_stats_sorted(i);
    }
    emit_static_stats();
    combine_dynamic_stats(numThreads);

    out->close();
}


/* ===================================================================== */


#if defined(TARGET_IA32) || defined(TARGET_IA32E)
/////////////////////////////////////////////////////////////////////////
// Add a disassembler
/////////////////////////////////////////////////////////////////////////

static char nibble_to_ascii_hex(UINT8 i) {
    if (i<10) return i+'0';
    if (i<16) return i-10+'A';
    return '?';
}

static void print_hex_line(char* buf, const UINT8* array, const int length) {
  int n = length;
  int i=0;
  if (length == 0)
      n = XED_MAX_INSTRUCTION_BYTES;
  for( i=0 ; i< n; i++)     {
      buf[2*i+0] = nibble_to_ascii_hex(array[i]>>4);
      buf[2*i+1] = nibble_to_ascii_hex(array[i]&0xF);
  }
  buf[2*i]=0;
}


static string
disassemble(UINT64 start, UINT64 stop) {
    UINT64 pc = start;
    xed_state_t dstate;
    xed_syntax_enum_t syntax = XED_SYNTAX_INTEL;
    xed_error_enum_t xed_error;
    xed_decoded_inst_t xedd;
    ostringstream os;
    if (sizeof(ADDRINT) == 4) 
        xed_state_init(&dstate,     
                       XED_MACHINE_MODE_LEGACY_32,
                       XED_ADDRESS_WIDTH_32b, 
                       XED_ADDRESS_WIDTH_32b);
    else
        xed_state_init(&dstate,
                       XED_MACHINE_MODE_LONG_64,
                       XED_ADDRESS_WIDTH_64b, 
                       XED_ADDRESS_WIDTH_64b);

    while( pc < stop ) {
        xed_decoded_inst_zero_set_mode(&xedd, &dstate);
        UINT32 len = 15;
        if (stop - pc < 15)
            len = stop-pc;

        xed_error = xed_decode(&xedd, reinterpret_cast<const UINT8*>(pc), len);
        bool okay = (xed_error == XED_ERROR_NONE);
        iostream::fmtflags fmt = os.flags();
        os << std::setfill('0')
           << "XDIS "
           << std::hex
           << std::setw(sizeof(ADDRINT)*2)
           << pc
           << std::dec
           << ": "
           << std::setfill(' ')
           << std::setw(4);

        if (okay) {
            char buffer[200];
            unsigned int dec_len, sp;

            os << xed_extension_enum_t2str(xed_decoded_inst_get_extension(&xedd));
            dec_len = xed_decoded_inst_get_length(&xedd);
            print_hex_line(buffer, reinterpret_cast<UINT8*>(pc), dec_len);
            os << " " << buffer;
            for ( sp=dec_len; sp < 12; sp++)     // pad out the instruction bytes
                os << "  ";
            os << " ";
            memset(buffer,0,200);
            int dis_okay = xed_format(syntax, &xedd, buffer, 200, pc);
            if (dis_okay) 
                os << buffer << endl;
            else
                os << "Error disasassembling pc 0x" << std::hex << pc << std::dec;
            pc += dec_len;
        }
        else { // print the byte and keep going.
            UINT8 memval = *reinterpret_cast<UINT8*>(pc);
            os << "???? " // no extension
               << std::hex
               << std::setw(2)
               << std::setfill('0')
               << static_cast<UINT32>(memval)
               << std::endl;
            pc += 1;
        }
        os.flags(fmt);
    }
    return os.str();
}
#endif
    
/* ===================================================================== */

int main(int argc, CHAR **argv)
{
    if( PIN_Init(argc,argv) )
        return Usage();

    // obtain  a key for TLS storage
    tls_key = PIN_CreateThreadDataKey(0);

    string filename =  KnobOutputFile.Value();
    if( KnobPid )
        filename += "." + decstr( getpid_portable() );
    out = new std::ofstream(filename.c_str());

    control.CheckKnobs(Handler, 0);
    control_stats.CheckKnobs(HandlerStats, 0);

    // make sure that exactly one thing-to-count knob is specified.
    if (KnobInstructionLengthMix.Value() && KnobCategoryMix.Value()) {
        cerr << "Must have at most  one of: -iform, -ilen or -category "
             << "as a pintool option" << endl;
        exit(1);
    }
    if (KnobInstructionLengthMix.Value())
        measurement = measure_ilen;
    if (KnobCategoryMix.Value())
        measurement = measure_category;
    if (KnobIformMix.Value()) {
#if defined(TARGET_IA32) || defined(TARGET_IA32E)
        measurement = measure_iform;
#else
        cerr << "Cannot only compute iform mixes on IA32 and Intel64" << endl;
#endif
    }

    
    TRACE_AddInstrumentFunction(Trace, 0);
    PIN_AddThreadStartFunction(ThreadStart, 0);
    PIN_AddFiniFunction(Fini, 0);


    PIN_StartProgram();    // Never returns
    return 0;
#if defined(NEED_TO_PAD)
    (void) pad0; //pacify compiler 
    (void) pad1;
    (void) pad2;
#endif
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
