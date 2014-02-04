
#undef DEBUG

#include <llvm/Support/Debug.h>
#include <llvm/Pass.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Constants.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/IRBuilder.h>

#include "DelegateExtractor.hpp"

using namespace llvm;

namespace {
  
  struct PrePass : public FunctionPass {
    static char ID;
    
    PrePass() : FunctionPass(ID) { }
    
    virtual bool runOnFunction(Function &F) {
      bool changed = false;
      
      for (auto inst = inst_begin(&F); inst != inst_end(&F); inst++) {
        if (auto call = dyn_cast<CallInst>(&*inst)) {
          if (call->getNumArgOperands() > 0) {
            auto arg = call->getArgOperand(0);
            if (auto cast = dyn_cast<AddrSpaceCastInst>(arg)) {
              if (cast->getSrcTy()->getPointerAddressSpace() == SYMMETRIC_SPACE) {
                errs() << "!! found method call on symmetric*\n";
                call->setIsNoInline();
                changed = true;
              }
            }
          }
        }
      }
      
      return changed;
    }
    
    virtual bool doInitialization(Module& M) {
      outs() << "-- Grappa Pre Pass --\n";
      outs().flush();
      
      if (auto fn = M.getFunction("printf")) {
        fn->addFnAttr("unbound");
      }
      
      //////////////////////////////////////////
      // Add annotations as function attributes
      auto global_annos = M.getNamedGlobal("llvm.global.annotations");
      if (global_annos) {
        auto a = cast<ConstantArray>(global_annos->getOperand(0));
        for (int i=0; i<a->getNumOperands(); i++) {
          auto e = cast<ConstantStruct>(a->getOperand(i));
          
          if (auto fn = dyn_cast<Function>(e->getOperand(0)->getOperand(0))) {
            auto anno = cast<ConstantDataArray>(cast<GlobalVariable>(e->getOperand(1)->getOperand(0))->getOperand(0))->getAsCString();
            
            fn->addFnAttr(anno);
          }
        }
      }
      
      return false;
    }
    
    virtual bool doFinalization(Module &M) {
      outs().flush();
      return true;
    }
    
  };
  
  char PrePass::ID = 0;
  
  //////////////////////////////
  // Register optional pass
  static RegisterPass<PrePass> X( "grappa-pre", "Grappa Pre Pass", false, false );
  
  //////////////////////////////
  // Register as default pass
  static RegisterStandardPasses PassRegistration(PassManagerBuilder::EP_EarlyAsPossible,
  [](const PassManagerBuilder&, PassManagerBase& PM) {
    outs() << "Registered Grappa Pre-Pass!\n";
    PM.add(new PrePass());
  });
  
}
