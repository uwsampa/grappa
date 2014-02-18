
#undef DEBUG

#include <llvm/Support/Debug.h>
#include <llvm/IR/Constants.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/CallSite.h>
#include <llvm/Support/GraphWriter.h>
#include <llvm/InstVisitor.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Format.h>

#include "Passes.h"
#include "DelegateExtractor.hpp"

using namespace llvm;

StringRef getColorString(unsigned ColorNumber) {
  static const int NumColors = 20;
  static const char* Colors[NumColors] = {
    "red", "blue", "green", "gold", "cyan", "purple", "orange",
    "darkgreen", "coral", "deeppink", "deepskyblue", "orchid", "brown", "yellowgreen",
    "midnightblue", "firebrick", "peachpuff", "yellow", "limegreen", "khaki"};
  return Colors[ColorNumber % NumColors];
}

static cl::opt<bool> PrintDot("grappa-dot", cl::desc("Dump pass info to dot format."));

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
    if (isa<Constant>(v)) {
      return v;
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
  
#define X(expr) if (!v) return false; else return (expr);
  
  bool isGlobalPtr(Value* v)    { X(dyn_cast_addr<GLOBAL_SPACE>(v->getType())); }
  bool isSymmetricPtr(Value* v) { X(dyn_cast_addr<SYMMETRIC_SPACE>(v->getType())); }
  bool isStatic(Value* v)       { X(isa<GlobalVariable>(v)); }
  bool isConst(Value* v)        { X(isa<Constant>(v) || isa<BasicBlock>(v)); }
  bool isStack(Value* v)        { X(isa<AllocaInst>(v) || isa<Argument>(v)); }
  
#undef X
  
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
        } else if (auto c = dyn_cast<AddrSpaceCastInst>(&i)) {
          prov = search(c->getOperand(0));
          
          for_each_use(u, *c) {
            if (auto m = dyn_cast<CallInst>(*u)) {
              if (auto pp = getProvenance(m)) {
                assert2(pp == prov, "!! two different provenances\n", *c, *m);
              }
              // TODO: do 'meet' of provenances of operands
              // for now, just making sure we're not doing anything dumb
              for_each_op(o, *m) {
                if (auto cc = dyn_cast<AddrSpaceCastInst>(*o)) {
                  assert2(cc == c, "!! two different operands being addrspacecast'd\n", *c, *m);
                }
              }
              
              // call inherits provenance of this cast
              setProvenance(m, prov);
              
            }
          }
          
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
  
  
  struct CandidateRegion;
  using CandidateMap = std::map<Instruction*,CandidateRegion*>;
  
  struct CandidateRegion {
    static long id_counter;
    long ID;
    
    Instruction* entry;
    Type *ty_input, *ty_output;
    
    class ExitMap {
      // holds either:
      // - Instruction* before
      // - BasicBlock* successor -> pred
      using Map = ValueMap<Value*,WeakVH>;
      Map m;

    public:
      
      void clear() { m.clear(); }
      
      template< typename F >
      void each(F yield) const {
        SmallVector<Map::value_type,8> v;
        for (auto p : m) v.push_back(p);
        
        for (auto p : v) {
          if (auto before = dyn_cast<Instruction>(p.first)) {
            yield(before, BasicBlock::iterator(before)->getNextNode());
          } else if (auto succ = dyn_cast<BasicBlock>(p.first)) {
            auto pred = cast<Instruction>(p.second);
            yield(pred, succ->begin());
          } else {
            assert(false && "unknown exit");
          }
        }
      }
      
      void add(Instruction* pred, BasicBlock* succ) {
        assert(pred && succ);
        m[succ] = pred;
      }
      
      void remove(Instruction* before, Instruction* after) {
        auto bb_after = after->getParent();
        if (m.count(bb_after)) {
          assert(m[bb_after] == before);
          m.erase(bb_after);
        } else if (m.count(before)) {
          m.erase(before);
        } else {
          assert(false && "trying to remove non-existent exit");
        }
      }
      
      void add(Instruction* before) {
        assert(before);
        m[before] = nullptr;
      }
      
      void removeSuccessor(BasicBlock* succ) {
        m.erase(succ);
      }
      
      bool isExitStart(Instruction* i) const {
        if (m.count(i)) {
          return true;
        } else if (isa<TerminatorInst>(i)) {
          auto pred = i->getParent();
          for (auto p : m)
            if (p.second == pred)
              return true;
        }
        return false;
      }
      
      bool isExitEnd(BasicBlock* b) const { return m.count(b); }
      
      unsigned size() { return m.size(); }
    };
    
    ExitMap exits;
    
    
    WeakVH target_ptr;
    SmallSet<Value*,4> valid_ptrs;
    
    CandidateMap& owner;
    
    GlobalPtrInfo& ginfo;
    DataLayout& layout;
    
    CandidateRegion(Value* target_ptr, Instruction* entry, CandidateMap& owner, GlobalPtrInfo& ginfo, DataLayout& layout):
      ID(id_counter++), entry(entry), target_ptr(target_ptr), owner(owner), ginfo(ginfo), layout(layout) {}
    
    template< typename F >
    void visit(F yield) {
      UniqueQueue<Instruction*> q;
      q.push(entry);
      
      while (!q.empty()) {
        BasicBlock::iterator it(q.pop());
        auto bb = it->getParent();
        while (it != bb->end()) {
          auto iit = it;
          it++;
          yield(iit);
          if (exits.isExitStart(iit)) break;
        }
        if (it == bb->end()) {
          for_each(sb, bb, succ)
            if (!exits.isExitEnd(*sb))
              q.push(sb->begin());
        }
      }
    }
    
    void expandRegion() {
      UniqueQueue<Instruction*> worklist;
      worklist.push(entry);
      
      SmallSet<BasicBlock*,8> bbs;
      SmallSetVector<BasicBlock*,8> try_again;
      
      while (!worklist.empty()) {
        auto i = BasicBlock::iterator(worklist.pop());
        auto bb = i->getParent();
        
        while ( i != bb->end() && validInRegion(i) ) {
          owner[i] = this;
          i++;
        }
        
        if (i == bb->end()) {
          bbs.insert(bb);
          for (auto sb = succ_begin(bb), se = succ_end(bb); sb != se; sb++) {
            
            // at least first instruction is valid
            auto target = (*sb)->begin();
            bool valid = validInRegion(target);
            
            // all predecessors already in region
            if (valid) for_each(pb, *sb, pred) {
              bool b = (bbs.count(*pb) > 0);
              if (!b) {
                DEBUG(outs() << "!! deferring -- invalid preds:\n" << *bb << "\n" << **pb);
                try_again.insert(*sb);
              }
              valid &= b;
            }
            
            if (valid) {
              worklist.push(target);
            } else {
              // exit at bb boundary
              exits.add(bb->getTerminator(), *sb);
            }
          }
        } else {
          exits.add(i->getPrevNode());
        }
        
        // check try-again list
        for (auto bb : try_again) {
          bool valid = true;
          for_each(pb, bb, pred) valid &= (bbs.count(*pb) > 0);
          if (valid) {
            auto b = bb;
            exits.removeSuccessor(b);
            try_again.remove(b);
            worklist.push(b->begin());
            break;
          }
        }
      } // while (!worklist.empty())
      
      //////////////////////////////////////////////////////
      // rollback before branch if all exits from single bb
      if (exits.size() > 1) {
        SmallSetVector<Instruction*,8> preds;
        exits.each([&](Instruction* before, Instruction* after){
          preds.insert(before);
        });
        if (preds.size() == 1) {
          exits.clear();
          owner[preds[0]] = nullptr;
          auto p = BasicBlock::iterator(preds[0])->getPrevNode();
          DEBUG(outs() << "@bh unique_pred =>" << *preds[0] << "\n");
          exits.add(p);
        }
      }

      computeInputsOutputs();
    }
    
    bool validInRegion(Instruction* i) {
      if (i->mayReadOrWriteMemory()) {
        if (auto p = getProvenance(i)) {
          if (valid_ptrs.count(p) || isSymmetricPtr(p) || isStatic(p) || isConst(p)) {
            return true;
          }
        } else if (isa<CallInst>(i)) { // || isa<InvokeInst>(i)) {
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
      } else if (isa<ReturnInst>(i) || isa<InvokeInst>(i)){
        return false;
      } else {
        return true;
      }
    }
    
    /////////////////////////
    // find inputs/outputs
    void computeInputsOutputs() {
      auto& ctx = entry->getContext();
      
      auto definedInRegion = [&](Value* v) {
        if (auto i = dyn_cast<Instruction>(v))
          if (owner[i] == this)
            return true;
        return false;
      };
      auto definedInCaller = [&](Value* v) {
        if (isa<Argument>(v)) return true;
        if (auto i = dyn_cast<Instruction>(v))
          if (owner[i] != this)
            return true;
        return false;
      };
      
      ValueSet inputs, outputs;
      visit([&](BasicBlock::iterator it){
        for_each_op(o, *it)  if (definedInCaller(*o)) inputs.insert(*o);
        for_each_use(u, *it) if (!definedInRegion(*u)) { outputs.insert(it); break; }
      });
      
      /////////////////////////////////////////////
      // create struct types for inputs & outputs
      SmallVector<Type*,8> in_types, out_types;
      for (auto& p : inputs)  {  in_types.push_back(p->getType()); }
      for (auto& p : outputs) { out_types.push_back(p->getType()); }
      
      ty_input = StructType::get(ctx, in_types);
      ty_output = StructType::get(ctx, out_types);
    }
    
    Function* extractRegion() {
      auto name = "d" + Twine(ID);
      DEBUG(outs() << "//////////////////\n// extracting " << name << "\n");
      DEBUG(outs() << "target_ptr =>" << *target_ptr << "\n");
      
      auto mod = entry->getParent()->getParent()->getParent();
      auto& ctx = entry->getContext();
      auto ty_i16 = Type::getInt16Ty(ctx);
      auto ty_void_ptr = Type::getInt8PtrTy(ctx);
      auto ty_void_gptr = Type::getInt8PtrTy(ctx, GLOBAL_SPACE);
      auto i64 = [&](int64_t v) { return ConstantInt::get(Type::getInt64Ty(ctx), v); };
      auto idx = [&](int i) { return ConstantInt::get(Type::getInt32Ty(ctx), i); };
      
      SmallSet<BasicBlock*,8> bbs;
      
      DEBUG({
        outs() << "pre exits:\n";
        exits.each([&](Instruction* before_exit, Instruction* after_exit){
          outs() << *before_exit << " (in " << before_exit->getParent()->getName() << ")\n  =>" << *after_exit << " (in " << after_exit->getParent()->getName() << ")\n";
        });
        outs() << "\n";
      });
      
      //////////////////////////////////////////////////////////////
      // first slice and dice at boundaries and build up set of BB's
      auto bb_in = entry->getParent();
      DEBUG(
        outs() << "entry =>" << *entry << "\n";
        outs() << "bb_in => " << bb_in->getName() << "\n";
      );

      auto old_fn = bb_in->getParent();
      
      if (BasicBlock::iterator(entry) != bb_in->begin()) {
        auto bb_new = bb_in->splitBasicBlock(entry, name+".eblk");
        bb_in = bb_new;
      }
      
      DEBUG({
        outs() << "bb_in => " << bb_in->getName() << "\n";

        outs() << "old exits:\n";
        exits.each([&](Instruction* before_exit, Instruction* after_exit){
          outs() << *before_exit << " (in " << before_exit->getParent()->getName() << ")\n  =>" << *after_exit << " (in " << after_exit->getParent()->getName() << ")\n";
        });
        outs() << "\n";
        outs() << "^^^^^^^^^^^^^^^\nnew exits:\n";
      });
      
      exits.each([&](Instruction* before, Instruction* after){
        auto bb_exit = before->getParent();
        BasicBlock* bb_after;
        if (bb_exit == after->getParent()) {
          std::string bbname = Twine(name+".exit." + bb_exit->getName()).str();
          bb_after = bb_exit->splitBasicBlock(after, bbname);
          DEBUG(outs() << *bb_exit->getTerminator() << " (in " << before->getParent()->getName() << ")\n  =>" << *after << " (in " << after->getParent()->getName() << ")\n");
          exits.remove(before, after);
          exits.add(bb_exit->getTerminator(), bb_after);
        } else {
          bb_after = after->getParent();
          bool found = false;
          for_each(sb, bb_exit, succ) {
            if (*sb == bb_after) {
              found = true;
              break;
            }
          }
          if (!found) {
            outs() << *bb_exit;
            outs() << *bb_after;
            old_fn->viewCFG();
          }
          assert(found && "after_exit not in an immediate successor of before_exit");
        }
      });
      DEBUG(outs() << "^^^^^^^^^^^^^^^\n");
      
      visit([&](BasicBlock::iterator it){
        DEBUG(outs() << "**" << *it << "\n");
        bbs.insert(it->getParent());
      });
      
      /////////////////////////
      // find inputs/outputs
      auto definedInRegion = [&](Value* v) {
        if (auto i = dyn_cast<Instruction>(v))
          if (bbs.count(i->getParent()))
            return true;
        return false;
      };
      auto definedInCaller = [&](Value* v) {
        if (isa<Argument>(v)) return true;
        if (auto i = dyn_cast<Instruction>(v))
          if (!bbs.count(i->getParent()))
            return true;
        return false;
      };
      
      ValueSet inputs, outputs;
      visit([&](BasicBlock::iterator it){
        for_each_op(o, *it)  if (definedInCaller(*o)) inputs.insert(*o);
        for_each_use(u, *it) if (!definedInRegion(*u)) { outputs.insert(it); break; }
      });
      
      /////////////////////////////////////////////
      // create struct types for inputs & outputs
      SmallVector<Type*,8> in_types, out_types;
      for (auto& p : inputs)  {  in_types.push_back(p->getType()); }
      for (auto& p : outputs) { out_types.push_back(p->getType()); }
      
      auto in_struct_ty = StructType::get(ctx, in_types);
      auto out_struct_ty = StructType::get(ctx, out_types);
      
      assert2(in_struct_ty == ty_input, "different input types", *ty_input, *in_struct_ty);
      if (out_struct_ty != ty_output) for (auto v : outputs) outs() << "-- " << *v << "\n";
      assert2(out_struct_ty == ty_output, "different output types", *ty_output, *out_struct_ty);
      
      /////////////////////////
      // create function shell
      auto new_fn = Function::Create(
                      FunctionType::get(ty_i16, (Type*[]){ ty_void_ptr, ty_void_ptr }, false),
                      GlobalValue::InternalLinkage, name, mod);
      
      auto bb_entry = BasicBlock::Create(ctx, name+".entry", new_fn);
      
      IRBuilder<> b(bb_entry);
      
      auto argi = new_fn->arg_begin();
      auto in_arg  = b.CreateBitCast(argi++, in_struct_ty->getPointerTo(), "struct.in");
      auto out_arg = b.CreateBitCast(argi++, out_struct_ty->getPointerTo(), "struct.out");
      
      /////////////////////////////
      // now clone blocks
      SmallDenseMap<BasicBlock*,BasicBlock*> bb_map;
      ValueToValueMapTy clone_map;
      for (auto bb : bbs) {
        bb_map[bb] = CloneBasicBlock(bb, clone_map, ".clone", new_fn);
      }
      for (auto bb : bbs) {
        assert(bb_map.count(bb));
        DEBUG(outs() << bb->getName() << " => " << bb_map[bb]->getName() << "\n");
      }
      
      ///////////////////////////
      // remap and load inputs
      for (int i=0; i < inputs.size(); i++) {
        auto v = inputs[i];
        clone_map[v] = b.CreateLoad(b.CreateGEP(in_arg, { idx(0), idx(i) }),
                                    "in." + v->getName());
      }
      
      auto bb_in_clone = cast<BasicBlock>(bb_map[bb_in]);
      b.CreateBr(bb_in_clone);
      
      auto bb_ret = BasicBlock::Create(ctx, name+".ret", new_fn);
      auto ty_ret = ty_i16;
      
      // create phi for selecting which return value to use
      // make sure it's first thing in BB
      b.SetInsertPoint(bb_ret);
      auto phi_ret = b.CreatePHI(ty_ret, exits.size(), "ret.phi");
      // return from end of created block
      b.CreateRet(phi_ret);
      
      ////////////////////////////////
      // store outputs at last use
      for (int i = 0; i < outputs.size(); i++) {
        assert(clone_map.count(outputs[i]) > 0);
        auto v = cast<Instruction>(clone_map[outputs[i]]);
        // insert at end of (cloned) block containing the (remapped) value
        b.SetInsertPoint(v->getParent()->getTerminator());
        b.CreateStore(v, b.CreateGEP(out_arg, { idx(0), idx(i) }, "out."+v->getName()));
      }
      
      /////////////////////////////////////////////////////////////////
      // (in original function)
      /////////////////////////////////////////////////////////////////
      
      // put allocas at top of function
      b.SetInsertPoint(old_fn->begin()->begin());
      auto in_alloca =  b.CreateAlloca(in_struct_ty,  0, name+".struct.in");
      auto out_alloca = b.CreateAlloca(out_struct_ty, 0, name+".struct.out");
      
      //////////////
      // emit call
      auto bb_call = BasicBlock::Create(ctx, name+".call", old_fn, bb_in);
      b.SetInsertPoint(bb_call);
      
      // replace uses of bb_in
      for_each(it, bb_in, pred) {
        auto bb = *it;
        if (bbs.count(bb) == 0) {
          bb->getTerminator()->replaceUsesOfWith(bb_in, bb_call);
        }
      }
      
      // copy inputs into struct
      for (int i = 0; i < inputs.size(); i++) {
        b.CreateStore(inputs[i], b.CreateGEP(in_alloca, {idx(0),idx(i)}, name+".gep.in"));
      }

      auto target_core = b.CreateCall(ginfo.get_core_fn, (Value*[]){
        b.CreateBitCast(target_ptr, ty_void_gptr)
      }, name+".target_core");
      
      auto call = b.CreateCall(ginfo.call_on_fn, {
        target_core, new_fn,
        b.CreateBitCast(in_alloca, ty_void_ptr),
        i64(layout.getTypeAllocSize(in_struct_ty)),
        b.CreateBitCast(out_alloca, ty_void_ptr),
        i64(layout.getTypeAllocSize(out_struct_ty))
      }, name+".call_on");
      
      auto exit_switch = b.CreateSwitch(call, bb_call, exits.size());
      
      // switch among exit blocks
      int exit_id = 0;
      exits.each([&](Instruction* before, Instruction* after){
        assert(after);
        BasicBlock *bb_after = after->getParent();
        assert(bb_after->getParent() == old_fn);
        assert(before->getParent() != bb_after);
        
        if (static_cast<Instruction*>(bb_after->begin()) != after) {
          errs() << "!! after_exit not at beginning of bb (" << name << ")\n";
          errs() << "   before_exit =>" << *before << "\n";
          errs() << "   after_exit =>" << *after << "\n";
          errs() << *bb_after;
          assert(false);
        }
        assert(before->getParent() != after->getParent());
        
        auto exit_code = ConstantInt::get(ty_ret, exit_id++);
        
        auto bb_pred = bb_map[before->getParent()];
        assert(bb_pred->getParent() == new_fn);
        
        // hook up exit from region with phi node in return block
        phi_ret->addIncoming(exit_code, bb_pred);
        
        // jump to old exit block when call returns the corresponding code
        exit_switch->addCase(exit_code, after->getParent());
        assert(exit_switch->getParent()->getParent() == old_fn);
        
        auto bb_before = before->getParent();
        bb_before->replaceSuccessorsPhiUsesWith(bb_call);
        
        // in extracted fn, remap branches outside to bb_ret
        clone_map[bb_after] = bb_ret;
      });
      
      // use clone_map to remap values in new function
      // (including branching to new bb_ret instead of exit blocks)
      
      for (auto bb : bbs) {
        assert(bb_map.count(bb));
        DEBUG(outs() << bb->getName() << " => " << bb_map[bb]->getName() << "\n");
      }

      for (auto p : bb_map) clone_map[p.first] = p.second;
      
      for_each(inst, new_fn, inst) {
        RemapInstruction(&*inst, clone_map, RF_IgnoreMissingEntries);
      }
      
      // load outputs (also rewriting uses, so have to do this *after* remap above)
      b.SetInsertPoint(exit_switch);
      for (int i = 0; i < outputs.size(); i++) {
        auto v = outputs[i];
        auto ld = b.CreateLoad(b.CreateGEP(out_alloca, {idx(0), idx(i)}), "out."+v->getName());
        v->replaceAllUsesWith(ld);
      }
      
      /////////////////////////////////
      // fixup global* uses in new_fn
      // (note: assuming any global* accesses are valid)
      SmallDenseMap<Value*,Value*> lptrs;
      
      for (auto bb = new_fn->begin(); bb != new_fn->end(); bb++) {
        for (auto inst = bb->begin(); inst != bb->end(); ) {
          Instruction *orig = inst++;
          auto prov = getProvenance(orig);
          if (isGlobalPtr(prov)) {
            if (auto gptr = ginfo.ptr_operand<GLOBAL_SPACE>(orig)) {
              ginfo.replace_with_local<GLOBAL_SPACE>(gptr, orig, lptrs);
            }
          } else if (isSymmetricPtr(prov)) {
            if (auto sptr = ginfo.ptr_operand<SYMMETRIC_SPACE>(orig)) {
              ginfo.replace_with_local<SYMMETRIC_SPACE>(sptr, orig, lptrs);
            }
          }
        }
      }
      
      ////////////////////////////////////////////////////
      // verify that all uses of these bbs are contained
      for (auto& bb : *new_fn) {
        for_each_use(u, bb) {
          if (auto ui = dyn_cast<Instruction>(*u)) {
            if (ui->getParent()->getParent() != new_fn) {
              errs() << "use =>" << *ui << "\n";
              assert(false && "!! use escaped");
            }
          }
        }
        for_each(sb, &bb, succ) {
          assert2(sb->getParent() == new_fn, "bad successor", bb, sb->getName());
        }
        for (auto& i : bb) {
          for_each_use(u, i) {
            if (auto ui = dyn_cast<Instruction>(*u)) {
              assert(ui->getParent()->getParent() == new_fn);
            }
          }
        }
      }
      
      for (auto bb : bbs) {
        for (auto& i : *bb) {
          for_each_use(u, i) {
            if (auto ui = dyn_cast<Instruction>(*u)) {
              if (!bbs.count(ui->getParent())) {
                errs() << "ui =>" << *ui << "\n";
                errs() << *ui->getParent() << "\n";
              }
            }
          }
        }
      }
      
      for (auto bb : bbs) {
        for (auto u = bb->use_begin(), ue = bb->use_end(); u != ue; u++) {
          if (auto uu = dyn_cast<Instruction>(*u)) {
            if (bbs.count(uu->getParent()) == 0) {
              auto uubb = uu->getParent();
              errs() << "use escaped => " << *uubb << *bb;
              assert(uubb->getParent() == old_fn);
              assert(false);
            }
          } else {
            errs() << "!! " << **u << "\n";
          }
        }
        for (auto& i : *bb) {
          for (auto u = i.use_begin(), ue = i.use_end(); u != ue; u++) {
            if (auto uu = dyn_cast<Instruction>(*u)) {
              if (bbs.count(uu->getParent()) == 0) {
                auto uubb = uu->getParent();
                errs() << "use escaped => " << *uubb << *bb;
                assert(uubb->getParent() == old_fn);
                assert(false);
              }
            } else {
              errs() << "!! " << **u << "\n";
            }
          }
        }
      }
      
      // delete old bbs
      for (auto bb : bbs) for (auto& i : *bb) i.dropAllReferences();
      for (auto bb : bbs) bb->eraseFromParent();
      
      DEBUG(
        outs() << "-------------------------------\n";
        outs() << *new_fn;
        outs() << "-------------------------------\n";
        outs() << *bb_call;
      );
      return new_fn;
    }
    
    void printHeader() {
      outs() << "# Candidate " << ID << ":\n";
      DEBUG(outs() << "  entry:\n  " << *entry << "\n");
      auto loc = entry->getDebugLoc();
      outs() << "  line: " << loc.getLine() << "\n";
      
      if (ty_input && ty_output) {
        outs() << "  in:  (" << format("%2d", layout.getTypeAllocSize(ty_input)) << ") "
               << *ty_input << "\n"
               << "  out: (" << format("%2d", layout.getTypeAllocSize(ty_output)) << ") "
               << *ty_output << "\n";
      }
      
      outs() << "  valid_ptrs:\n";
      for (auto p : valid_ptrs) outs() << "  " << *p << "\n";
      
      DEBUG({
        outs() << "  exits:\n";
        exits.each([&](Instruction* s, Instruction* e){
          outs() << "  " << *s << "\n     =>" << *e << "\n";
        });
        outs() << "\n";
      });
      outs() << "\n";
    }
    
    void prettyPrint(BasicBlock* bb = nullptr) {
      outs() << "~~~~~~~~~~~~~~~~~~~~~~~\n";
      printHeader();
      
      visit([&](BasicBlock::iterator i){
        if (static_cast<Instruction*>(i) == entry) {
          outs() << *i->getPrevNode() << "\n";
          outs() << "--------------------\n";
        }
        outs() << i << "\n";
        if (exits.isExitStart(i)) {
          outs() << "--------------------\n";
          outs() << *i->getNextNode() << "\n";
        }
      });
    }
    
    static void dotBB(raw_ostream& o, CandidateMap& candidates, BasicBlock* bb, CandidateRegion* this_region = nullptr) {
      o << "  \"" << bb << "\" [label=<\n";
      o << "  <table cellborder='0' border='0'>\n";
      o << "    <tr><td align='left'>" << bb->getName() << "</td></tr>\n";
      
      for (auto& i : *bb) {
        std::string s;
        { std::string _s; raw_string_ostream os(_s);
          os << i;
          s = DOT::EscapeString(os.str());
        }
        
        std::string cell_open  = "<tr><td align='left'>";
        std::string cell_close = "</td></tr>";
        
        std::string font_tag;
        { std::string _s; raw_string_ostream os(_s);
          os << "<font face='Inconsolata LGC' point-size='11'";
          if (candidates[&i])
            os << " color='" << getColorString(candidates[&i]->ID) << "'";
          os << ">";
          font_tag = os.str();
        }
        
        s = cell_open + font_tag + s + "</font></td></tr>";
        
        std::string newline = "</font>" + cell_close + "\n" + cell_open + font_tag;
        
        size_t p;
        while ((p = s.find("\\n")) != std::string::npos)
          s.replace(p, 2, newline);
        
        for (auto fn : (StringRef[]){"@grappa_get_core", "@grappa_get_pointer", "@grappa_get_pointer_symmetric"}) {
          auto fns = (fn+"(").str();
          auto fnc = ("<font color='blue'>"+fn+"</font>").str();
          while ((p = s.find(fns, p+fnc.size())) != std::string::npos)
            s.replace(p, fn.size(), fnc);
        }
        
        o << s << "\n";
      }
      
      o << "  </table>\n";
      o << "  >];\n";
      
      for_each(sb, bb, succ) {
        o << "  \"" << bb << "\"->\"" << *sb << "\";\n";
        if (this_region && candidates[sb->begin()] == this_region) {
          dotBB(o, candidates, *sb, this_region);
        }
      }
    }
    
    void dumpToDot() {
      Function& F = *entry->getParent()->getParent();
      
      auto s = F.getParent()->getModuleIdentifier();
      auto base = s.substr(s.rfind("/")+1);
      
      std::string _s;
      raw_string_ostream fname(_s);
      fname <<  "dots/" << base << "." << ID << ".dot";
      
      std::string err;
      outs() << "dot => " << fname.str() << "\n";
      raw_fd_ostream o(fname.str().c_str(), err);
      if (err.size()) {
        errs() << "dot error: " << err;
      }
      
      o << "digraph Candidate {\n";
      o << "  node[shape=record];\n";
      dotBB(o, owner, entry->getParent());
      o << "}\n";
      
      o.close();
    }
    
    static void dumpToDot(Function& F, CandidateMap& candidates, const Twine& name) {
      auto s = F.getParent()->getModuleIdentifier();
      auto base = s.substr(s.rfind("/")+1);
      
      std::string _s;
      raw_string_ostream fname(_s);
      fname <<  "dots/" << base << "." << name << ".dot";
      
      std::string err;
      outs() << "dot => " << fname.str() << "\n";
      raw_fd_ostream o(fname.str().c_str(), err);
      if (err.size()) {
        errs() << "dot error: " << err;
      }
      
      o << "digraph TaskFunction {\n";
      o << "  fontname=\"Inconsolata LGC\";\n";
      o << "  label=\"" << demangle(F.getName()) << "\"";
      o << "  node[shape=record];\n";
      
      for (auto& bb : F) {
        dotBB(o, candidates, &bb);
      }
      
      o << "}\n";
      
      o.close();
    }
    
  };
  
  int fixupFunction(Function* fn, GlobalPtrInfo& ginfo) {
    int fixed_up = 0;
    SmallDenseMap<Value*,Value*> lptrs;
    
    for (auto& bb : *fn ) {
      for (auto inst = bb.begin(); inst != bb.end(); ) {
        Instruction *orig = inst++;
        auto prov = getProvenance(orig);
        if (isGlobalPtr(prov)) {
          if (auto gptr = ginfo.ptr_operand<GLOBAL_SPACE>(orig)) {
            assert(!gptr && "!! too bad -- should do put/get\n");
          }
        } else if (isSymmetricPtr(prov)) {
          if (auto sptr = ginfo.ptr_operand<SYMMETRIC_SPACE>(orig)) {
            ginfo.replace_with_local<SYMMETRIC_SPACE>(sptr, orig, lptrs);
            fixed_up++;
          }
        }
      }
    }
    return fixed_up;
  }
  
  long CandidateRegion::id_counter = 0;
  
  bool ExtractorPass::runOnModule(Module& M) {
    
    auto layout = new DataLayout(&M);
    
//    if (! ginfo.init(M) ) return false;
    bool found_functions = ginfo.init(M);
    if (!found_functions)
      outs() << "Didn't find Grappa primitives, disabling extraction.\n";
    
    //////////////////////////
    // Find 'task' functions
    for (auto& F : M) {
      if (F.hasFnAttribute("async")) {
        task_fns.insert(&F);
      }
    }
    
    CandidateMap candidate_map;
    int ct = 0;
    
    UniqueQueue<Function*> worklist;
    for (auto fn : task_fns) worklist.push(fn);
    
    struct DbgRemover : public InstVisitor<DbgRemover> {
      void visitIntrinsicInst(IntrinsicInst& i) {
        if (i.getCalledFunction()->getName() == "llvm.dbg.value") {
          i.eraseFromParent();
        }
      }
    } dbg_remover;
    
    while (!worklist.empty()) {
      auto fn = worklist.pop();
      
      dbg_remover.visit(fn);
            
      AnchorSet anchors;
      analyzeProvenance(*fn, anchors);
      
      std::map<Value*,CandidateRegion*> candidates;
      std::vector<CandidateRegion*> cnds;
      
      for (auto a : anchors) {
        auto p = getProvenance(a);
        if (candidate_map[a]) {
          DEBUG({
            outs() << "anchor already in another delegate:\n";
            outs() << "  anchor =>" << *a << "\n";
            outs() << "  other  =>" << *candidate_map[a]->entry << "\n";
          });
        } else if (isGlobalPtr(p)) {

          auto r = new CandidateRegion(p, a, candidate_map, ginfo, *layout);
          r->valid_ptrs.insert(p);
          
          r->expandRegion();
          
          r->printHeader();
          
          r->visit([&](BasicBlock::iterator i){
            if (candidate_map[i] != r) {
              errs() << "!! bad visit: " << *i << "\n";
              assert(false);
            }
          });
          
          candidates[a] = r;
          cnds.push_back(r);
        }
      }
      
      auto taskname = "task" + Twine(ct++);
      if (cnds.size() > 0 && PrintDot) {
        CandidateRegion::dumpToDot(*fn, candidate_map, taskname);
      }
      
      for_each(it, *fn, inst) {
        auto inst = &*it;
        if (isa<CallInst>(inst) || isa<InvokeInst>(inst)) {
          CallSite cs(inst);
          if (cs.getCalledFunction()) worklist.push(cs.getCalledFunction());
        }
      }
      
      if (found_functions && cnds.size() > 0) {
        for (auto cnd : cnds) {
          
          auto new_fn = cnd->extractRegion();
          
          if (PrintDot) {
            CandidateRegion::dumpToDot(*new_fn, candidate_map, taskname+".d"+Twine(cnd->ID));
//            CandidateRegion::dumpToDot(*fn, candidate_map, taskname+".after.d"+Twine(cnd->ID));
          }
        }
      }
      
      if (found_functions) {
        int nfixed = fixupFunction(fn, ginfo);
        
        if ((nfixed || cnds.size()) && PrintDot) {
          CandidateRegion::dumpToDot(*fn, candidate_map, taskname+".after");
        }
      }
      
      for (auto c : cnds) delete c;
    }
    
    ////////////////////////////////////////////////
    // fixup any remaining global/symmetric things
    if (found_functions) for (auto& F : M) {
      fixupFunction(&F, ginfo);
    }
    
    outs().flush();
    return true;
  }
    
  bool ExtractorPass::doInitialization(Module& M) {
    outs() << "-- Grappa Extractor --\n";
    outs().flush();
    return false;
  }
  
  bool ExtractorPass::doFinalization(Module& M) {
    outs() << "---\n";
    outs().flush();
    return true;
  }
  
  char ExtractorPass::ID = 0;
  
  //////////////////////////////
  // Register optional pass
  static RegisterPass<ExtractorPass> X( "grappa-ex", "Grappa Extractor", false, false );
  
}
