/* ================================================================== */
/* Transactional Memory Pin Tool                                      */
/* ------------------------------------------------------------------ */
/*                                                                    */
/* functionality:                                                     */
/* a simple transactional memory model                                */
/*                                                                    */
/* commands line options:                                             */
/* -traceon           enables the log file of memory tracing          */
/* -tiebreaker <INT>  decides which transaction wins when conflicting */
/*                    (see PickSuccessfulTransaction() for detail)    */
/*                                                                    */
/* ================================================================== */

#include "utilities.h"
#include "memlog.h"
#include <map>

/* ------------------------------------------------------------------ */
/* Global Type(s)                                                     */
/* ------------------------------------------------------------------ */

class THREADSTATE 
{
  public:
    THREADSTATE(BOOL verbose) :
        intransaction(false),
        aborting(false)
        {
            memlog = new MEMLOG(verbose);
        }
    ~THREADSTATE() { delete memlog; }
    BOOL intransaction;
    BOOL aborting;
    CHECKPOINT chkpt;
    MEMLOG* memlog;
};

/* ------------------------------------------------------------------ */
/* Global Variables                                                   */
/* ------------------------------------------------------------------ */

map<UINT32, THREADSTATE*> threadstates;  // tracks the state of the threads
BOOL verbose;                            // whether to output trace file
fstream tracefile;                       // trace file
UINT32 tiebreaker;                       // scheme to decide which transaction will win
UINT32 ntransactions;                    // number of threads currently in a transaction

/* ------------------------------------------------------------------ */
/* Function Declarations                                              */
/* ------------------------------------------------------------------ */

VOID Init(UINT32, char**);
VOID Fini(INT32, VOID*);
VOID ThreadBegin(UINT32, VOID*, int, VOID*);
VOID ThreadEnd(UINT32, INT32, VOID*);
VOID FlagXRtn(RTN, VOID*);
VOID FlagMemIns(INS, VOID*);
VOID ProcessMemIns(BOOL, BOOL, BOOL, ADDRINT, UINT32, ADDRINT, ADDRINT, UINT32, UINT32);
VOID BeginTransaction(UINT32, CHECKPOINT*);
VOID CommitTransaction(UINT32);
VOID AbortTransaction(UINT32);
VOID AbortTransactionBegin(THREADSTATE*);
VOID AbortTransactionEnd(THREADSTATE*);
UINT32 PickSuccessfulTransaction(UINT32, set<UINT32>, BOOL);
VOID LogInsTrace(UINT32, ADDRINT, ADDRINT, const string*);

/* ================================================================== */

int main(UINT32 argc, char** argv) 
{
    Init(argc, argv);
    PIN_InitSymbols();
    PIN_Init(argc, argv);
    PIN_AddThreadBeginFunction(ThreadBegin, 0);
    PIN_AddThreadEndFunction(ThreadEnd, 0);
    RTN_AddInstrumentFunction(FlagXRtn, 0);
    INS_AddInstrumentFunction(FlagMemIns, 0);
    PIN_AddFiniFunction(Fini, 0);
    PIN_StartProgram();
    return 0;
}

/* ------------------------------------------------------------------ */
/* Initalization & Clean Up                                           */
/* ------------------------------------------------------------------ */

VOID Init(UINT32 argc, char** argv) 
{
    GetArg(argc, argv, "-traceon", verbose);
    ThreadBegin(0, NULL, 0, NULL);
    if (verbose) 
    {
        tracefile.open("transactionalmem.txt", fstream::out | fstream::app);
    }
    GetArg(argc, argv, "-tiebreaker", tiebreaker, 1);
    ntransactions = 0;
}

VOID Fini(INT32 code, VOID* v) 
{
    ThreadEnd(0, 0, NULL);
    if (verbose) 
    {
        tracefile.close();
    }
}

/* ------------------------------------------------------------------ */
/* Initialization and Clean Up of Thread State                        */
/* ------------------------------------------------------------------ */

VOID ThreadBegin(THREADID thread_id, VOID* sp, int flags, VOID* v) 
{
    std::cout << "Thread " << thread_id << " begins.\n" << flush;
    ASSERTX(threadstates.find(thread_id) == threadstates.end());
    threadstates[thread_id] = new THREADSTATE(verbose);
}

VOID ThreadEnd(THREADID thread_id, INT32 code, VOID* v) 
{
    std::cout << "Thread " << thread_id << " ends.\n" << flush;
    map<UINT32, THREADSTATE*>::iterator threadstate_ptr = threadstates.find(thread_id);
    ASSERTX(threadstate_ptr != threadstates.end());
    delete threadstate_ptr->second;
    threadstates.erase(threadstate_ptr);
}

/* ------------------------------------------------------------------ */
/* Instrumentation Routines                                           */
/* ------------------------------------------------------------------ */

VOID FlagXRtn(RTN rtn, VOID* v) 
{
    RTN_Open(rtn);
    const string* rtn_name = new string(RTN_Name(rtn));
    if (rtn_name->find("XBEGIN") != string::npos) 
    {
        RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)BeginTransaction,
                       IARG_THREAD_ID,
                       IARG_CHECKPOINT,
                       IARG_END);
    }
    else if (rtn_name->find("XEND") != string::npos) 
    {
        RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)CommitTransaction,
                       IARG_THREAD_ID,
                       IARG_END);
    }
    RTN_Close(rtn);
}

VOID FlagMemIns(INS ins, VOID* v) 
{
    if (INS_IsMemoryWrite(ins) || INS_IsMemoryRead(ins))
    {
        if (INS_IsMemoryWrite(ins) && INS_IsMemoryRead(ins) && INS_HasMemoryRead2(ins)) 
        {
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)ProcessMemIns,
                           IARG_BOOL, true, 
                           IARG_BOOL, true,
                           IARG_BOOL, true,
                           IARG_MEMORYWRITE_EA,
                           IARG_MEMORYWRITE_SIZE,
                           IARG_MEMORYREAD_EA,
                           IARG_MEMORYREAD2_EA,
                           IARG_MEMORYREAD_SIZE,
                           IARG_THREAD_ID,
                           IARG_END);
        }
        else if (INS_IsMemoryWrite(ins) && INS_IsMemoryRead(ins) && !INS_HasMemoryRead2(ins)) 
        {
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)ProcessMemIns,
                           IARG_BOOL, true, 
                           IARG_BOOL, true, 
                           IARG_BOOL, false,
                           IARG_MEMORYWRITE_EA,
                           IARG_MEMORYWRITE_SIZE,
                           IARG_MEMORYREAD_EA,
                           IARG_ADDRINT, 0,
                           IARG_MEMORYREAD_SIZE,
                           IARG_THREAD_ID,
                           IARG_END);
        }
        else if (INS_IsMemoryWrite(ins) && !INS_IsMemoryRead(ins)) 
        {
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)ProcessMemIns,
                           IARG_BOOL, true, 
                           IARG_BOOL, false,
                           IARG_BOOL, false,
                           IARG_MEMORYWRITE_EA,
                           IARG_MEMORYWRITE_SIZE,
                           IARG_ADDRINT, 0,
                           IARG_ADDRINT, 0,
                           IARG_UINT32, 0,
                           IARG_THREAD_ID,
                           IARG_END);
        }
        else if (!INS_IsMemoryWrite(ins) && INS_IsMemoryRead(ins) && INS_HasMemoryRead2(ins))
        {
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)ProcessMemIns,
                           IARG_BOOL, false, 
                           IARG_BOOL, true,
                           IARG_BOOL, true,
                           IARG_ADDRINT, 0,
                           IARG_UINT32, 0,
                           IARG_MEMORYREAD_EA,
                           IARG_MEMORYREAD2_EA,
                           IARG_MEMORYREAD_SIZE,
                           IARG_THREAD_ID,
                           IARG_END);
        }
        else if (!INS_IsMemoryWrite(ins) && INS_IsMemoryRead(ins) && !INS_HasMemoryRead2(ins)) 
        {
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)ProcessMemIns,
                           IARG_BOOL, false, 
                           IARG_BOOL, true,
                           IARG_BOOL, false,
                           IARG_ADDRINT, 0,
                           IARG_UINT32, 0,
                           IARG_MEMORYREAD_EA,
                           IARG_ADDRINT, 0,
                           IARG_MEMORYREAD_SIZE,
                           IARG_THREAD_ID,
                           IARG_END);
        }
        else 
        {
            ASSERTX(0);
        }
    }
    if (verbose) 
    {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)LogInsTrace,
                       IARG_THREAD_ID,
                       IARG_INST_PTR,
                       IARG_REG_VALUE, REG_STACK_PTR,
                       IARG_PTR, new string(INS_Disassemble(ins)),
                       IARG_END);
    }
}

/* ------------------------------------------------------------------ */
/* ProcessMemIns:                                                     */
/* meat of the transactional memory model: detects conflicts with     */
/* current transactions, aborts transaction(s) if necessary, and      */
/* updates transaction memory logs                                    */
/* ------------------------------------------------------------------ */

VOID ProcessMemIns(BOOL iswrite, BOOL isread, BOOL hasread2,
                   ADDRINT waddr, UINT32 wlen,
                   ADDRINT raddr, ADDRINT raddr2, UINT32 rlen,
                   THREADID thread_id) 
{
    PIN_LockClient();    
    if (ntransactions > 0)
    {
        map<UINT32, THREADSTATE*>::iterator threadstate_ptr = threadstates.find(thread_id);
        ASSERTX(threadstate_ptr != threadstates.end());
        THREADSTATE* threadstate = threadstate_ptr->second;
        // transaction aborting: memory already restored, but need to rollback execution
        if (threadstate->aborting) 
        {
            std::cout << "Thread " << thread_id << " " << flush;
            AbortTransactionEnd(threadstate);
        }
        else if ((ntransactions > 1) || (!(threadstate->intransaction))) 
        {
            // detect conflicts with other threads' transactions
            set<UINT32> conflicts;
            map<UINT32, THREADSTATE*>::iterator itr;
            for (itr = threadstates.begin(); itr != threadstates.end(); itr++) 
            {
                MEMLOG* memlog = (itr->second)->memlog;
                if ((itr->first != thread_id) &&
                    ((itr->second)->intransaction) &&
                    ((iswrite  && memlog->DetectConflict(waddr,  wlen, true, tracefile)) ||
                     (isread   && memlog->DetectConflict(raddr,  rlen, false, tracefile)) ||
                     (hasread2 && memlog->DetectConflict(raddr2, rlen, false, tracefile)))) 
                {
                    conflicts.insert(itr->first);
                }
            }
            // decide which thread(s) continue and which thread(s) abort (if any)
            UINT32 winner;
            if (!conflicts.empty()) 
            {
                winner = PickSuccessfulTransaction(thread_id, conflicts, threadstate->intransaction);
                std::cout << "Thread " << thread_id << " conflicts with thread(s): " << flush;
                set<UINT32>::iterator itr_conflicts;
                for (itr_conflicts = conflicts.begin(); itr_conflicts != conflicts.end(); itr_conflicts++) 
                {
                    std::cout << *itr_conflicts << " " << flush;
                }
                std::cout << "\n" << flush;
                // abort current thread's transaction
                if (winner != thread_id) 
                {
                    AbortTransaction(thread_id);
                }
                // abort conflicting transactions
                else 
                {
                    for (itr_conflicts = conflicts.begin(); itr_conflicts != conflicts.end(); itr_conflicts++) 
                    {
                        map<UINT32, THREADSTATE*>::iterator abortingthreadstate_ptr  = threadstates.find(*itr_conflicts);
                        ASSERTX(abortingthreadstate_ptr != threadstates.end());
                        THREADSTATE* abortingthreadstate = abortingthreadstate_ptr->second;
                        ASSERTX(abortingthreadstate->intransaction);
                        std::cout << "Thread " << abortingthreadstate_ptr->first << " " << flush;
                        AbortTransactionBegin(abortingthreadstate);
                    }
                }
                conflicts.clear();
            }
            else 
            {
                winner = thread_id;
            }
            // update current thread's transaction log if successful
            if ((winner == thread_id) && (threadstate->intransaction))
            {
                MEMLOG* memlog = threadstate->memlog;
                if (iswrite)  { memlog->RecordWrite(waddr, wlen, tracefile); }
                if (isread)   { memlog->RecordRead(raddr, rlen, tracefile); }
                if (hasread2) { memlog->RecordRead(raddr2, rlen, tracefile); }
            }
        }
        else 
        {
            // if current thread has the only transaction, skipped conflict detection code, but still need to update log
            ASSERTX((threadstate->intransaction) && (ntransactions == 1));
            MEMLOG* memlog = threadstate->memlog;
            if (iswrite)  { memlog->RecordWrite(waddr, wlen, tracefile); }
            if (isread)   { memlog->RecordRead(raddr, rlen, tracefile); }
            if (hasread2) { memlog->RecordRead(raddr2, rlen, tracefile); }
        }
    }
    PIN_UnlockClient();
}

/* ------------------------------------------------------------------ */
/* BeginTransaction:                                                  */
/* checkpoints the processor state and changes the thread's state to  */
/* indicate that it's inside a transaction                            */
/* ------------------------------------------------------------------ */

VOID BeginTransaction(THREADID thread_id, CHECKPOINT* _chkpt) 
{
    PIN_LockClient();
    std::cout << "Thread " << thread_id << " entering transaction.\n" << flush;
    if (verbose) 
    {
        tracefile << "Thread " << thread_id << " entering transaction.\n" << flush;
    }
    ntransactions++;
    map<UINT32, THREADSTATE*>::iterator threadstate_ptr = threadstates.find(thread_id);
    ASSERTX(threadstate_ptr != threadstates.end());
    THREADSTATE* threadstate = threadstate_ptr->second;
    threadstate->intransaction = true;
    PIN_SaveCheckpoint(_chkpt, &(threadstate->chkpt));
    PIN_UnlockClient();
}

/* ------------------------------------------------------------------ */
/* CommitTransaction:                                                 */
/* since the transaction has completed successful, we can trash the   */
/* the checkpointed processor state and memory log; also change the   */
/* thread's state to indicate that it's no longer inside a transaction*/
/* ------------------------------------------------------------------ */

VOID CommitTransaction(THREADID thread_id)
{
    PIN_LockClient();
    std::cout << "Thread " << thread_id << " committing transaction.\n" << flush;
    ntransactions--;
    map<UINT32, THREADSTATE*>::iterator threadstate_ptr = threadstates.find(thread_id);
    ASSERTX(threadstate_ptr != threadstates.end());
    THREADSTATE* threadstate = threadstate_ptr->second;
    threadstate->intransaction = false;
    (threadstate->memlog)->Reset();
    PIN_UnlockClient();
}

/* ------------------------------------------------------------------ */
/* AbortTransaction:                                                  */
/* transaction is aborted due to detected memory conflict             */
/* note, this is split into two parts because we cannot roll back an  */
/* aborted thread's execution if it is not inside the pin tool        */
/* currently, but we cannot release the current thread without first  */
/* undoing the aborted thread's shared memory changes; it's ok to     */
/* wait to roll back the aborted thread the next time we instrument   */
/* its load or store, b/c it has not changed any globally visible     */
/* in between                                                         */
/* ------------------------------------------------------------------ */

VOID AbortTransaction(THREADID thread_id) 
{
    std::cout << "Thread " << thread_id << " aborting transaction.\n" << flush;
    map<UINT32, THREADSTATE*>::iterator threadstate_ptr  = threadstates.find(thread_id);
    ASSERTX(threadstate_ptr != threadstates.end());
    THREADSTATE* threadstate = threadstate_ptr->second;
    ASSERTX(threadstate->intransaction);
    
    AbortTransactionBegin(threadstate);
    AbortTransactionEnd(threadstate);
}

/* ------------------------------------------------------------------ */
/* AbortTransactionBegin:                                             */
/* undo the aborted transaction's memory changes, and sets the        */
/* thread's state to 'aborting'                                       */
/* ------------------------------------------------------------------ */

VOID AbortTransactionBegin(THREADSTATE* threadstate)
{
    std::cout << "restoring memory (abort, part I).\n" << flush;
    (threadstate->memlog)->RestoreMem(tracefile);
    threadstate->aborting = true;
}

/* ------------------------------------------------------------------ */
/* AbortTransactionEnd:                                               */
/* roll back to the beginning of the transaction                      */
/* ------------------------------------------------------------------ */

VOID AbortTransactionEnd(THREADSTATE* threadstate)
{
    std::cout << "resuming (abort, part II).\n" << flush;
    ASSERTX(threadstate->aborting);
    threadstate->aborting = false;
    PIN_UnlockClient();
    PIN_Resume(&(threadstate->chkpt));
}

/* ------------------------------------------------------------------ */
/* PickSuccessfulTransaction:                                         */
/* very simple schemes to pick which transaction gets to proceed      */
/* in the case of conflicts                                           */
/* (1) always lets the non-transaction memop win over transactions    */
/* (2) between multiple transactions:                                 */
/* -tiebreaker 1    pick the smallest ID to win                       */
/* -tiebreaker 2    pick the largest ID to win                        */
/* ------------------------------------------------------------------ */

UINT32 PickSuccessfulTransaction(UINT32 current, set<UINT32> conflicts, BOOL current_in_transaction) 
{
    if (!current_in_transaction)
    {
        std::cout << "not in transaction!\n" << flush;
        return current;
    }
    ASSERTX(!conflicts.empty());
    ASSERTX(conflicts.find(current) == conflicts.end());
    switch(tiebreaker) 
    {
      case 1:
        return((current < *(conflicts.begin())) ? current : *(conflicts.begin()));
      case 2:
        return((current > *(conflicts.rbegin())) ? current : *(conflicts.begin()));
      default:
        ASSERTX(0);
        return 0;
    }
}

/* ------------------------------------------------------------------ */
/* LogInsTrace:                                                       */
/* records the instructions and some state (stack pointer and         */
/* the value at the top of the stack)                                 */
/* ------------------------------------------------------------------ */

VOID LogInsTrace(THREADID thread_id, ADDRINT ip, ADDRINT sp, const string* disassm) 
{
    tracefile << dec << thread_id << ":\t" << hex << ip << "\t" << sp << "\t" << flush;
    PrintHexWord(sp, tracefile);
    tracefile << "\t" << *disassm << "\n";
}

/* ------------------------------------------------------------------ */

