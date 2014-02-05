#pragma once

#include <llvm/Pass.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/ADT/SetVector.h>
#include <set>
#include <queue>
#include <unordered_map>
#include <cxxabi.h>


#undef DEBUG_TYPE
#define DEBUG_TYPE "grappa"

#define void_ty      Type::getVoidTy(*ctx)
#define i64_ty       Type::getInt64Ty(*ctx)
#define i16_ty       Type::getInt16Ty(*ctx)
#define void_ptr_ty  Type::getInt8PtrTy(*ctx, 0)
#define void_gptr_ty Type::getInt8PtrTy(*ctx, GLOBAL_SPACE)
#define void_sptr_ty Type::getInt8PtrTy(*ctx, SYMMETRIC_SPACE)

/// helper for iterating over preds/succs/uses
#define for_each(var, arg, prefix) \
for (auto var = prefix##_begin(arg), _var##_end = prefix##_end(arg); var != _var##_end; var++)

#define for_each_op(var, arg) \
for (auto var = arg.op_begin(), var##_end = arg.op_end(); var != var##_end; var++)

#define for_each_use(var, arg) \
for (auto var = arg.use_begin(), var##_end = arg.use_end(); var != var##_end; var++)

using namespace llvm;

inline std::string demangle(StringRef name) {
  char *cs = abi::__cxa_demangle(name.data(), 0, 0, 0);
  std::string s(cs);
  free(cs);
  return s;
}

using AnchorSet = SetVector<Instruction*>;

////////////////////
/// Address spaces
static const int GLOBAL_SPACE = 100;
static const int SYMMETRIC_SPACE = 200;


inline PointerType* getAddrspaceType(Type *orig, int addrspace = 0) {
  if (auto p = dyn_cast<PointerType>(orig)) {
    return PointerType::get(p->getElementType(), addrspace);
  }
  return nullptr;
}

template< int AddrSpace >
inline PointerType* dyn_cast_addr(Type* ty) {
  PointerType *pt = dyn_cast<PointerType>(ty);
  if (pt && pt->getAddressSpace() == AddrSpace) return pt;
  else return nullptr;
}

template< int AddrSpace, typename InstType >
inline InstType* dyn_cast_addr(Value* v) {
  if (auto ld = dyn_cast<InstType>(v)) {
    if (ld->getPointerAddressSpace() == AddrSpace) {
      return ld;
    }
  }
  return nullptr;
}

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

////////////////////////////////////////////////
/// Utilities for working with Grappa pointers
struct GlobalPtrInfo {
  Function *call_on_fn, *get_core_fn, *get_pointer_fn, *get_pointer_symm_fn;
  
  LLVMContext *ctx;
  
  bool init(Module& M) {
    ctx = &M.getContext();
    
    bool disabled = false;
    
    auto getFunction = [&disabled,&M](StringRef name) {
      auto fn = M.getFunction(name);
      if (!fn) {
        llvm::errs() << "unable to find " << name << " -- disabling GrappaGen pass\n";
        disabled = true;
      }
      return fn;
    };
    
    call_on_fn = getFunction("grappa_on");
    get_core_fn = getFunction("grappa_get_core");
    get_pointer_fn = getFunction("grappa_get_pointer");
    get_pointer_symm_fn = getFunction("grappa_get_pointer_symmetric");
    
    return !disabled;
  }
  
  bool isaGlobalPointer(Type* type) {
    PointerType* pt = dyn_cast<PointerType>(type);
    if( pt && pt->getAddressSpace() == GLOBAL_SPACE ) return true;
    return false;
  }
  
  Value* symm_get_ptr(IRBuilder<>& b, Value *sptr, Type *lptrTy) {
    // if not given a type, just get the addrspace(0) version of sptr's type
    if (!lptrTy) lptrTy = getAddrspaceType(sptr->getType(), 0);
    
    Twine name = sptr->getName().size() > 0 ? sptr->getName()+".s" : "s";

    Value *v;

    v = b.CreateBitCast(sptr, void_sptr_ty,                name+".void");
    v = b.CreateCall(get_pointer_symm_fn, (Value*[]){ v }, name+".lvoid");
    v = b.CreateBitCast(v, lptrTy,                         name+".local");
    
    return v;
  }
  
  Value* symm_get_ptr(Instruction *insert_pt, Value *sptr, Type *lptrTy) {
    IRBuilder<> b(insert_pt);
    return symm_get_ptr(b, sptr, lptrTy);
  }

  
  Value* replace_global_with_local(Value *gptr, Instruction *orig) {
    IRBuilder<> b(orig);
    
    Type *ty = getAddrspaceType(gptr->getType(), 0);
    Twine name = gptr->getName().size() ? gptr->getName()+".g" : "g";

    Value *v = gptr;
    v = b.CreateBitCast(v, void_gptr_ty, name+".void");
    v = b.CreateCall(get_pointer_fn, (Value*[]){ v }, name+".lvoid");
    v = b.CreateBitCast(v, ty, name+".lptr");
    
    if (auto ld = dyn_cast<LoadInst>(orig)) {
      v = b.CreateLoad(gptr, ld->isVolatile(), name+".val");
    } else if (auto st = dyn_cast<StoreInst>(orig)) {
      v = b.CreateStore(st->getOperand(0), gptr, st->isVolatile());
    } else {
      assert(false && "tried to replace instruction of unknown type!");
    }
    
    orig->replaceAllUsesWith(v);
    orig->eraseFromParent();
    
    return v;
  }
  
};

namespace Grappa {
  
  enum ProvenanceClass { Unknown, Indeterminate, Global, Symmetric, Static, Const, Stack };
  
  class ProvenanceProp : public FunctionPass {
  public:
    
    static MDNode *INDETERMINATE;
    
    DenseMap<Value*,ProvenanceClass> prov;
    
    LLVMContext *ctx;
    
    MDNode* provenance(Value* v);
    
    ProvenanceClass classify(Value* v);
//    MDNode* meet(Value* a, Value* b);
    
    MDNode* search(Value *v);
    
    void viewGraph(Function *fn);
    void prettyPrint(Function& fn);
    
    static char ID;
    ProvenanceProp() : FunctionPass(ID) {}
    virtual void getAnalysisUsage(AnalysisUsage& AU) const { AU.setPreservesAll(); }
    virtual bool runOnFunction(Function& F);
    virtual bool doInitialization(Module& M);
  };
  
  struct ExtractorPass : public ModulePass {
    
    static char ID;
    
    GlobalPtrInfo ginfo;
    
    std::set<Function*> task_fns;
    
    void analyzeProvenance(Function& fn, AnchorSet& anchors);
    
    ExtractorPass() : ModulePass(ID) { }
    
    virtual bool runOnModule(Module& M);
    virtual bool doInitialization(Module& M);
    virtual bool doFinalization(Module& M);
    
    virtual void getAnalysisUsage(AnalysisUsage& AU) const {
      AU.addRequired<ProvenanceProp>();
    }
  };
  
}
