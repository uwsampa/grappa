
#undef DEBUG

//#ifdef DEBUG
////#define _DEBUG
//#undef DEBUG
//#endif

#include <llvm/Support/Debug.h>
#include <llvm/Pass.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Constants.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/DebugLoc.h>
#include <llvm/IR/Instructions.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/ADT/Twine.h>
#include <llvm/Support/Debug.h>
#include <llvm/PassManager.h>
#include <llvm/Support/CFG.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/IR/DataLayout.h>

#include <llvm/Transforms/Utils/CodeExtractor.h>
//#include "MyCodeExtractor.h"

#include <llvm/InstVisitor.h>

#include "DelegateExtractor.hpp"

#include <sstream>
#include <set>
#include <map>
#include <queue>
#include <list>

//// I'm not sure why, but I can't seem to enable debug output in the usual way
//// (with command line flags). Here's a hack.
//#undef DEBUG
//#define DEBUG(s) s

using namespace llvm;

//STATISTIC(grappaGlobalRefs, "calls to Grappa's distributed shared memory system");

namespace {
  
  struct GrappaEarly : public FunctionPass {
    static char ID;
    
    GrappaEarly() : FunctionPass(ID) { }
    
    virtual bool runOnFunction(Function &F) {
      bool changed = false;
      
      return changed;
    }
    
    virtual bool doInitialization(Module& module) {
      outs() << "-- Grappa Early Pass:\n";
      DEBUG( errs() << "  * debug on\n" );
      
      return false;
    }
    
    virtual bool doFinalization(Module &M) {
      outs().flush();
      return true;
    }
    
    virtual void getAnalysisUsage(AnalysisUsage& AU) const {}
    
  };
  
  char GrappaEarly::ID = 0;
  
  //////////////////////////////
  // Register optional pass
  static RegisterPass<GrappaEarly> X(
                                   "grappa-early", "Grappa Pre-pass",
                                   false, false
                                   );
  
//  //////////////////////////////
//  // Register as default pass
//  static void registerPass(const PassManagerBuilder&, PassManagerBase& PM) {
//    fprintf(stderr, "Registered GrappaGen pass!\n");
//    PM.add(new GrappaEarly());
//  }
//  static RegisterStandardPasses GrappaGenRegistration(PassManagerBuilder::EP_EarlyAsPossible, registerPass);

  static RegisterStandardPasses GrappaEarlyRegistration(PassManagerBuilder::EP_EarlyAsPossible,
    [](const PassManagerBuilder&, PassManagerBase& PM) {
      fprintf(stderr, "Registered GrappaEarly pass!\n");
      PM.add(new GrappaEarly());
    }
  );
  
}
