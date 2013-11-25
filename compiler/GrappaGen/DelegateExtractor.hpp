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

#ifdef DEBUG
#undef DEBUG
#endif

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

#include <llvm/Transforms/Utils/CodeExtractor.h>

namespace llvm {

  struct OperandIterator {
    Instruction* self;
    OperandIterator(Instruction* self): self(self) {}
    User::op_iterator begin() { return self->op_begin(); }
    User::op_iterator end()   { return self->op_end(); }
  };
  
  struct GlobalPtrInfo {
    Function *call_on_fn, *get_core_fn, *get_pointer_fn;
    
    static const int SPACE = 100;
    
    bool isaGlobalPointer(Type* type) {
      PointerType* pt = dyn_cast<PointerType>(type);
      if( pt && pt->getAddressSpace() == SPACE ) return true;
      return false;
    }
    
  };
  
  inline PointerType* dyn_cast_global(Type* ty) {
    PointerType *pt = dyn_cast<PointerType>(ty);
    if (pt && pt->getAddressSpace() == GlobalPtrInfo::SPACE) return pt;
    else return nullptr;
  }

  template< typename InstType >
  inline InstType* dyn_cast_global(Value* v) {
    if (auto ld = dyn_cast<InstType>(v)) {
      if (ld->getPointerAddressSpace() == GlobalPtrInfo::SPACE) {
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
    
    SetVector<BasicBlock*> bbs;
    DominatorTree* dom;
    Module& mod;
    DataLayout* layout;
    GlobalPtrInfo& ginfo;
    
  public:
    
    DelegateExtractor(BasicBlock* bb, Module& mod, GlobalPtrInfo& ginfo);
//    DelegateExtractor(ArrayRef<BasicBlock*> bbs, DominatorTree* dom);
    
    /// Compute the inputs and outputs and catalog all uses of global pointers in the specified code.
    void findInputsOutputsUses(ValueSet& inputs, ValueSet& outputs, GlobalPtrMap& global_ptr_uses) const;
    
    /// Construct generic delegate from region specified for this DelegateExtractor.
    /// Replaces the original BasicBlock with a new one that invokes `grappa_on()`
    ///
    /// @return constructed delegate function of type: void(void* in, void* out)
    Function* constructDelegateFunction(Value* gptr);
  };

}