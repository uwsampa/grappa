#include "utilities.h"
#include "memlog.h"

/* ------------------------------------------------------------------ */
/* Global Variables                                                   */
/* ------------------------------------------------------------------ */

CONTEXT ctxt;          // saved architectural state
MEMLOG* memlog;        // saved memory state
BOOL verbose;          // whether to output trace file
fstream tracefile;     // trace file
fstream ctxtfile;      // file with the saved architectural state
fstream memfile;       // file with the saved memory state
BOOL started;          // whether we have jumped to our saved context
UINT32 rtn_count;      // number of routines executed since started

/* ------------------------------------------------------------------ */
/* Function Declarations                                              */
/* ------------------------------------------------------------------ */

VOID Init(UINT32, char**);
VOID Fini(INT32, VOID*);
VOID FlagIns(INS, VOID*);
VOID FlagRtn(RTN, VOID*);
VOID StartCheckpoint(const string*);
VOID CountRtn(const string*);

/* ================================================================== */

int main(UINT32 argc, char** argv) 
{
    Init(argc, argv);
    PIN_InitSymbols();
    PIN_Init(argc, argv);
    INS_AddInstrumentFunction(FlagIns, 0);
    RTN_AddInstrumentFunction(FlagRtn, 0);
    PIN_AddFiniFunction(Fini, 0);
    PIN_StartProgram();
    return 0;
}

/* ------------------------------------------------------------------ */
/* Initalization & Clean Up                                           */
/* ------------------------------------------------------------------ */

VOID Init(UINT32 argc, char** argv) 
{
    started = false;
    rtn_count = 0;
    GetArg(argc, argv, "-traceon", verbose);
    memlog = new MEMLOG(verbose);
    if (verbose) 
    {
        tracefile.open("checkpoint.txt", fstream::out | fstream::app);
    }
    ctxtfile.open("ctxtsave.txt", fstream::in);
    memfile.open("memsave.txt", fstream::in);
}

VOID Fini(INT32 code, VOID* v) 
{
    ASSERTX(rtn_count > 0);
    if (verbose) 
    {
        std::cout << "# Routines Executed: " << rtn_count << "\n" << flush;
        tracefile.close();
    }
    ctxtfile.close();
    memfile.close();
    delete memlog;
}

/* ------------------------------------------------------------------ */
/* Instrumentation Routines                                           */
/* ------------------------------------------------------------------ */

VOID FlagRtn(RTN rtn, VOID* v) 
{
    RTN_Open(rtn);
    RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)CountRtn,
                   IARG_PTR, new string(RTN_Name(rtn)),
                   IARG_END);
    RTN_Close(rtn);
}

VOID FlagIns(INS ins, VOID* v) 
{
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)StartCheckpoint,
                   IARG_PTR, new string(INS_Disassemble(ins)),
                   IARG_END);
}

/* ------------------------------------------------------------------ */
/* StartCheckpoint:                                                   */
/* ------------------------------------------------------------------ */

VOID StartCheckpoint(const string* disassm)
{
    if (!started) 
    {
        for (REG reg = REG_PHYSICAL_CONTEXT_BEGIN; reg <= REG_PHYSICAL_CONTEXT_END; reg = REG(reg + 1)) 
        {
            ADDRINT val;
            ctxtfile >> hex >> val;
            PIN_SetContextReg(&ctxt, reg, val);
        }
        memlog->InitWriteLog(memfile, tracefile);
        memlog->RestoreMem(tracefile);
        started = true;
        PIN_ExecuteAt(&ctxt);
    }
}

/* ------------------------------------------------------------------ */
/* CountRtn:                                                          */
/* ------------------------------------------------------------------ */

VOID CountRtn(const string* rtn_name) 
{
    if (started) 
    {
        rtn_count++;
        if (verbose) 
        {
            std::cout << *rtn_name << "\n" << flush;
        }
    }
}

/* ------------------------------------------------------------------ */
