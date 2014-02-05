
#undef DEBUG

#include <llvm/Support/Debug.h>
#include <llvm/IR/Constants.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/CallSite.h>

#include "Passes.h"
#include "DelegateExtractor.hpp"

using namespace llvm;

namespace Grappa {
  
  Value* search(Value* v) {
    if (auto gep = dyn_cast<GetElementPtrInst>(v)) {
      if (!gep->isInBounds()) return v;
      if (gep->hasIndices()) {
        if (gep->getPointerAddressSpace() == GLOBAL_SPACE) {
          auto idx = gep->getOperand(1);
          if (idx != ConstantInt::get(idx->getType(), 0)) {
            return v;
          }
        }
      }
      return search(gep->getPointerOperand());
    }
    if (auto c = dyn_cast<CastInst>(v)) {
      auto vv = search(c->getOperand(0));
      if (isa<PointerType>(vv->getType()))
        return vv;
      else
        return v;
    }
    if (auto c = dyn_cast<ConstantExpr>(v)) {
      auto ci = c->getAsInstruction();
      auto vv = search(ci);
      delete ci;
      return vv;
    }
    
    return v;
  }
  
  void setProvenance(Instruction* inst, Value* ptr) {
    inst->setMetadata("grappa.prov", MDNode::get(inst->getContext(), ptr));
  }
  
  Value* getProvenance(Instruction* inst) {
    if (auto m = inst->getMetadata("grappa.prov")) {
      return m->getOperand(0);
    }
    return nullptr;
  }
  
  bool isGlobalPtr(Value* v)    { return dyn_cast_addr<GLOBAL_SPACE>(v->getType()); }
  bool isSymmetricPtr(Value* v) { return dyn_cast_addr<SYMMETRIC_SPACE>(v->getType()); }
  bool isStatic(Value* v)       { return isa<GlobalVariable>(v); }
  bool isConst(Value* v)       { return isa<Constant>(v) || isa<BasicBlock>(v); }
  bool isStack(Value* v)        { return isa<AllocaInst>(v) || isa<Argument>(v); }
  
  bool isAnchor(Instruction* inst) {
    Value* ptr = getProvenance(inst);
    if (ptr)
      return isGlobalPtr(ptr) || isStack(ptr);
    else
      return false;
  }
  
  void ExtractorPass::analyzeProvenance(Function& fn, AnchorSet& anchors) {
    for (auto& bb: fn) {
      for (auto& i : bb) {
        Value *prov = nullptr;
        if (auto l = dyn_cast<LoadInst>(&i)) {
          prov = search(l->getPointerOperand());
        } else if (auto s = dyn_cast<StoreInst>(&i)) {
          prov = search(s->getPointerOperand());
        }
        if (prov) {
          setProvenance(&i, prov);
          if (isAnchor(&i)) {
            anchors.insert(&i);
          }
        }
      }
    }
  }
  
  struct CandidateRegion {
    Instruction* entry;
    std::map<Instruction*,Instruction*> exits;
    
    SmallSet<Value*,4> valid_ptrs;
    
    CandidateRegion(Instruction* entry): entry(entry) {}
    
    void expandRegion() {
      UniqueQueue<Instruction*> worklist;
      worklist.push(entry);
      
      SmallSet<BasicBlock*,8> bbs;
      
      while (!worklist.empty()) {
        auto i = BasicBlock::iterator(worklist.pop());
        auto bb = i->getParent();
        
        while ( i != bb->end() && validInRegion(i) ) i++;
        
        if (i == bb->end()) {
          bbs.insert(bb);
          for (auto sb = succ_begin(bb), se = succ_end(bb); sb != se; sb++) {
            bool valid = true;
            // all predecessors already in region
            for_each(pb, *sb, pred) valid &= (bbs.count(*pb) > 0);
            // at least first instruction is valid
            auto target = (*sb)->begin();
            valid &= validInRegion(target);
            
            if (valid) {
              worklist.push(target);
            } else {
              // exit
              auto ex = i->getPrevNode();
              if (exits.count(target) > 0 && exits[target] != ex) {
                assert(false && "unhandled case");
              } else {
                exits[target] = ex;
              }
            }
          }
        } else {
          exits[i] = i->getPrevNode();
        }
      }
      
    }
    
    bool validInRegion(Instruction* i) {
      if (i->mayReadOrWriteMemory()) {
        if (auto p = getProvenance(i)) {
          if (valid_ptrs.count(p) || isSymmetricPtr(p) || isStatic(p) || isConst(p)) {
            return true;
          }
        } else if (isa<CallInst>(i) || isa<InvokeInst>(i)) {
          // do nothing for now
          auto cs = CallSite(i);
          if (auto fn = cs.getCalledFunction()) {
            if (fn->hasFnAttribute("unbound") || fn->doesNotAccessMemory()) {
              return true;
            }
          }

        } else {
          errs() << "!! no provenance:" << *i;
        }
        return false;
      } else {
        return true;
      }
    }
    
    void prettyPrint(BasicBlock* bb = nullptr) {
      outs() << "~~~~~~~~~~~~~~~~~~~~~~~\n";
      outs() << "Candidate: {" << *entry << "  }\n";
      outs() << "  valid_ptrs:\n";
      for (auto p : valid_ptrs) outs() << "  " << *p << "\n";
      outs() << "  exits:\n";
      for (auto p : exits) outs() << "  " << *p.first << "\n     =>" << *p.second << "\n";
      outs() << "\n";
      
      BasicBlock::iterator i;
      if (!bb) {
        bb = entry->getParent();
        i = BasicBlock::iterator(entry);
        if (i != bb->begin()) {
          outs() << *i->getPrevNode() << "\n";
        }
      } else {
        i = bb->begin();
        outs() << bb->getName() << ":\n";
      }
      outs() << "--------------------\n";
      
      for (; i != bb->end(); i++) {
        if (exits.count(i)) {
          outs() << "--------------------\n";
          i++;
          if (i != bb->end()) outs() << *i << "\n";
          break;
        }
        outs() << *i << "\n";
      }
      if (i == bb->end()) {
        for_each(sb, bb, succ) {
          prettyPrint(*sb);
        }
      }
    }
  };

  
  
  bool ExtractorPass::runOnModule(Module& M) {
    outs() << "Running extractor...\n";
    bool changed = false;
    
//    if (! ginfo.init(M) ) return false;
    ginfo.init(M);
    
    //////////////////////////
    // Find 'task' functions
    for (auto& F : M) {
      if (F.hasFnAttribute("async")) {
        task_fns.insert(&F);
      }
    }
    
    outs() << "task_fns.count => " << task_fns.size() << "\n";
    
    std::vector<DelegateExtractor*> candidates;
    
    for (auto fn : task_fns) {
      
//      auto& provenance = getAnalysis<ProvenanceProp>(*fn);
//      provenance.prettyPrint(*fn);
//      auto dex = new DelegateExtractor(M, ginfo);
      AnchorSet anchors;
      analyzeProvenance(*fn, anchors);
      
      for (auto a : anchors) {
        auto p = getProvenance(a);
        if (isGlobalPtr(p)) {
          CandidateRegion r(a);
          r.valid_ptrs.insert(p);
          r.expandRegion();
          
          r.prettyPrint();
          outs() << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
        }
      }
      
    }
    
//    outs() << "<< anchors:\n";
//    for (auto a : anchors) {
//      outs() << "  " << *a << "\n";
//    }
//    outs() << ">>>>>>>>>>\n";
//    outs().flush();
    
    return changed;
  }
    
  bool ExtractorPass::doInitialization(Module& M) {
    outs() << "-- Grappa Extractor --\n";
    outs().flush();
    return false;
  }
  
  bool ExtractorPass::doFinalization(Module& M) {
    outs().flush();
    return true;
  }
  
  char ExtractorPass::ID = 0;
  
  //////////////////////////////
  // Register optional pass
  static RegisterPass<ExtractorPass> X( "grappa-ex", "Grappa Extractor", false, false );
  
}
