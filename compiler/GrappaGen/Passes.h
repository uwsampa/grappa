#pragma once

#include <llvm/Pass.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/ADT/SetVector.h>
#include <llvm/ADT/StringMap.h>
#include <set>
#include <queue>
#include <unordered_map>
#include <cxxabi.h>
#include <llvm/Analysis/AliasAnalysis.h>

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
for (auto var = (arg).op_begin(), var##_end = (arg).op_end(); var != var##_end; var++)

#define for_each_use(var, arg) \
for (auto var = (arg).use_begin(), var##_end = (arg).use_end(); var != var##_end; var++)

#define assert2(cond, msg, p1, p2) \
  do { if (!(cond)) { errs() << "!! " << msg << "\n" << p1 << "\n" << p2 << "\n\n"; assert(false && msg); } } while (0)

template< typename T >
void print_rest(const T& first) {
  llvm::errs() << "  " << first << "\n";
}

template< typename T, typename... Ts>
void print_rest(const T& first, const Ts&... rest) {
  llvm::errs() << "  " << first << "\n";
  print_rest(rest...);
}

template<typename... Ts> void assert_var(bool cnd, llvm::StringRef file, unsigned line,
                                         const Ts&... rest) {
  if (!cnd) {
    llvm::errs() << "assert failed at " << file << ":" << line << "\n";
    print_rest(rest...);
    assert(false);
  }
}

#define assertN(cnd, ...) assert_var((cnd), __FILE__, __LINE__, __VA_ARGS__)


using namespace llvm;

inline std::string demangle(StringRef name) {
  char *cs = abi::__cxa_demangle(name.data(), 0, 0, 0);
  if (cs) {
    std::string s(cs);
    free(cs);
    return s;
  } else {
    return name;
  }
}

inline std::string mangleSimpleGlobal(StringRef n) {
  std::string _s;
  raw_string_ostream name(_s);
  name << "_ZN";
  
  auto sr = StringRef(n);
  while (true) {
    auto p = sr.split("::");
    name << p.first.size() << p.first;
    sr = p.second;
    if (sr.empty()) break;
  }
  name << "E";
  return name.str();
}

using AnchorSet = SetVector<Instruction*>;
using ValueSet = SetVector<Value*>;

inline Constant* idx(int i, LLVMContext& ctx) { return ConstantInt::get(Type::getInt32Ty(ctx), i); }

inline Value* CreateAlignedStore(IRBuilder<>& b, Value* val, Value* ptr,
                          int field = -1, const Twine& name = "") {
  unsigned sz = 0;
  auto& c = ptr->getContext();
  if (field >= 0) {
    ptr = b.CreateInBoundsGEP(ptr, {idx(0,c), idx(field,c)}, name);
    sz = cast<PointerType>(ptr->getType())->getElementType()->getPrimitiveSizeInBits() / 8;
  }
  return (sz) ? b.CreateAlignedStore(val, ptr, sz)
              : b.CreateStore(val, ptr);
}

inline Value* CreateAlignedLoad(IRBuilder<>& b, Value* ptr,
                          int field = -1, const Twine& name = "") {
  unsigned sz = 0;
  auto& c = ptr->getContext();
  if (field >= 0) {
    ptr = b.CreateInBoundsGEP(ptr, {idx(0,c), idx(field,c)}, name);
    sz = cast<PointerType>(ptr->getType())->getElementType()->getPrimitiveSizeInBits() / 8;
  }
  return (sz) ? b.CreateAlignedLoad(ptr, sz, name)
              : b.CreateLoad(ptr, name);
}

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

inline PointerType* getAddrspaceType(Value *orig, int addrspace = 0) {
  return getAddrspaceType(orig->getType(), addrspace);
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
  
  using LocalPtrMap = SmallDenseMap<Value*,Value*>;
  
  StringMap<Function*> fn_map;
  LLVMContext *ctx;
  Module *mod;
  
  Function*& fn(StringRef name) {
    assertN(fn_map.count(name), "missing function", name);
    return fn_map[name];
  }
  
//  Function *call_on_fn, *call_on_async_fn,
//           *make_gptr_fn,
//           *global_get_fn, *global_put_fn,
//           *global_get_i64, *global_put_i64,
//           *get_core_fn, *get_pointer_fn, *get_pointer_symm_fn;
//#define call_on_fn fn["call_on"]
  
    
  bool init(Module& M) {
    mod = &M;
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
    
    fn_map["on"] = getFunction("grappa_on");
    fn_map["on_async"] = getFunction("grappa_on_async");
    fn_map["on_async_16"] = getFunction("grappa_on_async_16");
    fn_map["on_async_32"] = getFunction("grappa_on_async_32");
    fn_map["on_async_64"] = getFunction("grappa_on_async_64");
    fn_map["on_async_128"] = getFunction("grappa_on_async_128");
    
    fn_map["get_core"] = getFunction("grappa_get_core");
    fn_map["get_pointer"] = getFunction("grappa_get_pointer");
    fn_map["get_pointer_symmetric"] = getFunction("grappa_get_pointer_symmetric");
    fn_map["get"] = getFunction("grappa_get");
    fn_map["put"] = getFunction("grappa_put");
    fn_map["get_i64"] = getFunction("grappa_get_i64");
    fn_map["put_i64"] = getFunction("grappa_put_i64");
    fn_map["get_on"] = getFunction("grappa_get_on");
    fn_map["put_on"] = getFunction("grappa_put_on");
    fn_map["get_i64_on"] = getFunction("grappa_get_i64_on");
    fn_map["put_i64_on"] = getFunction("grappa_put_i64_on");
    fn_map["make_gptr"] = getFunction("grappa_make_gptr");
    
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
    v = b.CreateCall(fn("get_pointer_symmetric"), (Value*[]){ v }, name+".lvoid");
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
    v = b.CreateCall(fn("get_pointer"), (Value*[]){ v }, name+".lvoid");
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
  
  Value* SmartCast(IRBuilder<>& b, Value* v, Type* ty, const Twine& name = "") {
    if (CastInst::isCastable(v->getType(), ty)) {
      return b.CreateCast(CastInst::getCastOpcode(v, true, ty, true), v, ty, name);
    } else {
      assert2(false, "smart cast not smart enough", *v, *ty);
    }
    return nullptr;
  }
  
  Value* replace_global_access(Value *ptr, Value* core, Instruction *orig, LocalPtrMap& lptrs,
                               DataLayout& layout) {
    auto& C = orig->getContext();
    Type *ty = cast<PointerType>(ptr->getType())->getElementType();
    Twine name = ptr->getName().size() ? ptr->getName()+".g" : "g";
    
    auto sz = ConstantExpr::getSizeOf(ty);
    
    IRBuilder<> b(orig);
    Value * v = nullptr;
    
    if (layout.getTypeAllocSize(ty) == 8) {
      outs() << "specializing i64 -- line " << orig->getDebugLoc().getLine() << "\n";
      auto i64 = Type::getInt64Ty(C);
      auto i64_ptr_ty = Type::getInt64PtrTy(C, ptr->getType()->getPointerAddressSpace());
      
      v = ptr;
      
      if (isa<LoadInst>(orig)) {
        
        if (ty != i64) v = SmartCast(b, v, i64_ptr_ty, name+".ptr.bc");
        if (core) {
          v = b.CreateCall(fn("get_i64_on"), { core, v }, name+".val");
        } else {
          v = b.CreateCall(fn("get_i64"), { v }, name+".val");
        }
        if (ty != i64) v = SmartCast(b, v, ty, name+".val.bc");
        
      } else if (auto s = dyn_cast<StoreInst>(orig)) {
        
        auto val = s->getValueOperand();
        if (ty != i64) {
          v = SmartCast(b, v, i64_ptr_ty, name+".ptr.bc");
          val = SmartCast(b, val, i64, name+".val.bc");
        }
        if (core) {
          v = b.CreateCall(fn("put_i64_on"), { core, v, val });
        } else {
          v = b.CreateCall(fn("put_i64"), { v, val });
        }
        
      } else {
        assert(false && "unimplemented case");
      }
      
    } else {
      
      auto alloca_loc = orig->getParent()->getParent()->getEntryBlock().begin();
      auto tmp = new AllocaInst(ty, name+".tmp", alloca_loc);
      
      auto v_tmp = b.CreateBitCast(tmp, void_ptr_ty, name+".tmp.void");
      
      auto dst_ptr_ty = Type::getInt8PtrTy(C, ptr->getType()->getPointerAddressSpace());
      auto v_ptr = b.CreateBitCast(ptr, dst_ptr_ty, name+".void");
      
      if (auto l = dyn_cast<LoadInst>(orig)) {
        
        // remote get into temp
        if (core) {
          v = b.CreateCall(fn("get_on"), {core, v_tmp, v_ptr, sz });
        } else {
          v = b.CreateCall(fn("get"), {v_tmp, v_ptr, sz});
        }
        // load value out of temp
        v = b.CreateLoad(tmp, l->isVolatile(), name+".val");
        
      } else if (auto s = dyn_cast<StoreInst>(orig)) {
        
        // store into temporary alloca
        v = b.CreateStore(s->getValueOperand(), tmp, s->isVolatile());
        // do remote put out of alloca
        if (core) {
          v = b.CreateCall(fn("put_on"), {core, v_ptr, v_tmp, sz});
        } else {
          v = b.CreateCall(fn("put"), {v_ptr, v_tmp, sz});
        }
        
      } else if (isa<AddrSpaceCastInst>(orig)) {
        assert(false && "unimplemented");
      }
    }
    
    orig->replaceAllUsesWith(v);
    orig->eraseFromParent();
    
    return v;
  }
  
  template< int SPACE >
  Value* replace_with_local(Value *xptr, Instruction *orig, LocalPtrMap& lptrs) {
    Function* get_xptr_fn = nullptr;
    Type*     void_xptr_ty = nullptr;
    StringRef suffix;
    if (SPACE == GLOBAL_SPACE) {
      get_xptr_fn = fn("get_pointer");
      void_xptr_ty = void_gptr_ty;
      suffix = "g";
    } else if (SPACE == SYMMETRIC_SPACE) {
      get_xptr_fn = fn("get_pointer_symmetric");
      void_xptr_ty = void_sptr_ty;
      suffix = "s";
    }
    
    // insert after gptr use for better reuse
    assert(!isa<TerminatorInst>(xptr));
    
    IRBuilder<> b(orig);
    if (auto i = dyn_cast<Instruction>(xptr)) {
      b.SetInsertPoint(i->getNextNode());
    }

    Twine name = (xptr->getName().size() ? xptr->getName()+"." : "" ) + suffix;
    
    auto lptr = lptrs[xptr];
    if (!lptr) {
      Type *lty = getAddrspaceType(xptr->getType(), 0);
      Value *v = xptr;
             v = b.CreateBitCast(v, void_xptr_ty, name+".void");
             v = b.CreateCall(get_xptr_fn, (Value*[]){ v }, name+".lvoid");
             v = b.CreateBitCast(v, lty, name+".lptr");
      lptr = lptrs[xptr] = v;
    }
    
    Value *v = nullptr;
    b.SetInsertPoint(orig);
    
    if (auto ld = dyn_cast<LoadInst>(orig)) {
      v = b.CreateLoad(lptr, ld->isVolatile(), name+".val");
    } else if (auto st = dyn_cast<StoreInst>(orig)) {
      v = b.CreateStore(st->getOperand(0), lptr, st->isVolatile());
    } else if (auto ac = dyn_cast<AddrSpaceCastInst>(orig)) {
      v = b.CreateBitCast(lptr, ac->getType(), name+".bc");
    } else {
      assert(false && "tried to replace instruction of unknown type!");
    }
    
    if (v) {
      orig->replaceAllUsesWith(v);
      orig->eraseFromParent();
    }
    
    return v;
  }
  
  template< int SPACE >
  Value* ptr_operand(Instruction* orig) {
    if (auto ld = dyn_cast_addr<SPACE,LoadInst>(orig)) {
      return ld->getPointerOperand();
    } else if (auto st = dyn_cast_addr<SPACE,StoreInst>(orig)) {
      return st->getPointerOperand();
    } else if (auto c = dyn_cast<AddrSpaceCastInst>(orig)) {
      if (c->getSrcTy()->getPointerAddressSpace() == SPACE) {
        return c->getOperand(0);
      }
    }
    return nullptr;
  }
  
  Instruction* insert_get_pointer(Instruction *gptr) {
    assert(!isa<TerminatorInst>(gptr));
    IRBuilder<> b(gptr->getNextNode());
    
    Type *ty = getAddrspaceType(gptr->getType(), 0);
    Twine name = gptr->getName().size() ? gptr->getName()+".g" : "g";
    
    Value *v = gptr;
    v = b.CreateBitCast(v, void_gptr_ty, name+".void");
    v = b.CreateCall(fn("get_pointer"), (Value*[]){ v }, name+".lvoid");
    v = b.CreateBitCast(v, ty, name+".lptr");
    
    return cast<Instruction>(v);
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
    DataLayout* layout;
    
    DenseMap<Function*,GlobalVariable*> async_fns;
    
    void analyzeProvenance(Function& fn, AnchorSet& anchors);
    int fixupFunction(Function* fn);
    
    ExtractorPass() : ModulePass(ID) { }
    
    virtual bool runOnModule(Module& M);
    virtual bool doInitialization(Module& M);
    virtual bool doFinalization(Module& M);
    
    virtual void getAnalysisUsage(AnalysisUsage& AU) const {
      AU.addRequired<ProvenanceProp>();
      AU.addRequired<AliasAnalysis>();
    }
  };
  
}
