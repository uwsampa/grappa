#ifdef DEBUG
//#define _DEBUG
#undef DEBUG
#endif

#define DEBUG_TYPE "grappa_gen"
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
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Utils/CodeExtractor.h>
#include <llvm/Analysis/Verifier.h>

#include <llvmUtil.h>
#include <llvmGlobalToWide.h>

// TODO: make shared header that both can include
#define GLOBAL_SPACE 100

#include <sstream>
#include <set>
#include <map>

// I'm not sure why, but I can't seem to enable debug output in the usual way
// (with command line flags). Here's a hack.
#undef DEBUG
#define DEBUG(s) s

#define dump_var_l(before, dumpee, after) \
errs() << before << #dumpee << ": "; \
dumpee->dump(); \
errs() << after

#define dump_var(dumpee) dump_var_l("", dumpee, "\n")

using namespace llvm;

namespace {
  
  template< typename T >
  void ir_comment(T* target, StringRef label, const Twine& text) {
    auto& ctx = target->getContext();
    target->setMetadata(label, MDNode::get(ctx, MDString::get(ctx, text.str())));
  }

  template< typename T >
  void ir_comment(T* target, StringRef label, const DebugLoc& dloc) {
    auto& ctx = target->getContext();
    target->setMetadata(label, dloc.getAsMDNode(ctx));
  }
  
  struct GrappaWrapper : public ModulePass {
    static char ID;
    
    ModulePass* global_to_wide_pass;
    GlobalToWideInfo * wide_info;
    
    GrappaWrapper() : ModulePass(ID) { }
    ~GrappaWrapper() { }
    
    virtual bool runOnModule(Module& module) {
      bool changed = false;
      
      changed |= global_to_wide_pass->runOnModule(module);
      
      verifyModule(module);
      
//      outs() << "------------------------\n";
//      for (auto& fn : module) {
//        for (auto& bb : fn) {
//          for (auto& inst : bb) {
//            if (auto call = dyn_cast<CallInst>(&inst)) {
//              auto fn = call->getCalledFunction();
//              if (fn && call->getCalledFunction()->getName().startswith(".gf.")) {
//                outs() << " global_fn => " << *call->getCalledFunction() << "\n";
//              }
//            }
//          }
//        }
//      }
      outs().flush();
      errs() << "^^^^^^^^^^^^^^^^^^^^^^^^\n";
      return changed;
    }
    
    virtual bool doInitialization(Module& module) {
      errs() << "initializing\n";
      
      wide_info = new GlobalToWideInfo();
      auto& info = *wide_info;
      
      auto get_fn = [&module,&info](StringRef name) {
        auto fn = module.getFunction(name);
        if (!fn) {
          llvm::errs() << "unable to find " << name << "\n";
          abort();
        }
        // info.specialFunctions.insert(fn);
        return fn;
      };
      auto& ctx = module.getContext();

      info.localeIdType = Type::getInt16Ty(ctx);
      info.nodeIdType = Type::getInt16Ty(ctx);
      info.globalSpace = GLOBAL_SPACE;
      info.wideSpace = GLOBAL_SPACE + 1;
      
      GlobalPointerInfo ptr_info;
//      ptr_info.wideTy = Type::getInt64Ty(ctx);
      ptr_info.wideTy = Type::getInt8PtrTy(ctx);
      outs() << "@bh wideTy: " << *ptr_info.wideTy << "\n";
      
//      ptr_info.globalToWideFn = get_fn("grappa_global_to_wide_void");
//      ptr_info.wideToGlobalFn = get_fn("grappa_wide_to_global_void");
//      outs() << "@bh wideToGlobalFn: " << *ptr_info.wideToGlobalFn->getType() << "\n";
      
      info.addrFn = get_fn("grappa_wide_get_pointer");
      info.nodeFn = get_fn("grappa_wide_get_core");
      info.locFn = get_fn("grappa_wide_get_locale_core");
      info.makeFn = get_fn("grappa_make_wide");
//      info.addrFn = ptr_info.addrFn = get_fn("grappa_wide_get_pointer");
//      info.nodeFn = ptr_info.locFn = get_fn("grappa_wide_get_core");
//      info.locFn = ptr_info.nodeFn = get_fn("grappa_wide_get_locale_core");
//      info.makeFn = ptr_info.makeFn = get_fn("grappa_make_wide");
      
      outs() << "makeFn: " << *info.makeFn->getType() << "\n";
      
//      info.gTypes[Type::getVoidTy(ctx)] = ptr_info;
      info.gTypes[Type::getInt8PtrTy(ctx, GLOBAL_SPACE)] = ptr_info;
      info.gTypes[Type::getInt64PtrTy(ctx, GLOBAL_SPACE)] = ptr_info;
      
      info.getFn = get_fn("grappa_get");
//      info.getFn = module.getOrInsertFunction("grappa_get", Type::getVoidTy(ctx),
//                                              Type::getInt8PtrTy(ctx), ptr_info.wideTy,
//                                              Type::getInt64Ty(ctx), Type::getInt64Ty(ctx),
//                                              nullptr);
//      
      outs() << "@bh getFn:         " << *info.getFn->getType() << "\n";
//      outs() << "@bh get or insert: " << *getFn->getType() << "\n";
//      outs() << "----------\n" << *getFn << "-------------\n";
      
      info.putFn = get_fn("grappa_put");
      info.getPutFn = get_fn("grappa_getput_void");
      info.memsetFn = get_fn("grappa_memset_void");
      
      global_to_wide_pass = createGlobalToWide(&info, "");
      return false;
    }
    
    virtual bool doFinalization(Module &M) {
      return true;
    }
    
  };
  
  char GrappaWrapper::ID = 0;
  
  //////////////////////////////
  // Register optional pass
  static RegisterPass<GrappaWrapper> X("grappa", "Grappa Code Generator",
                                   false, false
                                   );
  
  //////////////////////////////
  // Register as default pass
  static void registerGrappaGen(const PassManagerBuilder&, PassManagerBase& PM) {
    fprintf(stderr, "Registered Grappa Global-To-Wide Wrapper.\n");
    PM.add(new GrappaWrapper());
  }
  static RegisterStandardPasses GrappaGenRegistration(PassManagerBuilder::EP_ScalarOptimizerLate, registerGrappaGen);
  
}
