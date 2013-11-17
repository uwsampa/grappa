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
    
    GrappaWrapper() : ModulePass(ID) { }
    ~GrappaWrapper() { }
    
    virtual bool runOnModule(Module& module) {
      return global_to_wide_pass->runOnModule(module);
    }
    
    virtual bool doInitialization(Module& module) {
      errs() << "initializing\n";
      
      GlobalToWideInfo info;
      
      auto get_ty = [&module](StringRef name) {
        auto ty = module.getTypeByName(name);
        if (!ty) {
          llvm::errs() << "unable to find " << name << "\n";
          abort();
        }
        return ty;
      };
      
      auto get_fn = [&module,&info](StringRef name) {
        auto fn = module.getFunction(name);
        if (!fn) {
          llvm::errs() << "unable to find " << name << "\n";
          abort();
        }
        return fn;
        info.specialFunctions.insert(fn);
        return fn;
      };
      
      info.localeIdType = Type::getInt16Ty(module.getContext());
      info.nodeIdType = Type::getInt16Ty(module.getContext());
      info.globalSpace = GLOBAL_SPACE;
      info.wideSpace = GLOBAL_SPACE + 1;
      
      GlobalPointerInfo ptr_info;
      ptr_info.wideTy = get_ty("struct.GlobalAddressBase");
      ptr_info.globalToWideFn = get_fn("grappa_global_to_wide_void");
      ptr_info.wideToGlobalFn = get_fn("grappa_wide_to_global_void");
      
      info.addrFn = ptr_info.addrFn = get_fn("grappa_pointer");
      info.locFn = ptr_info.locFn = get_fn("grappa_core");
      info.nodeFn = ptr_info.nodeFn = get_fn("grappa_core");
      info.makeFn = ptr_info.makeFn = get_fn("grappa_make_wide");
      
      auto voidTy = Type::getVoidTy(module.getContext());
      info.gTypes[voidTy] = ptr_info;
      
      info.getFn = get_fn("grappa_get");
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
    fprintf(stderr, "Registered Grappa pass!\n");
    PM.add(new GrappaWrapper());
  }
  static RegisterStandardPasses GrappaGenRegistration(PassManagerBuilder::EP_ScalarOptimizerLate, registerGrappaGen);
  
}
