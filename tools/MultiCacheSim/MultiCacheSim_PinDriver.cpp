#include "pin.H"

#include <signal.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <string.h>

#include "MultiCacheSim.h"

MultiCacheSim *The_Cache;
  
KNOB<unsigned int> KnobCacheSize(KNOB_MODE_WRITEONCE, "pintool",
			   "csize", "65536", "Cache Size");//default cache is 64KB

KNOB<unsigned int> KnobBlockSize(KNOB_MODE_WRITEONCE, "pintool",
			   "bsize", "64", "Block Size");//default block is 64B

KNOB<unsigned int> KnobAssoc(KNOB_MODE_WRITEONCE, "pintool",
			   "assoc", "2", "Associativity");//default associativity is 2-way

KNOB<unsigned int> KnobNumCaches(KNOB_MODE_WRITEONCE, "pintool",
			   "numcaches", "1", "Number of Caches to Simulate");

KNOB<string> KnobProtocol(KNOB_MODE_WRITEONCE, "pintool",
			   "proto", "MSI", "Cache Coherence Protocol To Simulate");


#define MAX_NTHREADS 64
unsigned long instrumentationStatus[MAX_NTHREADS];

enum MemOpType { MemRead = 0, MemWrite = 1 };

INT32 usage()
{
    cerr << "MultiCacheSim -- A Multiprocessor cache simulator with a pin frontend";
    cerr << KNOB_BASE::StringKnobSummary();
    cerr << endl;
    return -1;
}


VOID TurnInstrumentationOn(ADDRINT tid){
  instrumentationStatus[PIN_ThreadId()] = true; 
}

VOID TurnInstrumentationOff(ADDRINT tid){
  instrumentationStatus[PIN_ThreadId()] = false; 
}


VOID instrumentRoutine(RTN rtn, VOID *v){
    
  if(strstr(RTN_Name(rtn).c_str(),"INSTRUMENT_OFF")){
    RTN_Open(rtn);
    RTN_InsertCall(rtn, 
                   IPOINT_BEFORE, 
                   (AFUNPTR)TurnInstrumentationOff, 
                   IARG_THREAD_ID,
                   IARG_END);
    RTN_Close(rtn);
  }
   
  
  if(strstr(RTN_Name(rtn).c_str(),"INSTRUMENT_ON")){
    RTN_Open(rtn);
    RTN_InsertCall(rtn, 
                   IPOINT_BEFORE, 
                   (AFUNPTR)TurnInstrumentationOn, 
                   IARG_THREAD_ID,
                   IARG_END);
    RTN_Close(rtn);
  }

}

VOID instrumentImage(IMG img, VOID *v)
{

}

void Read(THREADID tid, ADDRINT addr, ADDRINT inst){
  The_Cache->readLine(tid,inst,addr);
}

void Write(THREADID tid, ADDRINT addr, ADDRINT inst){
  The_Cache->writeLine(tid,inst,addr);
}

VOID instrumentTrace(TRACE trace, VOID *v)
{

  //BOOL ignore = false;
  RTN rtn = TRACE_Rtn(trace);

//  if(RTN_Valid(rtn)) {
//    LEVEL_CORE::IMG_TYPE imgType = IMG_Type(SEC_Img(RTN_Sec(rtn)));
//    ignore = !((imgType == IMG_TYPE_STATIC) || (imgType == IMG_TYPE_SHARED));
//  }

  for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl)) {
    for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins)) {  
      if(INS_IsMemoryRead(ins)) {
	  INS_InsertCall(ins, 
			 IPOINT_BEFORE, 
			 (AFUNPTR)Read, 
			 IARG_THREAD_ID,
			 IARG_MEMORYREAD_EA,
			 IARG_INST_PTR,
			 IARG_END);
      } else if(INS_IsMemoryWrite(ins)) {
	  INS_InsertCall(ins, 
			 IPOINT_BEFORE, 
			 (AFUNPTR)Write, 
			 IARG_THREAD_ID,//thread id
			 IARG_MEMORYWRITE_EA,//address being accessed
			 IARG_INST_PTR,//instruction address of write
			 IARG_END);
      }
    }
  }
}


VOID threadBegin(UINT32 threadid, VOID * sp, int flags, VOID *v)
{
  
}
    
VOID threadEnd(UINT32 threadid, INT32 code, VOID *v)
{

}

VOID dumpInfo(){
}


VOID Fini(INT32 code, VOID *v)
{
  The_Cache->dumpStatsForAllCaches();
}

BOOL segvHandler(THREADID threadid,INT32 sig,CONTEXT *ctx,BOOL hasHndlr,VOID*v){
  return TRUE;//let the program's handler run too
}

BOOL termHandler(THREADID threadid,INT32 sig,CONTEXT *ctx,BOOL hasHndlr,VOID*v){
  return TRUE;//let the program's handler run too
}


int main(int argc, char *argv[])
{
  PIN_InitSymbols();
  if( PIN_Init(argc,argv) ) {
    return usage();
  }
  
  for(int i = 0; i < MAX_NTHREADS; i++){
    instrumentationStatus[i] = true;
  }

  unsigned long csize = KnobCacheSize.Value();
  unsigned long bsize = KnobBlockSize.Value();
  unsigned long assoc = KnobAssoc.Value();
  unsigned long num = KnobNumCaches.Value();
  CoherenceProtocol proto;

  if(!strcmp(KnobProtocol.Value().c_str(),"MESI")){
    proto = PROTO_MESI;
  }else{//default to MSI
    proto = PROTO_MSI;
  }

  The_Cache = new MultiCacheSim(stdout, csize, assoc, bsize, proto);
  for(unsigned int i = 0; i < num; i++){
    The_Cache->createNewCache();
  } 

  RTN_AddInstrumentFunction(instrumentRoutine,0);
  IMG_AddInstrumentFunction(instrumentImage, 0);
  TRACE_AddInstrumentFunction(instrumentTrace, 0);

  PIN_AddSignalInterceptFunction(SIGTERM,termHandler,0);
  PIN_AddSignalInterceptFunction(SIGSEGV,segvHandler,0);

  PIN_AddThreadBeginFunction(threadBegin, 0);
  PIN_AddThreadEndFunction(threadEnd, 0);
  PIN_AddFiniFunction(Fini, 0);
 
  PIN_StartProgram();
  
  return 0;
}
