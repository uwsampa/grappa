
#undef DEBUG

#include <llvm/Support/Debug.h>
#include <llvm/IR/Constants.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/IRBuilder.h>

#include "Passes.h"
#include "DelegateExtractor.hpp"

using namespace llvm;

namespace Grappa {
  
  bool ExtractorPass::runOnModule(Module& M) {
    outs() << "Running extractor...\n";
    bool changed = false;
    
//    if (! ginfo.init(M) ) return false;
    ginfo.init(M);
    
    //////////////////////////
    // Find 'task' functions
    for (auto& F : M) {
      if (F.hasFnAttribute("async")) {
        task_fns.insert(&F);
      }
    }
    
    outs() << "task_fns.count => " << task_fns.size() << "\n";
    
    std::vector<DelegateExtractor*> candidates;
    
    for (auto fn : task_fns) {
      
      auto& provenance = getAnalysis<ProvenanceProp>(*fn);
//      provenance.viewGraph(fn);
      provenance.prettyPrint(*fn);
      
      // auto dex = new DelegateExtractor(M, ginfo);
      
      
      
    }
    
    return changed;
  }
    
  bool ExtractorPass::doInitialization(Module& M) {
    outs() << "-- Grappa Extractor --\n";
    outs().flush();
    return false;
  }
  
  bool ExtractorPass::doFinalization(Module& M) {
    outs().flush();
    return true;
  }
  
  char ExtractorPass::ID = 0;
  
  //////////////////////////////
  // Register optional pass
  static RegisterPass<ExtractorPass> X( "grappa-ex", "Grappa Extractor", false, false );
  
}
