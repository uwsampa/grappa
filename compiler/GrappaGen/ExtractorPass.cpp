
#undef DEBUG

#include <llvm/Support/Debug.h>
#include <llvm/IR/Constants.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/IRBuilder.h>

#include "Passes.h"
#include "DelegateExtractor.hpp"

using namespace llvm;

namespace Grappa {
  
  Value* search(Value* v) {
    if (auto gep = dyn_cast<GetElementPtrInst>(v)) {
      if (gep->hasIndices()) {
        auto idx = gep->getOperand(1);
        if (idx != ConstantInt::get(idx->getType(), 0)) {
          return gep;
        }
      }
      return search(gep->getPointerOperand());
    }
    if (auto c = dyn_cast<CastInst>(v)) {
      auto vv = search(c->getOperand(0));
      if (isa<PointerType>(vv->getType()))
        return vv;
      else
        return v;
    }
    return v;
  }
  
  void analyze(Function& fn) {
    auto& ctx = fn.getContext();
    for (auto& bb: fn) {
      for (auto& i : bb) {
        Value *prov = nullptr;
        if (auto l = dyn_cast<LoadInst>(&i)) {
          prov = search(l->getPointerOperand());
        } else if (auto s = dyn_cast<StoreInst>(&i)) {
          prov = search(s->getPointerOperand());
        }
        if (prov) {
          i.setMetadata("grappa.prov", MDNode::get(ctx, prov));
        }
      }
    }
    
  }
  
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
      
//      auto& provenance = getAnalysis<ProvenanceProp>(*fn);
//      provenance.prettyPrint(*fn);
      
//      auto dex = new DelegateExtractor(M, ginfo);
      
      analyze(*fn);
      
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
