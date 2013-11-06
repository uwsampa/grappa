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

#include <sstream>
#include <set>
#include <map>

// I'm not sure why, but I can't seem to enable debug output in the usual way
// (with command line flags). Here's a hack.
#undef DEBUG
#define DEBUG(s) s

using namespace llvm;

//STATISTIC(grappaGlobalRefs, "calls to Grappa's distributed shared memory system");

namespace {
  
#define dump_var(dumpee) \
    errs() << #dumpee << ": "; \
    dumpee->dump(); \
    errs() << "\n"
  
  static const int GLOBAL_SPACE = 100;
  
  bool isaGlobalPointer(Type* type) {
    PointerType* pt = dyn_cast<PointerType>(type);
    if( pt && pt->getAddressSpace() == GLOBAL_SPACE ) return true;
    return false;
  }
  
  //////////////////////////////////////
  /// From llvmGlobalToWide.cpp
  AllocaInst* makeAlloca(llvm::Type* type,
                         const char* name,
                         Instruction* insertBefore)
  {
    // It's important to alloca at the front of the function in order
    // to avoid having an alloca in a loop which is a good way to achieve
    // stack overflow.
    Function *func = insertBefore->getParent()->getParent();
    BasicBlock* entryBlock = & func->getEntryBlock();
    
    if( insertBefore->getParent() == entryBlock ) {
      // Add before specific instruction in entry block.
    } else if(llvm::Instruction *i = func->getEntryBlock().getTerminator()) {
      // Add before terminator in entry block.
      insertBefore = i;
    } else {
      // Add at the end of entry block.
      insertBefore = NULL;
    }
    
    AllocaInst *tempVar;
    
    if( insertBefore ) {
      tempVar = new AllocaInst(type, name, insertBefore);
    } else {
      tempVar = new AllocaInst(type, name, entryBlock);
    }
    
    return tempVar;
  }
  
  Constant* createSizeof(Type* type)
  {
    // assert( !containsGlobalPointers(info, type) );
    return ConstantExpr::getSizeOf(type);
  }
  
  void myReplaceInstWithInst(Instruction* Old, Instruction* New)
  {
    assert(Old->getParent());
    assert(New->getParent());
    Old->replaceAllUsesWith(New);
    if(Old->hasName() && !New->hasName()) New->takeName(Old);
    Old->eraseFromParent();
  }
  
  Constant* getFunctionSafe(Module& module, const char* fn_name) {
    auto fn = module.getFunction(fn_name);
    if (!fn) {
      llvm::errs() << "unable to find " << fn_name << "\n";
      abort();
    }
    return fn;
  }
  
  template< typename T >
  void ir_comment(T* target, StringRef label, const Twine& text) {
    auto& ctx = target->getContext();
    target->setMetadata(label, MDNode::get(ctx, MDString::get(ctx, text.str())));
  }

  template< typename T >
  void ir_comment(T* target, StringRef label, const DebugLoc& dloc) {
    auto& ctx = target->getContext();
    outs() << "dloc: "; dloc.dump(ctx); outs() << "\n";
    target->setMetadata(label, dloc.getAsMDNode(ctx));
  }
  
  struct GrappaGen : public FunctionPass {
    static char ID;
    
    Constant *get_fn, *put_fn;
    
    Type *void_ty, *void_ptr_ty, *void_gptr_ty;
    
    GrappaGen() : FunctionPass(ID) { }
    
    void replaceWithRemoteLoad(LoadInst *orig_ld) {
      auto ty = orig_ld->getType();
      auto gptr = orig_ld->getPointerOperand();
      
      auto alloc = makeAlloca(ty, "", orig_ld); // allocate space to load into
      ir_comment(alloc, "grappa.alloca", orig_ld->getDebugLoc());
      
      auto void_alloc = new BitCastInst(alloc, void_ptr_ty, "", orig_ld);
      auto void_gptr = new BitCastInst(gptr, void_gptr_ty, "", orig_ld);
      auto szof = createSizeof(ty);
      
      Value* args[] = { void_alloc, void_gptr, szof };
      CallInst::Create(get_fn, args, "", orig_ld);
      
      // Now load from alloc'd area
      auto new_ld = new LoadInst(alloc, "", orig_ld->isVolatile(), orig_ld->getAlignment(), orig_ld->getOrdering(), orig_ld->getSynchScope(), orig_ld);
      
      myReplaceInstWithInst(orig_ld, new_ld);
      
      errs() << "global load("; ty->dump(); errs() << ")\n";
    }

    void replaceWithRemoteStore(StoreInst *orig) {
      auto gptr = orig->getPointerOperand();
      auto val = orig->getValueOperand();
      auto ty = val->getType();
      
      // TODO: find out if we can store directly from source
      auto alloc = makeAlloca(ty, "", orig); // allocate space to put from
      
      // Now store into alloc'd area
      new StoreInst(val, alloc, orig->isVolatile(), orig->getAlignment(), orig->getOrdering(), orig->getSynchScope(), orig);
      
      Value * args[] = {
        new BitCastInst(gptr, void_gptr_ty, "", orig),
        new BitCastInst(alloc, void_ptr_ty, "", orig),
        createSizeof(ty)
      };
      auto put = CallInst::Create(put_fn, args, "", orig);
      
      // substitue original instruction for the new last one (call to 'grappa_put')
      myReplaceInstWithInst(orig, put);
      
      errs() << "global store("; ty->dump(); errs() << ")\n";
    }
    
    virtual bool runOnFunction(Function &F) {
      bool changed = false;
      // errs() << "processing fn: " << F.getName() << "\n";
      
      std::vector<LoadInst*> global_loads;
      std::vector<StoreInst*> global_stores;
      
      for (auto& bb : F) {
        for (auto& inst : bb) {
          switch ( inst.getOpcode() ) {
            
            case Instruction::Load: {
              auto orig_ld = cast<LoadInst>(&inst);
              if (orig_ld->getPointerAddressSpace() == GLOBAL_SPACE) {
                global_loads.push_back(orig_ld);
                changed = true;
              }
              break;
            }
            
            case Instruction::Store: {
              auto orig_store = cast<StoreInst>(&inst);
              if (orig_store->getPointerAddressSpace() == GLOBAL_SPACE) {
                global_stores.push_back(orig_store);
              }
              break;
            }
            
          }
        }
      }
      
      for (auto* inst : global_loads) { replaceWithRemoteLoad(inst); }
      for (auto* inst : global_stores) { replaceWithRemoteStore(inst); }
      
      return changed;
    }
    
    virtual bool doInitialization(Module& module) {
      errs() << "initializing\n";
      
      // for (auto& f : module.getFunctionList()) {
      //   llvm::errs() << "---"; f.dump();
      // }
      
      get_fn = getFunctionSafe(module, "grappa_get");
      put_fn = getFunctionSafe(module, "grappa_put");
      
      void_ptr_ty = Type::getInt8PtrTy(module.getContext(), 0);
      void_gptr_ty = Type::getInt8PtrTy(module.getContext(), GLOBAL_SPACE);
      
      // module = &m;
      // auto int_ty = m.getTypeByName("int");
      // delegate_read_fn = m.getOrInsertFunction("delegate_read_int", int_ty, );
      return false;
    }
    
    virtual bool doFinalization(Module &M) {
      return true;
    }
    
    ~GrappaGen() {
      llvm::errs() << "closing\n";
    }
  };
  
  char GrappaGen::ID = 0;
  
  //////////////////////////////
  // Register optional pass
  static RegisterPass<GrappaGen> X(
                                   "grappa-gen", "Grappa Code Generator",
                                   false, false
                                   );
  
  //////////////////////////////
  // Register as default pass
  static void registerGrappaGen(const PassManagerBuilder&, PassManagerBase& PM) {
    fprintf(stderr, "Registered GrappaGen pass!\n");
    PM.add(new GrappaGen());
  }
  static RegisterStandardPasses GrappaGenRegistration(PassManagerBuilder::EP_OptimizerLast, registerGrappaGen);
}
