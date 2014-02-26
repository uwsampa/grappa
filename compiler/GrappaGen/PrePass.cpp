
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
  
  void remap(Instruction* inst, ValueToValueMapTy& vmap) {
    Instruction* to_delete = nullptr;
    
    RemapInstruction(inst, vmap, RF_IgnoreMissingEntries);
    
    if (auto bc = dyn_cast<BitCastInst>(inst)) {
      if (bc->getType()->getPointerAddressSpace() !=
          bc->getSrcTy()->getPointerAddressSpace()) {
        auto new_bc = new BitCastInst(bc->getOperand(0),
                            PointerType::get(
                              bc->getType()->getPointerElementType(),
                              bc->getSrcTy()->getPointerAddressSpace()
                            ),
                          inst->getName()+".fix",
                          inst);
        vmap[bc] = new_bc;
        to_delete = bc;
      }
    } else if (auto gep = dyn_cast<GetElementPtrInst>(inst)) {
      if (gep->getPointerAddressSpace() != gep->getType()->getPointerAddressSpace()) {
        SmallVector<Value*,4> idxs;
        for (auto i=1; i<gep->getNumOperands(); i++) idxs.push_back(gep->getOperand(i));
        auto new_gep = GetElementPtrInst::Create(gep->getPointerOperand(), idxs,
                                             gep->getName()+"fix", gep);
        if (gep->isInBounds()) new_gep->setIsInBounds();
        vmap[gep] = new_gep;
        to_delete = gep;
      }
    }
    
    SmallVector<User*, 32> uses(inst->use_begin(), inst->use_end());
    for (auto u : uses) {
      if (auto iu = dyn_cast<Instruction>(u)) {
        remap(iu, vmap);
      }
    }
    
    if (to_delete) to_delete->eraseFromParent();
  }
  
  struct PrePass : public FunctionPass {
    static char ID;
    
    PrePass() : FunctionPass(ID) { }
    
    virtual bool runOnFunction(Function &F) {
      auto& M = *F.getParent();
      auto layout = new DataLayout(&M);
      bool changed = false;
      
      SmallVector<Instruction*, 8> to_remove;
      
      auto md = M.getOrInsertNamedMetadata("grappa.asyncs");
      
      SmallVector<AddrSpaceCastInst*,32> casts;
      SmallVector<CallInst*,32> calls;
      
      for (auto inst = inst_begin(&F); inst != inst_end(&F); inst++) {
        if (auto call = dyn_cast<CallInst>(&*inst)) {
          if (call->getNumArgOperands() > 0) {
            auto arg = call->getArgOperand(0);
            if (auto c = dyn_cast<AddrSpaceCastInst>(arg)) {
              auto space = c->getSrcTy()->getPointerAddressSpace();
              if (space == SYMMETRIC_SPACE) { //  || space == GLOBAL_SPACE) {
                DEBUG(outs() << "!! found method call on symmetric*\n");
                if (!InlineSpecialMethods)
                  call->setIsNoInline();
                changed = true;
              } else if (space == GLOBAL_SPACE) {
                casts.push_back(c);
              }
            }
          }
          
          auto cf = call->getCalledFunction();
          if (cf && cf->getName() == "grappa_noop_gce") {
            auto gce = call->getOperand(0);
            if (isa<GlobalVariable>(gce)) {
              md->addOperand(MDNode::get(F.getContext(), {&F, gce}));
            } else {
              assert(isa<Constant>(gce));
            }
            
            to_remove.push_back(call);
          }
        }
      }
      
      ValueToValueMapTy vmap;
      struct MyTypeMapper : public ValueMapTypeRemapper {
        Type* remapType(Type *srcTy) override {
          if (auto ptrTy = dyn_cast<PointerType>(srcTy)) {
            return PointerType::get(ptrTy->getElementType(), GLOBAL_SPACE);
          } else {
            return srcTy;
          }
        }
      } tmap;

      for (auto c : casts) {
        outs() << "~~~~~~~~~~~\n" << *c << "\n";
        auto ptr = c->getOperand(0);
        auto src_space = c->getSrcTy()->getPointerAddressSpace();
        auto src_elt_ty = c->getSrcTy()->getPointerElementType();
        auto dst_elt_ty = c->getType()->getPointerElementType();
        if (src_elt_ty != dst_elt_ty) {
          ptr = new BitCastInst(c->getOperand(0),
                                PointerType::get(dst_elt_ty, src_space),
                                c->getName()+".precast", c);
          c->setOperand(0, ptr);
          outs() << "****" << *ptr << "\n****" << *c << "\n";
        }
        
        SmallVector<User*, 32> us(c->use_begin(), c->use_end());
        for (auto u : us) {
          outs() << "--" << *u << "\n";
          if (auto call = dyn_cast<CallInst>(u)) {
            auto called_fn = call->getCalledFunction();
            if (!called_fn) continue;
            vmap[c] = ptr;
//            RemapInstruction(call, vmap, RF_IgnoreMissingEntries); //, &tmap);
//            call->replaceUsesOfWith(c, ptr);
            outs() << "++" << *call << "\n";
            calls.push_back(call);
          }
        }
        
//        int uses = 0;
//        for_each_use(u, *c) {
//          uses++;
//          errs() << "!!" << **u << "\n";
//        }
//        assert(uses == 0);
//        c->eraseFromParent();
      }
      
      for (auto call : calls) {
        InlineFunctionInfo info(nullptr, layout);
        auto inlined = InlineFunction(call, info);
        outs() << "------------------------\n" << *call->getCalledFunction() << "\n";
        assert(inlined);
      }
      
      for (auto c : casts) {
        remap(c, vmap);
        c->eraseFromParent();
      }
      
//      for (auto& BB : F) {
////        SmallVector<BitCastInst*,8> bcs;
//        for (auto& I : BB) {
//          remap(&I, vmap);
////          if (auto bc = dyn_cast<BitCastInst>(&I)) {
////            if (bc->getType()->getPointerAddressSpace() !=
////                bc->getSrcTy()->getPointerAddressSpace()) {
////              bcs.push_back(bc);
////            }
////          }
//        }
////        for (auto bc : bcs) {
////          errs() << "fixing =>" << *bc << "\n";
////          RemapInstruction(bc, vmap, RF_IgnoreMissingEntries); //, &tmap);
//////
//////          auto new_bc = new BitCastInst(bc->getOperand(0),
//////                              PointerType::get(
//////                                bc->getType()->getPointerElementType(),
//////                                bc->getSrcTy()->getPointerAddressSpace()
//////                              )
//////                            );
//////          bc->replaceAllUsesWith(new_bc);
//////          bc->eraseFromParent();
////        }
//      }
      
      if (casts.size()) {
        outs() << "^^^^^^^^^^^^^^^^^^^^^^^^^" << F << "\n";
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
