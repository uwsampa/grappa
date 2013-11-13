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
  
#define dump_var_l(before, dumpee, after) \
    errs() << before << #dumpee << ": "; \
    dumpee->dump(); \
    errs() << after

#define dump_var(dumpee) dump_var_l("", dumpee, "\n")
  
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
    target->setMetadata(label, dloc.getAsMDNode(ctx));
  }
  
  struct GrappaGen : public FunctionPass {
    static char ID;
    
    Constant *get_fn, *put_fn, *read_long_fn, *fetchadd_i64_fn;
    
    Type *void_ty, *void_ptr_ty, *void_gptr_ty, *i64_ty;
    
    GrappaGen() : FunctionPass(ID) { }
    
    void replaceWithRemoteLoad(LoadInst *orig_ld) {
      outs() << "global get:"; orig_ld->dump();
      outs() << "  uses: " << orig_ld->getNumUses() << "\n";
      orig_ld->getOperandUse(0);
      auto ty = orig_ld->getType();
      auto gptr = orig_ld->getPointerOperand();
      
      if (ty == i64_ty) {
        outs() << "  specializing -- grappa_read_long()\n";
        Value *args[] = {
          gptr
        };
        auto f = CallInst::Create(read_long_fn, args, "", orig_ld);
        ir_comment(f, "grappa.read", "");
        myReplaceInstWithInst(orig_ld, f);
        // (call gets inlined by -O2 so won't be able to find 'grappa_read_long' call)
        
      } else {
        
        auto alloc = makeAlloca(ty, "", orig_ld); // allocate space to load into
        ir_comment(alloc, "grappa.get.alloca", "");
        
        auto void_alloc = new BitCastInst(alloc, void_ptr_ty, "", orig_ld);
        auto void_gptr = new BitCastInst(gptr, void_gptr_ty, "", orig_ld);
        auto szof = createSizeof(ty);
        
        Value* args[] = { void_alloc, void_gptr, szof };
        CallInst::Create(get_fn, args, "", orig_ld);
        
        // Now load from alloc'd area
        auto new_ld = new LoadInst(alloc, "", orig_ld->isVolatile(), orig_ld->getAlignment(), orig_ld->getOrdering(), orig_ld->getSynchScope(), orig_ld);
        
        myReplaceInstWithInst(orig_ld, new_ld);
        ir_comment(new_ld, "grappa.get", "");
      }
      
    }

    void replaceWithRemoteStore(StoreInst *orig) {
      outs() << "global put:"; orig->dump();
      
      auto gptr = orig->getPointerOperand();
      auto val = orig->getValueOperand();
      auto ty = val->getType();
      
      // TODO: find out if we can store directly from source
      auto alloc = makeAlloca(ty, "", orig); // allocate space to put from
      ir_comment(alloc, "grappa.put.alloca", "");
      
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
      ir_comment(put, "grappa.put", "");
      
    }
    
    virtual bool runOnFunction(Function &F) {
      bool changed = false;
      // errs() << "processing fn: " << F.getName() << "\n";
      
      std::multiset<LoadInst*> global_loads;
      std::multiset<StoreInst*> global_stores;
      
      struct FetchAdd {
        LoadInst *ld;
        StoreInst *store;
        Value *inc;
      };
      std::vector<FetchAdd> fetchadds;
      
      for (auto& bb : F) {
        
        // look for fetch_add opportunity
        std::map<Value*,LoadInst*> target_lds;
        std::map<Value*,Value*> target_increments;
        
        for (auto& inst : bb) {
          switch ( inst.getOpcode() ) {
            
            case Instruction::Load: {
              auto& ld = *cast<LoadInst>(&inst);
              if (ld.getPointerAddressSpace() == GLOBAL_SPACE) {
                if (ld.getPointerOperand()->getType()->getPointerElementType() == i64_ty) {
                  target_lds[ld.getPointerOperand()] = &ld;
                  target_increments[&ld] = nullptr;
                }
                
                global_loads.insert(&ld);
                changed = true;
              }
              break;
            }
            
            case Instruction::Store: {
              auto store = cast<StoreInst>(&inst);
              if (store->getPointerAddressSpace() == GLOBAL_SPACE) {
                
                auto ptr = store->getPointerOperand();
                if (target_lds.count(ptr) > 0) {
                  errs() << "target_lds\n";
                  auto ld = target_lds[ptr];
                  auto inc = target_increments[ld];
                  if (inc != nullptr) {
                    errs() << "found fetch_add:\n";
                    dump_var_l("  ", ptr, "");
                    dump_var_l("  ", inc, "");
                    dump_var_l("  ", store, "");
                    dump_var_l("  ", ld, "");
                    errs() << "\n";
                    fetchadds.emplace_back( FetchAdd{ld, store, inc} );
                    global_loads.erase(ld);
                  } else {
                    global_stores.insert(store);
                  }
                } else {
                  global_stores.insert(store);
                }
              }
              break;
            }
            
            case Instruction::Add: {
              auto& o = *cast<BinaryOperator>(&inst);
              if (target_increments.count(o.getOperand(0))) {
                target_increments[o.getOperand(0)] = o.getOperand(1);
              } else if (target_increments.count(o.getOperand(1))) {
                target_increments[o.getOperand(1)] = o.getOperand(0);
              }
              break;
            }
          }
        }
      }
      
      for (auto& fa : fetchadds) {
        Value *args[] = { fa.ld->getPointerOperand(), fa.inc };
        auto f = CallInst::Create(fetchadd_i64_fn, args, "", fa.ld);
        myReplaceInstWithInst(fa.ld, f);
        // leave add instruction there (in case its result is used)
        fa.store->removeFromParent();
        ir_comment(f, "grappa.fetchadd", "");
      }
      
      for (auto* inst : global_loads) {
        replaceWithRemoteLoad(inst);
      }
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
      read_long_fn = getFunctionSafe(module, "grappa_read_long");
      fetchadd_i64_fn = getFunctionSafe(module, "grappa_fetchadd_i64");
      
      i64_ty = llvm::Type::getInt64Ty(module.getContext());
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
  static RegisterStandardPasses GrappaGenRegistration(PassManagerBuilder::EP_ScalarOptimizerLate, registerGrappaGen);
  
}
