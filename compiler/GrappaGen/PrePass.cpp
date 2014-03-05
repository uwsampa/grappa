
#undef DEBUG

#include <llvm/Support/Debug.h>
#include <llvm/Pass.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Constants.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/CallSite.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Support/CommandLine.h>

#include "DelegateExtractor.hpp"

using namespace llvm;

static cl::opt<bool> InlineSpecialMethods("grappa-inline-methods",
                                          cl::desc("Allow methods on global*/symmetric* to be inlined (means they might get split up)."));

namespace {
  
  struct PrePass : public FunctionPass {
    static char ID;
    
    PrePass() : FunctionPass(ID) { }
    
    virtual bool runOnFunction(Function &F) {
      auto& M = *F.getParent();
      auto layout = new DataLayout(&M);
      bool changed = false;
      
      SmallVector<Instruction*, 8> to_remove;
      
      auto md = M.getOrInsertNamedMetadata("grappa.asyncs");
      
      for (auto inst = inst_begin(&F); inst != inst_end(&F); inst++) {
        if (auto call = dyn_cast<CallInst>(&*inst)) {
          if (call->getNumArgOperands() > 0) {
            auto arg = call->getArgOperand(0);
            if (auto c = dyn_cast<AddrSpaceCastInst>(arg)) {
              auto space = c->getSrcTy()->getPointerAddressSpace();
              if (space == SYMMETRIC_SPACE || space == GLOBAL_SPACE) {
                DEBUG(outs() << "!! found method call on symmetric*\n");
                if (!InlineSpecialMethods)
                  call->setIsNoInline();
                changed = true;
              }
            }
          }
          
          auto cf = call->getCalledFunction();
          if (cf && cf->getName() == "grappa_noop_gce") {
            auto gce = call->getOperand(0);
            if (isa<GlobalVariable>(gce)) {
              md->addOperand(MDNode::get(F.getContext(), {&F, gce}));
              F.setName("async");
            } else {
              assert(isa<Constant>(gce));
            }
            
            to_remove.push_back(call);
          }
        }
      }
      
      for (auto inst : to_remove) inst->eraseFromParent();
      
      return changed;
    }
    
    virtual bool doInitialization(Module& M) {
      outs() << "-- Grappa Pre Pass --\n";
      outs().flush();
      
      for (auto name : (StringRef[]){ "printf", "fprintf" })
        if (auto fn = M.getFunction(name))
          fn->addFnAttr("unbound");
      
//      for (auto& F : M.getFunctionList()) {
//        if (F.getName().find("google10LogMessage") != std::string::npos) {
//          F.addFnAttr("unbound");
//        }
//      }
      
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
    DEBUG(outs() << "Registered Grappa Pre-Pass!\n");
    PM.add(new PrePass());
  });
  
}
