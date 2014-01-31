
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
    bool changed = false;
    
    if (! ginfo.init(M) ) return false;
    
    //////////////////////////
    // Find 'task' functions
    auto global_annos = M.getNamedGlobal("llvm.global.annotations");
    if (global_annos) {
      auto a = cast<ConstantArray>(global_annos->getOperand(0));
      for (int i=0; i<a->getNumOperands(); i++) {
        auto e = cast<ConstantStruct>(a->getOperand(i));
        
        if (auto fn = dyn_cast<Function>(e->getOperand(0)->getOperand(0))) {
          auto anno = cast<ConstantDataArray>(cast<GlobalVariable>(e->getOperand(1)->getOperand(0))->getOperand(0))->getAsCString();
          
          if (anno == "async") { task_fns.insert(fn); }
        }
      }
    }
    
    std::vector<DelegateExtractor*> candidates;
    
    for (auto fn : task_fns) {
      
      auto dex = new DelegateExtractor(M, ginfo);
      
      
      
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
