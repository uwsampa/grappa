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
#include <stdio.h>
#include "pin.H"
#include "instlib.H"
#include "portability.H"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <cstdlib>
#if defined( __GNUC__)

//typedef unsigned short UINT16;
typedef int8_t  INT8;
typedef int16_t INT16;
typedef int64_t INT64;

#define ALIGN16 __attribute__ ((aligned(16)))
#define ALIGN8  __attribute__ ((aligned(8)))

#elif defined(_MSC_VER)

/*
typedef unsigned __int8 UINT8 ;
typedef unsigned __int16 UINT16;
typedef unsigned __int32 UINT32;
typedef unsigned __int64 UINT64;

typedef __int8 INT8;
typedef __int16 INT16;
typedef __int32 INT32;
typedef __int64 INT64;
*/

#define ALIGN16 __declspec(align(16))
#define ALIGN8  __declspec(align(8))

#else
#error Expect usage of either GNU or MS compiler.
#endif
FILE  *log_inl;
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE,         "pintool",
    "o", "verify_fp_context.out", "output file");

/*
KNOB<UINT32> KnobLastInsToInstrument(KNOB_MODE_WRITEONCE, "pintool",
    "last_ins", "1000000", "LastInsToInstrument");
KNOB<UINT32> KnobFirstInsToInstrument(KNOB_MODE_WRITEONCE, "pintool",
    "first_ins", "0", "LastInsToInstrument");
*/

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

    /*while( pc < stop )*/ {
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
                os << "Error disasassembling pc 0x" << std::hex << pc << std::dec << endl;
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


BOOL CompareIntContext (CONTEXT *context1, CONTEXT *context2)
{
    BOOL compareOk = TRUE;
    if (PIN_GetContextReg( context1, REG_INST_PTR ) != PIN_GetContextReg( context2, REG_INST_PTR ))
    {
        fprintf (log_inl,"REG_INST_PTR ERROR\n");
        compareOk = FALSE;
    }
    if (PIN_GetContextReg( context1, REG_GAX )  != PIN_GetContextReg( context2, REG_GAX ))
    {
        fprintf (log_inl,"REG_GAX ERROR\n");
        compareOk = FALSE;
    }
    if (PIN_GetContextReg( context1, REG_GBX )  != PIN_GetContextReg( context2, REG_GBX ))
    {
        fprintf (log_inl,"REG_GBX ERROR\n");
        compareOk = FALSE;
    }
    if (PIN_GetContextReg( context1, REG_GCX )  != PIN_GetContextReg( context2, REG_GCX ))
    {
        fprintf (log_inl,"REG_GCX ERROR\n");
        compareOk = FALSE;
    }
    if (PIN_GetContextReg( context1, REG_GDX )  != PIN_GetContextReg( context2, REG_GDX ))
    {
        fprintf (log_inl,"REG_GDX ERROR\n");
        compareOk = FALSE;
    }
    if (PIN_GetContextReg( context1, REG_GSI )  != PIN_GetContextReg( context2, REG_GSI ))
    {
        fprintf (log_inl,"REG_GSI ERROR\n");
        compareOk = FALSE;
    }
    if (PIN_GetContextReg( context1, REG_GDI )  != PIN_GetContextReg( context2, REG_GDI ))
    {
        fprintf (log_inl,"REG_GDI ERROR\n");
        compareOk = FALSE;
    }
    if (PIN_GetContextReg( context1, REG_GBP )  != PIN_GetContextReg( context2, REG_GBP ))
    {
        fprintf (log_inl,"REG_GBP ERROR\n");
        compareOk = FALSE;
    }
    if (PIN_GetContextReg( context1, REG_STACK_PTR )  != PIN_GetContextReg( context2, REG_STACK_PTR ))
    {
        fprintf (log_inl,"REG_STACK_PTR ERROR\n");
        compareOk = FALSE;
    }
    if (PIN_GetContextReg( context1, REG_SEG_SS ) != PIN_GetContextReg( context2, REG_SEG_SS ))
    {
        fprintf (log_inl,"REG_SEG_SS ERROR\n");
        compareOk = FALSE;
    }
    if (PIN_GetContextReg( context1, REG_SEG_CS ) != PIN_GetContextReg( context2, REG_SEG_CS ))
    {
        fprintf (log_inl,"REG_SEG_CS ERROR\n");
        compareOk = FALSE;
    }
    if (PIN_GetContextReg( context1, REG_SEG_DS ) != PIN_GetContextReg( context2, REG_SEG_DS ))
    {
        fprintf (log_inl,"REG_SEG_DS ERROR\n");
        compareOk = FALSE;
    }
    if (PIN_GetContextReg( context1, REG_SEG_ES ) != PIN_GetContextReg( context2, REG_SEG_ES ))
    {
        fprintf (log_inl,"REG_SEG_ES ERROR\n");
        compareOk = FALSE;
    }
    if (PIN_GetContextReg( context1, REG_SEG_FS ) != PIN_GetContextReg( context2, REG_SEG_FS ))
    {
        fprintf (log_inl,"REG_SEG_FS ERROR\n");
        compareOk = FALSE;
    }
    if (PIN_GetContextReg( context1, REG_SEG_GS ) != PIN_GetContextReg( context2, REG_SEG_GS ))
    {
        fprintf (log_inl,"REG_SEG_GS ERROR\n");
        compareOk = FALSE;
    }
    if (PIN_GetContextReg( context1, REG_GFLAGS ) != PIN_GetContextReg( context2, REG_GFLAGS ))
    {
        fprintf (log_inl,"REG_GFLAGS ERROR\n");
        compareOk = FALSE;
    }
#ifdef TARGET_IA32E
    if (PIN_GetContextReg( context1, REG_R8 ) != PIN_GetContextReg( context2, REG_R8 ))
    {
        fprintf (log_inl,"REG_R8 ERROR\n");
        compareOk = FALSE;
    }
    if (PIN_GetContextReg( context1, REG_R9 ) != PIN_GetContextReg( context2, REG_R9 ))
    {
        fprintf (log_inl,"REG_R9 ERROR\n");
        compareOk = FALSE;
    }
    if (PIN_GetContextReg( context1, REG_R10 ) != PIN_GetContextReg( context2, REG_R10 ))
    {
        fprintf (log_inl,"REG_R10 ERROR\n");
        compareOk = FALSE;
    }
    if (PIN_GetContextReg( context1, REG_R11 ) != PIN_GetContextReg( context2, REG_R11 ))
    {
        fprintf (log_inl,"REG_R11 ERROR\n");
        compareOk = FALSE;
    }
    if (PIN_GetContextReg( context1, REG_R12 ) != PIN_GetContextReg( context2, REG_R12 ))
    {
        fprintf (log_inl,"REG_R12 ERROR\n");
        compareOk = FALSE;
    }
    if (PIN_GetContextReg( context1, REG_R13 ) != PIN_GetContextReg( context2, REG_R13 ))
    {
        fprintf (log_inl,"REG_R13 ERROR\n");
        compareOk = FALSE;
    }
    if (PIN_GetContextReg( context1, REG_R14 ) != PIN_GetContextReg( context2, REG_R14 ))
    {
        fprintf (log_inl,"REG_R14 ERROR\n");
        compareOk = FALSE;
    }
    if (PIN_GetContextReg( context1, REG_R15 ) != PIN_GetContextReg( context2, REG_R15 ))
    {
        fprintf (log_inl,"REG_R15 ERROR\n");
        compareOk = FALSE;
    }
#endif
    return (compareOk);
}




BOOL CompareFpContext(FPSTATE  *fpContextFromPin, FPSTATE *fpContextFromXsave, BOOL compareYmm)
{
    BOOL compareOk = TRUE;
    
    if ( fpContextFromPin->fxsave_legacy._fcw != fpContextFromXsave->fxsave_legacy._fcw)
    {
        fprintf (log_inl,"fcw ERROR\n");
        compareOk = FALSE;
    }
    if ( fpContextFromPin->fxsave_legacy._fsw != fpContextFromXsave->fxsave_legacy._fsw)
    {
        fprintf (log_inl,"_fsw ERROR\n");
        compareOk = FALSE;
    }
    if ( fpContextFromPin->fxsave_legacy._ftw != fpContextFromXsave->fxsave_legacy._ftw)
    {
        fprintf (log_inl,"_ftw ERROR\n");
        compareOk = FALSE;
    }
    if ( fpContextFromPin->fxsave_legacy._fop != fpContextFromXsave->fxsave_legacy._fop)
    {
        fprintf (log_inl,"_fop ERROR\n");
        compareOk = FALSE;
    }
    if ( fpContextFromPin->fxsave_legacy._fpuip != fpContextFromXsave->fxsave_legacy._fpuip)
    {
        fprintf (log_inl,"_fpuip ERROR\n");
        compareOk = FALSE;
    }
    /* the _cs field seems to be changing randomly when running 32on64 linux
       needs further investigation to prove it is not a Pin bug */
    if ( fpContextFromPin->fxsave_legacy._cs != fpContextFromXsave->fxsave_legacy._cs)
    {
        fprintf (log_inl,"_cs ERROR\n");
        compareOk = FALSE;
    } 
    if ( fpContextFromPin->fxsave_legacy._fpudp != fpContextFromXsave->fxsave_legacy._fpudp)
    {
        fprintf (log_inl,"_fpudp ERROR\n");
        compareOk = FALSE;
    }
    /* the _ds field seems to be changing randomly when running 32on64 linux
       needs further investigation to prove it is not a Pin bug */
    if ( fpContextFromPin->fxsave_legacy._ds != fpContextFromXsave->fxsave_legacy._ds)
    {
        fprintf (log_inl,"_ds ERROR\n");
        compareOk = FALSE;
    }  
    if ( fpContextFromPin->fxsave_legacy._mxcsr != fpContextFromXsave->fxsave_legacy._mxcsr)
    {
        fprintf (log_inl,"_mxcsr ERROR\n");
        compareOk = FALSE;
    }
    if ( fpContextFromPin->fxsave_legacy._mxcsrmask != fpContextFromXsave->fxsave_legacy._mxcsrmask)
    {
        fprintf (log_inl,"_mxcsrmask ERROR\n");
        compareOk = FALSE;
    }
    int i;
    for (i=0; i< 8*16; i++)
    {
        if ( fpContextFromPin->fxsave_legacy._st[i] != fpContextFromXsave->fxsave_legacy._st[i])
        {
            fprintf (log_inl,"_st[%d] ERROR\n", i);
            compareOk = FALSE;
        }
    }
    for (i=0; 
#ifdef TARGET_IA32E
        i< 16*16;
#else
        i< 8*16; 
#endif
        i++)
    {
        if ( fpContextFromPin->fxsave_legacy._xmm[i] != fpContextFromXsave->fxsave_legacy._xmm[i])
        {
            fprintf (log_inl,"_xmm[%d] ERROR\n", i);
            compareOk = FALSE;
        }
    }

    if (compareYmm)
    {
    for (i=0; 
#ifdef TARGET_IA32E
        i< 16*16;
#else
        i< 8*16; 
#endif
        i++)
    {
        if ( fpContextFromPin->_xstate._ymmUpper[i] != fpContextFromXsave->_xstate._ymmUpper[i])
        {
            fprintf (log_inl,"_xstate._ymmUpper[%d] ERROR\n", i);
            compareOk = FALSE;
        }
    }
    }


    return (compareOk);
}

extern "C" void Do_Xsave(FPSTATE *xsaveContext);
extern "C" void Do_Fxsave(FPSTATE *xsaveContext);
CONTEXT contextFromPin;
CHAR fpContextSpaceForXsave[FPSTATE_SIZE+FPSTATE_ALIGNMENT];
CHAR fpContextSpaceForFxsave[FPSTATE_SIZE+FPSTATE_ALIGNMENT];
CHAR fpContextSpaceForFpConextFromPin[FPSTATE_SIZE+FPSTATE_ALIGNMENT];
CHAR fpContextSpaceForFpConextFromPin1[FPSTATE_SIZE+FPSTATE_ALIGNMENT];

FPSTATE *fpContextFromXsave;
FPSTATE *fpContextFromFxsave;  



VOID VerifyFpContext(ADDRINT pcval, CONTEXT * context)
{
    
    
    
    //printf ("fpContextFromXsave %x FPSTATE_SIZE %d FPSTATE_ALIGNMENT %d\n", fpContextFromXsave, FPSTATE_SIZE, FPSTATE_ALIGNMENT);
    //fflush (stdout);
    Do_Xsave (fpContextFromXsave);

    
    //printf ("fpContextFromFxsave %x\n", fpContextFromFxsave);
    //fflush (stdout);
    Do_Fxsave (fpContextFromFxsave);
    

    PIN_SaveContext(context, &contextFromPin); 
    
    FPSTATE *fpContextFromPin = 
        reinterpret_cast<FPSTATE *>
        (( reinterpret_cast<ADDRINT>(fpContextSpaceForFpConextFromPin) + (FPSTATE_ALIGNMENT - 1)) & (-1*FPSTATE_ALIGNMENT));
    unsigned char * ptr = (reinterpret_cast< unsigned char *>(fpContextFromPin))+ sizeof (FXSAVE);
    // set values after fxsave part of fp context - to verify that the deprecated call to PIN_GetContextFPState does NOT change these
    memset (ptr, 0xa5, sizeof(FPSTATE) - sizeof (FXSAVE));
    PIN_GetContextFPState(&contextFromPin, reinterpret_cast<void *>(fpContextFromPin));

    for (int i=0; i<sizeof(FPSTATE) - sizeof (FXSAVE); i++,ptr++)
    {
        if (*ptr != 0xa5)
        {
            printf ("**** ERROR: value set after FXSAVE part in deprecated PIN_GetContextFPState *ptr = %x  (i %d)\n", *ptr, i);
            exit (-1);
        }
    }

    PIN_GetContextFPState(&contextFromPin, fpContextFromPin);
    ptr = (reinterpret_cast< unsigned char *>(fpContextFromPin))+ sizeof (FXSAVE);

    FPSTATE *fpContextFromPin1 = 
        reinterpret_cast<FPSTATE *>
        (( reinterpret_cast<ADDRINT>(fpContextSpaceForFpConextFromPin1) + (FPSTATE_ALIGNMENT - 1)) & (-1*FPSTATE_ALIGNMENT));
    
    PIN_GetContextFPState(&contextFromPin, fpContextFromPin1);
    // set values after fxsave part of fp context - to verify that the deprecated call to PIN_SetContextFPState does NOT change these
    unsigned char * ptr1 = (reinterpret_cast< unsigned char *>(fpContextFromPin1)) + sizeof (FXSAVE);
    memset (ptr1, 0xa5, sizeof(FPSTATE) - sizeof (FXSAVE));
    PIN_SetContextFPState(&contextFromPin, reinterpret_cast<const void *>(fpContextFromPin1));
    PIN_GetContextFPState(&contextFromPin, fpContextFromPin1);
    if (memcmp (ptr1, ptr, sizeof(FPSTATE) - sizeof (FXSAVE)) != 0)
    {
        printf ("**** ERROR: value set after FXSAVE part in deprecated PIN_SetContextFPState\n");
        exit (-1);
    }

    if (!CompareFpContext (fpContextFromPin, fpContextFromFxsave, FALSE))
    {
        fprintf (log_inl, "***ERROR in fxsave fp context\n");
        printf ("***ERROR in fxsave fp context see file %s\n",  KnobOutputFile.Value().c_str());
        fflush (stdout);
        string s = disassemble ((pcval),(pcval)+15);
        fprintf (log_inl,"    %s\n", s.c_str());
        exit (-1);
    }
}






int numInstruction = 0;
VOID Instruction(INS ins, VOID *v)
{
    INT32 xedEtension = INS_Extension(ins);
    if (xedEtension==XED_EXTENSION_AVX ||
        xedEtension==XED_EXTENSION_SSE ||
        xedEtension==XED_EXTENSION_SSE2 ||
        xedEtension==XED_EXTENSION_SSE3 ||
        xedEtension==XED_EXTENSION_SSE4 ||
        xedEtension==XED_EXTENSION_SSE4A ||
        xedEtension==XED_EXTENSION_SSSE3 ||
        xedEtension==XED_EXTENSION_X87
        )
    {
        numInstruction++;
        xed_iclass_enum_t iclass = (xed_iclass_enum_t) INS_Opcode(ins);
        //if (numInstruction<=1)
        {
        //printf ("InstrumentingX# %d:  IP: %x   instruction: %s\n", numInstruction, INS_Address(ins), INS_Disassemble(ins).c_str());
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)VerifyFpContext,
                   IARG_INST_PTR,
                   IARG_CONTEXT,
                   IARG_END);
        INS_InsertCall(ins, IPOINT_AFTER, (AFUNPTR)VerifyFpContext,
                   IARG_INST_PTR,
                   IARG_CONTEXT,
                   IARG_END);
        }
    }
}

VOID Fini(INT32 code, VOID *v)
{
    fprintf (log_inl,  "SUCCESS\n");
    fclose(log_inl);
}

int main(int argc, char *argv[])
{
    
    if( PIN_Init(argc,argv) )
    {
        return (-1);
    }

    
    fpContextFromXsave = 
            reinterpret_cast<FPSTATE *>
        (( reinterpret_cast<ADDRINT>(fpContextSpaceForXsave) + (FPSTATE_ALIGNMENT - 1)) & (-1*FPSTATE_ALIGNMENT));
    memset (fpContextFromXsave, 0, sizeof (FPSTATE));

    fpContextFromFxsave =   
        reinterpret_cast<FPSTATE *>
        (( reinterpret_cast<ADDRINT>(fpContextSpaceForFxsave) + (FPSTATE_ALIGNMENT - 1)) & (-1*FPSTATE_ALIGNMENT));

    log_inl = fopen( KnobOutputFile.Value().c_str(), "w");

    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);

    // Never returns
    PIN_StartProgram();
    
    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
