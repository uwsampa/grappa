
#undef DEBUG

//#ifdef DEBUG
////#define _DEBUG
//#undef DEBUG
//#endif

#include <llvm/Support/Debug.h>
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
#include <llvm/Support/CFG.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/Analysis/Dominators.h>
#include <llvm/Analysis/DomPrinter.h>
#include <llvm/IR/IRBuilder.h>

#include <llvm/Transforms/Utils/CodeExtractor.h>
//#include "MyCodeExtractor.h"

#include <llvm/InstVisitor.h>

#include "DelegateExtractor.hpp"

#include <sstream>
#include <set>
#include <map>
#include <queue>
#include <list>

//// I'm not sure why, but I can't seem to enable debug output in the usual way
//// (with command line flags). Here's a hack.
//#undef DEBUG
//#define DEBUG(s) s

using namespace llvm;

//STATISTIC(grappaGlobalRefs, "calls to Grappa's distributed shared memory system");

namespace {
  
  template< typename T, unsigned SizeHint = 32 >
  class UniqueQueue {
    std::queue<T> q;
    SmallSet<T,SizeHint> visited;
  public:
    void push(T bb) {
      if (visited.count(bb) == 0) {
        visited.insert(bb);
        q.push(bb);
      }
    }
    T pop() {
      auto bb = q.front();
      q.pop();
      return bb;
    }
    bool empty() { return q.empty(); }
  };
  
#define dump_var_l(before, dumpee, after) \
    errs() << before << #dumpee << ": "; \
    dumpee->dump(); \
    errs() << afterDT

#define dump_var(dumpee) dump_var_l("", dumpee, "\n")
  
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
    
    struct FetchAdd {
      LoadInst *ld;
      StoreInst *store;
      Value *inc;
      Instruction *op;
    };
    
    GlobalPtrInfo ginfo;
    
    Function *get_fn, *put_fn, *read_long_fn, *fetchadd_i64_fn, *call_on_fn, *get_core_fn,
             *get_pointer_fn, *get_pointer_symm_fn;
    
    Function *myprint_i64;
    
    Type *void_ty, *void_ptr_ty, *void_gptr_ty, *void_symmptr_ty, *i64_ty;
    
    DominatorTree *DT;
    
    GrappaGen() : FunctionPass(ID) { }
    
    
    /// Experimental code to replace FetchAdd with a `grappa_on` using CodeExtractor directly
    void replaceFetchAdd(FetchAdd& fa) {
      errs() << "#########################################\nfetch_add:\n" << *fa.ld << "  " << *fa.op << "  " << *fa.store << "\n";
      
      fa.ld->getPointerOperand()->setName("gptr");
      
      BasicBlock* block = fa.ld->getParent();
      assert( block == fa.op->getParent() && block == fa.store->getParent() );
      
      auto find_in_bb = [](BasicBlock *bb, Value *v) {
        for (auto it = bb->begin(), end = bb->end(); it != end; it++) {
          if (&*it == v) return it;
        }
        return bb->end();
      };
      
      auto ld_gptr = fa.ld->getPointerOperand();
      
      // split basic block into pre-delegate (block), delegate_blk, and post_blk
      auto load_loc = find_in_bb(block, fa.ld);
      auto delegate_blk = block->splitBasicBlock(load_loc, "grappa.delegate");
      
      auto store_loc = find_in_bb(delegate_blk, fa.store);
      auto post_blk = delegate_blk->splitBasicBlock(++store_loc, "delegate.post");
      
      // get type of load inst
      auto ld_ty = dyn_cast<PointerType>(ld_gptr->getType())->getElementType()->getPointerTo();
      
      errs() << "\n-- pre block:\n" << *block;
      errs() << "\n-- delegate block:\n" << *delegate_blk;
      errs() << "\n-- post block:\n" << *post_blk;
      
      // remove debug call that was holding onto references to insts moved to other fn
      // TODO: generalize this to remove anything with function-local refs (or even better: repair function-local refs)
      for (BasicBlock::iterator i = post_blk->begin(); i != post_blk->end(); ) {
        auto inst = i++;
        if (auto c = dyn_cast<CallInst>(inst)) {
          if (c->getCalledFunction()->getName() == "llvm.dbg.value") {
            c->eraseFromParent();
          }
        }
      }
      
      auto struct_args = false;
      CodeExtractor ex(delegate_blk, struct_args);
      SetVector<Value*> ins, outs;
      ex.findInputsOutputs(ins, outs);
      errs() << "\nins:"; for (auto v : ins) errs() << "\n  " << *v; errs() << "\n";
      errs() << "outs:"; for (auto v : outs) errs() << "\n  " << *v; errs() << "\n";
      assert( ins.size()  == 1 );
      assert( outs.size() == 1 );
      
      auto delegate_fn = ex.extractCodeRegion();
      errs() << "\n--------\ndelegate: " << *delegate_fn;
      
      auto prepost = block->getNextNode();
      
      // find call to extracted function created by CodeExtrator
      CallInst *delegate_call;
      for (auto& i : *prepost) {
        if (auto call = dyn_cast<CallInst>(&i)) {
          if (call->getCalledFunction() == delegate_fn) {
            delegate_call = call;
            break;
          }
        }
      }
      assert( delegate_call->getNumArgOperands() == 2 );
      
      errs() <<"\n-----------\nblock:\n" << *block;
      errs() << "\n---------\nprepost: " << *prepost;
      
      // first load global pointer from pointer-to-pointer
      //        auto arg_input = delegate_call->getArgOperand(0);
      //        errs() << "arg_input: " << *arg_input << "\n  type: " << *arg_input->getType() << "\n";
      auto arg_ptr = fa.ld->getPointerOperand();
      auto arg_ptrptr = new BitCastInst(arg_ptr, arg_ptr->getType()->getPointerTo(), "", fa.ld);
      auto ld_input = new LoadInst(arg_ptrptr, "", fa.ld);
      
      auto vptr = CallInst::Create(get_pointer_fn, (Value*[]){
        new BitCastInst(ld_input, void_gptr_ty, "", fa.ld),
      }, "", fa.ld);
      
      auto ptr = new BitCastInst(vptr, ld_ty, "", fa.ld);
      
      fa.ld->setOperand(fa.ld->getPointerOperandIndex(), ptr);
      fa.store->setOperand(fa.store->getPointerOperandIndex(), ptr);
      
      errs() << "\n--------\ndelegate <fixed up>: " << *delegate_fn;
      
      // get size (in bytes) of the value pointed-to as a ConstantInt*
      auto get_size_bytes = [this](Value *p) {
        assert( p->getType()->isPointerTy() );
        auto p_ty = p->getType()->getPointerElementType();
        assert( p_ty->getScalarSizeInBits() % 8 == 0 );
        auto sz_val = p_ty->getScalarSizeInBits() / 8;
        auto const_int = ConstantInt::get(i64_ty, sz_val);
        assert( const_int != NULL );
        return const_int;
      };
      
      errs() << "@@@@@@\ndelegate_call: ";
      for (unsigned i=0; i< delegate_call->getNumOperands(); i++) {
        errs() << "  " << *delegate_call->getOperand(i) << "\n";
      }
      errs() << "\n";
      
      auto gptr = delegate_call->getOperand(0);
      
      //        Value *arg_in; // has to be pointer to the arg
      //        if (auto gptrptr = dyn_cast<LoadInst>(gptr)) {
      //          arg_in = gptrptr->getPointerOperand();
      //        } else {
      //          arg_in = nullptr;
      //          assert(false && "need to implement alloca");
      //        }
      auto arg_in = gptr;
      
      auto arg_in_sz = get_size_bytes(arg_in);
      auto arg_out = delegate_call->getOperand(1);
      auto arg_out_sz = get_size_bytes(arg_out);
      
      //        Value *get_core_args = { fa.ld->getPointerOperand() };
      //        auto target_core = CallInst::Create(get_core_fn, get_core_args, "", delegate_call);
      auto target_core = CallInst::Create(get_core_fn, (Value*[]){
        new BitCastInst(gptr, void_gptr_ty, "", delegate_call)
      }, "", delegate_call);
      
      auto on_fn_ty = call_on_fn->getFunctionType()->getParamType(1);
      
      CallInst::Create(call_on_fn, (Value*[]){
        target_core,
        new BitCastInst(delegate_fn, on_fn_ty, "", delegate_call),
        new BitCastInst(arg_in, void_ptr_ty, "", delegate_call),
        arg_in_sz,
        new BitCastInst(arg_out, void_ptr_ty, "", delegate_call),
        arg_out_sz
      }, "", delegate_call);
      delegate_call->eraseFromParent();
      
      errs() << "-- prepost now: " << *prepost;
      
    }
    
    void specializeFetchAdd(FetchAdd& fa) {
      Value *args[] = { fa.ld->getPointerOperand(), fa.inc };
      auto f = CallInst::Create(fetchadd_i64_fn, args, "", fa.ld);
      myReplaceInstWithInst(fa.ld, f);
      // leave add instruction there (in case its result is used)
      fa.store->dropAllReferences();
      fa.store->removeFromParent();
      ir_comment(f, "grappa.fetchadd", "");
    }
    
    void replaceFetchAddWithGenericDelegate(FetchAdd& fa, Module* mod) {
      errs() << "#########################################\nfetch_add:\n" << *fa.ld << "  " << *fa.op << "  " << *fa.store << "\n";
      
      fa.ld->getPointerOperand()->setName("gptr");
      
      BasicBlock* block = fa.ld->getParent();
      assert( block == fa.op->getParent() && block == fa.store->getParent() );
      
      auto find_in_bb = [](BasicBlock *bb, Value *v) {
        for (auto it = bb->begin(), end = bb->end(); it != end; it++) {
          if (&*it == v) return it;
        }
        return bb->end();
      };
      
      auto ld_gptr = fa.ld->getPointerOperand();
      
      // split basic block into pre-delegate (block), delegate_blk, and post_blk
      auto load_loc = find_in_bb(block, fa.ld);
      auto delegate_blk = block->splitBasicBlock(load_loc, "fa");
      
      auto store_loc = find_in_bb(delegate_blk, fa.store);
      auto post_blk = delegate_blk->splitBasicBlock(++store_loc, "delegate.post");
      
      errs() << "\n-- pre block:\n" << *block;
      errs() << "\n-- delegate block:\n" << *delegate_blk;
      errs() << "\n-- post block:\n" << *post_blk;
      errs() << "---------------------\n";
      
      DelegateExtractor dex(*mod, ginfo);
      dex.bbs.insert(delegate_blk);
      dex.gptrs.insert(ld_gptr);
      dex.extractFunction();
    }
    
    void replaceWithRemoteLoad(LoadInst *orig_ld) {
      errs() << "global get:"; orig_ld->dump();
      errs() << "  uses: " << orig_ld->getNumUses() << "\n";
      orig_ld->getOperandUse(0);
      auto ty = orig_ld->getType();
      auto gptr = orig_ld->getPointerOperand();
      
      if (ty == i64_ty) {
        errs() << "  specializing -- grappa_read_long()\n";
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
      errs() << "global put:"; orig->dump();
      
      auto gptr = orig->getPointerOperand();
      auto val = orig->getValueOperand();
      auto ty = val->getType();
      
      // TODO: find out if we can store directly from source
      auto alloc = makeAlloca(ty, "", orig); // allocate space to put from
      ir_comment(alloc, "grappa.put.alloca", "");
      
      // Now store into alloc'd area
      new StoreInst(val, alloc, orig->isVolatile(), orig->getAlignment(), orig->getOrdering(), orig->getSynchScope(), orig);
      
      auto put = CallInst::Create(put_fn, (Value*[]){
        new BitCastInst(gptr, void_gptr_ty, "", orig),
        new BitCastInst(alloc, void_ptr_ty, "", orig),
        createSizeof(ty)
      }, "", orig);
      
      // substitue original instruction for the new last one (call to 'grappa_put')
      myReplaceInstWithInst(orig, put);
      ir_comment(put, "grappa.put", "");
      errs() << "\n";
      
    }
    
    virtual bool runOnFunction(Function &F) {
//      DEBUG( outs() << F.getName() << "\n" );
      auto& mod = *F.getParent();
      DT = &getAnalysis<DominatorTree>();

      bool changed = false;
      // errs() << "processing fn: " << F.getName() << "\n";
      
      std::multiset<LoadInst*> global_loads;
      std::multiset<StoreInst*> global_stores;
      
      std::vector<FetchAdd> fetchadds;
      
      //////////////////////////////////////
      // find candidate regions
      //
      // algorithm:
      // - find all global accesses
      // - for each global access:
      //   - walk forward as far as possible; stop if:
      //     - different global address (could lead to branching opportunity)
      //     - Core-local access (including global statics--can't guarantee it's okay to move those)
      //     - output (allows code movement to be observed)
      //   - check input/output sizes; if larger than threshold, ignore
      //   - save as candidate extraction region (with use info)
      //
      // enabling runtime choices/multi-address candidates
      // - don't stop if different global address, rather save candidate so far, then continue
      // - save multi-address candidate (and associate with single-address candidates that overlap)
      // - add runtime branch:
      //     if (addr1.core() == addr2.core()) { call multi_address_region }
      //     else { call addr1; ...; call addr2 }
      
      std::map<BasicBlock*,Value*> candidate_bbs;
      
      std::list<DelegateExtractor*> candidates;
      
      UniqueQueue<BasicBlock*> todo;
      todo.push(F.begin());
      
      while (!todo.empty()) {
        auto& dex = *new DelegateExtractor(mod, ginfo);

        auto bb = todo.pop();
        
        if ( auto entry = dex.findStart(bb) ) {
          DEBUG( errs() << "\n// found candidate\n" );
          
          dex.expand(entry);
          
          // add all exits to todo list
          for (auto& e : dex.exits) {
            todo.push(e.first);
          }
          
          // dex.viewUnextracted();
          candidates.push_back(&dex);
          
          DEBUG( outs() << dex );
          
        } else { // if no global loads/stores
          // just add successors
          for(auto s = succ_begin(bb), se = succ_end(bb); s != se; s++)
            todo.push(*s);
          
          delete &dex;
        }
        
      }
      
      std::vector<Function*> fns;
      fns.push_back(&F);
      
      for (auto c : candidates) {
        DEBUG( errs() << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );
        fns.push_back( c->extractFunction() );
      }
      
      if (candidates.size() > 0) {
        DEBUG( outs() << "\n" );
      }
      
      /////////////////////////////////////
      // Fix symmetric loads
      
      for (auto fn : fns) {
        for (auto bb = fn->begin(); bb != fn->end(); bb++) {
          for (auto inst = bb->begin(); inst != bb->end(); ) {
            
            bool replaceOrig = false;
            Value *sptr = nullptr;
            if (auto orig = dyn_cast_addr<SYMMETRIC_SPACE,LoadInst>(inst)) {
              outs() << "load<sym>          =>" << *orig << "\n";
              sptr = orig->getPointerOperand();
            } else if (auto orig = dyn_cast_addr<SYMMETRIC_SPACE,StoreInst>(inst)) {
              outs() << "store<sym>         =>" << *orig << "\n";
              sptr = orig->getPointerOperand();
            } else if (auto orig = dyn_cast<AddrSpaceCastInst>(inst)) {
              if (orig->getSrcTy()->getPointerAddressSpace() == SYMMETRIC_SPACE) {
              outs() << "addrspacecast<sym> =>" << *orig << "\n";
                sptr = orig->getOperand(0);
                replaceOrig = true;
              }
            }
            
            // doing this so we don't lose the iterator if we replace inst
            Instruction *orig = inst;
            inst++;
            
            // inst doesn't need to be replaced
            if (!sptr) continue;
            
            // find the 'addrspace(0)*' version of the symmetric ptr type
            // sptrTy('T symmetric*') -> lptrTy('T*')
            auto sptrTy = dyn_cast<PointerType>(sptr->getType());
            auto lptrTy = PointerType::get(sptrTy->getElementType(), 0);
            
            // get local pointer out of symmetric pointer
            IRBuilder<> b(orig); Value *v;
            v = b.CreateBitCast(sptr, void_symmptr_ty);
            v = b.CreateCall(get_pointer_symm_fn, (Value*[]){ v });
            v = b.CreateBitCast(v, lptrTy);
            
            auto lptr = v;
            lptr->setName(sptr->getName() + ".sym.local");
            
            // replace invalid use of symmetric pointer
            if (replaceOrig) {
              orig->replaceAllUsesWith(lptr);
              orig->eraseFromParent();
            } else {
              orig->replaceUsesOfWith(sptr, lptr);
            }
            
          }
        }
      }
      
      /////////////////////////////////////
      
      return changed;
    }
    
    virtual bool doInitialization(Module& module) {
      outs() << "-- Grappa Pass:\n";
      DEBUG( errs() << "  * debug on\n" );
      auto getFunction = [&module](StringRef name) {
        auto fn = module.getFunction(name);
        if (!fn) {
          llvm::errs() << "unable to find " << name << "\n";
          abort();
        }
        return fn;
      };
      
      get_fn = getFunction("grappa_get");
      put_fn = getFunction("grappa_put");
      read_long_fn = getFunction("grappa_read_long");
      fetchadd_i64_fn = getFunction("grappa_fetchadd_i64");
      ginfo.call_on_fn = call_on_fn = getFunction("grappa_on");
      ginfo.get_core_fn = get_core_fn = getFunction("grappa_get_core");
      ginfo.get_pointer_fn = get_pointer_fn = getFunction("grappa_get_pointer");
      ginfo.get_pointer_symm_fn = get_pointer_symm_fn = getFunction("grappa_get_pointer_symmetric");
      
      myprint_i64 = getFunction("myprint_i64");
      
      i64_ty = llvm::Type::getInt64Ty(module.getContext());
      void_ptr_ty = Type::getInt8PtrTy(module.getContext(), 0);
      void_gptr_ty = Type::getInt8PtrTy(module.getContext(), GLOBAL_SPACE);
      void_symmptr_ty = Type::getInt8PtrTy(module.getContext(), SYMMETRIC_SPACE);
      void_ty = Type::getVoidTy(module.getContext());
      
      // module = &m;
      // auto int_ty = m.getTypeByName("int");
      // delegate_read_fn = m.getOrInsertFunction("delegate_read_int", int_ty, );
      
      return false;
    }
    
    virtual bool doFinalization(Module &M) {
      outs().flush();
      return true;
    }
    
    virtual void getAnalysisUsage(AnalysisUsage& AU) const {
      AU.addRequired<DominatorTree>();
    }
    
    ~GrappaGen() { }
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
