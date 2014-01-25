//===-- Transform/Utils/DelegateExtractor.h - Code extraction util --*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// A utility to support extracting code from one function into its own
// stand-alone function.
//
//===----------------------------------------------------------------------===//

#pragma once

//#ifdef DEBUG
//#undef DEBUG
//#endif

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
#include <llvm/IR/Dominators.h>
#include <llvm/Analysis/DomPrinter.h>
#include <llvm/Analysis/CFGPrinter.h>
#include <llvm/Support/GraphWriter.h>
#include <llvm/Support/InstIterator.h>
#include <llvm/Transforms/Utils/CodeExtractor.h>

#include <list>
#include <queue>

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

namespace llvm {
  

  inline void replaceAllUsesInFnWith(Function *fn, Value *orig, Value *repl) {
    for (auto inst = inst_begin(fn), ie = inst_end(fn); inst != ie; inst++) {
      for (auto op = inst->op_begin(), op_e = inst->op_end(); op != op_e; op++) {
        
      }
    }
    
  }
  
  static const int GLOBAL_SPACE = 100;
  static const int SYMMETRIC_SPACE = 200;
  
  inline PointerType* getAddrspaceType(Type *orig, int addrspace = 0) {
    if (auto p = dyn_cast<PointerType>(orig)) {
      return PointerType::get(p->getElementType(), addrspace);
    }
    return nullptr;
  }
  
  struct GlobalPtrInfo {
    Function *call_on_fn, *get_core_fn, *get_pointer_fn, *get_pointer_symm_fn;
    
    bool isaGlobalPointer(Type* type) {
      PointerType* pt = dyn_cast<PointerType>(type);
      if( pt && pt->getAddressSpace() == GLOBAL_SPACE ) return true;
      return false;
    }
    
  };
  
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
  
  struct GlobalPtrUse {
    int16_t loads, stores, uses;
    GlobalPtrUse(): loads(0), stores(0), uses(0) {}
  };
  using GlobalPtrMap = std::map<Value*,GlobalPtrUse>;
  using ValueSet = SetVector<Value*>;
  
  
  
  class DelegateExtractor {
    
  public:
    SmallSetVector<Value*,4> gptrs;
    
    /// map of exit blocks (outside bbs) -> pred blocks (in bbs)
    SmallSetVector<std::pair<BasicBlock*,BasicBlock*>,4> exits;
    
    SetVector<BasicBlock*> bbs;
//    DominatorTree* dom;
    LLVMContext *ctx;
    Module* mod;
    
    DataLayout* layout;
    GlobalPtrInfo& ginfo;
    
    Function* outer_fn;
    
  public:
    
    DelegateExtractor(Module& mod, GlobalPtrInfo& ginfo);
    
    /// Compute the inputs and outputs and catalog all uses of global pointers in the specified code.
    void findInputsOutputsUses(ValueSet& inputs, ValueSet& outputs, GlobalPtrMap& global_ptr_uses) const;
    
    /// Construct generic delegate from region specified for this DelegateExtractor.
    /// Replaces the original BasicBlock with a new one that invokes `grappa_on()`
    ///
    /// @return constructed delegate function of type: void(void* in, void* out)
    Function* extractFunction();
    
    bool valid_in_delegate(Instruction* inst, ValueSet& available_vals);
    
    /// find start of DelegateRegion
    BasicBlock* findStart(BasicBlock *bb);
    
    // expand region as far as possible within BB
    bool expand(BasicBlock *bb);
    
    void print(raw_ostream& o) const;
    
//    bool greedyExtract(BasicBlock *start) {
//      auto bb = findStart(start);
//      DEBUG(errs() << "start:" << *bb << "\n");
//      
//      if (!bb) {
//        return false;
//      } else {
//        expand(bb);
//        return true;
//      }
//    }
    
    void viewUnextracted();
  };

  inline raw_ostream& operator<<(raw_ostream& o, const DelegateExtractor& r) {
    r.print(o);
    return o;
  }
  
  template <>
  struct GraphTraits<DelegateExtractor*> : public GraphTraits<const BasicBlock*> {
    static NodeType *getEntryNode(DelegateExtractor *d) { return &d->outer_fn->getEntryBlock(); }
    
    // nodes_iterator/begin/end - Allow iteration over all nodes in the graph
    typedef Function::iterator nodes_iterator;
    static nodes_iterator nodes_begin(DelegateExtractor *d) { return d->outer_fn->begin(); }
    static nodes_iterator nodes_end  (DelegateExtractor *d) { return d->outer_fn->end(); }
    static size_t         size       (DelegateExtractor *d) { return d->outer_fn->size(); }
  };
  
  template<>
  struct DOTGraphTraits<DelegateExtractor*> : public DefaultDOTGraphTraits {
    DOTGraphTraits(bool simple=false): DefaultDOTGraphTraits(simple) {}
    
    using FTraits = DOTGraphTraits<const Function*>;
    
    static std::string getGraphName(DelegateExtractor *f) { return "Delegate"; }
    
    std::string getNodeLabel(const BasicBlock *Node, DelegateExtractor* dex) {
      return FTraits::getCompleteNodeLabel(Node, dex->outer_fn);
    }
    
    template< typename T >
    static std::string getEdgeSourceLabel(const BasicBlock *Node, T I) {
      return FTraits::getEdgeSourceLabel(Node, I);
    }
    
    static std::string getNodeAttributes(const BasicBlock* Node, DelegateExtractor* dex) {
      auto bb = const_cast<BasicBlock*>(Node);
      if (dex->bbs.count(bb)) {
        return "color=\"blue\",style=\"filled\"";
      } else {
        return "";
      }
    }
  };
  
}
