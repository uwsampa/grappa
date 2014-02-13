
#undef DEBUG

#include <llvm/Support/Debug.h>
#include <llvm/IR/Constants.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/CallSite.h>
#include <llvm/Support/GraphWriter.h>
#include <llvm/InstVisitor.h>
#include <llvm/Support/CommandLine.h>

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
  
  
  struct CandidateRegion;
  using CandidateMap = std::map<Instruction*,CandidateRegion*>;
  
  struct CandidateRegion {
    static long id_counter;
    long ID;
    
    Instruction* entry;
    
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
    
    CandidateRegion(Value* target_ptr, Instruction* entry, CandidateMap& owner):
      ID(id_counter++), entry(entry), target_ptr(target_ptr), owner(owner) {}
    
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
                outs() << "!! deferring -- invalid preds:\n" << *bb << "\n" << **pb;
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
      } else {
        return true;
      }
    }
    
    Function* extractRegion(GlobalPtrInfo& ginfo, DataLayout& layout) {
      auto name = "d" + Twine(ID);
      outs() << "--------------- extracting " << name << " ----------------\n";
      errs() << "target_ptr =>" << *target_ptr << "\n";
      
      auto mod = entry->getParent()->getParent()->getParent();
      auto& ctx = entry->getContext();
      auto ty_i16 = Type::getInt16Ty(ctx);
      auto ty_void_ptr = Type::getInt8PtrTy(ctx);
      auto ty_void_gptr = Type::getInt8PtrTy(ctx, GLOBAL_SPACE);
      auto i64 = [&](int64_t v) { return ConstantInt::get(Type::getInt64Ty(ctx), v); };
      auto idx = [&](int i) { return ConstantInt::get(Type::getInt32Ty(ctx), i); };
      
      SmallSet<BasicBlock*,8> bbs;
      
      outs() << "pre exits:\n";
      exits.each([&](Instruction* before_exit, Instruction* after_exit){
        outs() << *before_exit << " (in " << before_exit->getParent()->getName() << ")\n  =>" << *after_exit << " (in " << after_exit->getParent()->getName() << ")\n";
      });
      outs() << "\n";
      
      if (exits.size() > 1) {
        SmallSetVector<Instruction*,8> preds;
        exits.each([&](Instruction* before, Instruction* after){
          preds.insert(before);
        });
        if (preds.size() == 1) {
          exits.clear();
          auto p = BasicBlock::iterator(preds[0])->getPrevNode();
          outs() << "@bh unique_pred =>" << *preds[0] << "\n";
          exits.add(p);
        }
      }
      
      //////////////////////////////////////////////////////////////
      // first slice and dice at boundaries and build up set of BB's
      auto bb_in = entry->getParent();
      outs() << "entry =>" << *entry << "\n";
      outs() << "bb_in => " << bb_in->getName() << "\n";

      auto old_fn = bb_in->getParent();
      
      if (BasicBlock::iterator(entry) != bb_in->begin()) {
        auto bb_new = bb_in->splitBasicBlock(entry, name+".eblk");
        bb_in = bb_new;
      }
      outs() << "bb_in => " << bb_in->getName() << "\n";

      outs() << "old exits:\n";
      exits.each([&](Instruction* before_exit, Instruction* after_exit){
        outs() << *before_exit << " (in " << before_exit->getParent()->getName() << ")\n  =>" << *after_exit << " (in " << after_exit->getParent()->getName() << ")\n";
      });
      outs() << "\n";
      outs() << "^^^^^^^^^^^^^^^\nnew exits:\n";
      exits.each([&](Instruction* before, Instruction* after){
        auto bb_exit = before->getParent();
        BasicBlock* bb_after;
        if (bb_exit == after->getParent()) {
          std::string bbname = Twine(name+".exit." + bb_exit->getName()).str();
          bb_after = bb_exit->splitBasicBlock(after, bbname);
          outs() << *bb_exit->getTerminator() << " (in " << before->getParent()->getName() << ")\n  =>" << *after << " (in " << after->getParent()->getName() << ")\n";
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
      outs() << "^^^^^^^^^^^^^^^\n";
      
      visit([&](BasicBlock::iterator it){
        outs() << "**" << *it << "\n";
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
      ValueToValueMapTy clone_map;
      for (auto bb : bbs) {
        clone_map[bb] = CloneBasicBlock(bb, clone_map, ".clone", new_fn);
      }
      for (auto bb : bbs) {
        assert(clone_map.count(bb));
        outs() << bb->getName() << " => " << clone_map[bb]->getName() << "\n";
      }
      
      ///////////////////////////
      // remap and load inputs
      for (int i=0; i < inputs.size(); i++) {
        auto v = inputs[i];
        clone_map[v] = b.CreateLoad(b.CreateGEP(in_arg, { idx(0), idx(i) }),
                                    "in." + v->getName());
      }
      
      auto bb_in_clone = cast<BasicBlock>(clone_map[bb_in]);
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
        
        if (clone_map.count(before->getParent()) == 0) {
          outs() << "-------------------\n";
          assert(before->getParent()->getParent() == old_fn);
          for (auto bb : bbs) outs() << bb << " -- " << clone_map.count(bb) << "\n";
          outs() << "before_exit =>" << *before << " (in " << before->getParent() << ")\n";
          assert(false);
        }
        auto bb_pred = cast<BasicBlock>(clone_map[before->getParent()]);
        assert(bb_pred->getParent() == new_fn);
        
        // hook up exit from region with phi node in return block
        phi_ret->addIncoming(exit_code, bb_pred);
        
        // jump to old exit block when call returns the corresponding code
        exit_switch->addCase(exit_code, after->getParent());
        assert(exit_switch->getParent()->getParent() == old_fn);
        
        auto bb_before = before->getParent();
        assert(bbs.count(bb_before));
        bb_before->replaceAllUsesWith(bb_call);
        
        // in extracted fn, remap branches outside to bb_ret
        clone_map[bb_after] = bb_ret;
      });
      
      // use clone_map to remap values in new function
      // (including branching to new bb_ret instead of exit blocks)
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
      SmallDenseMap<Value*,Value*> lptrs;
      SmallSet<Value*, 4> new_valid_ptrs;
      for (auto p : valid_ptrs)
        if (clone_map.count(p))
          new_valid_ptrs.insert(clone_map[p]);
      for (auto p : new_valid_ptrs)
        valid_ptrs.insert(p);
      
      for (auto bb = new_fn->begin(); bb != new_fn->end(); bb++) {
        for (auto inst = bb->begin(); inst != bb->end(); ) {
          Instruction *orig = inst++;
          auto prov = getProvenance(orig);
          if (prov && valid_ptrs.count(prov)) {
            if (auto gptr = ginfo.global_ptr_operand(orig)) {
              ginfo.replace_global_with_local(gptr, orig, lptrs);
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
          assert(sb->getParent() == new_fn);
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
          }
        }
      }

      // delete old bbs
      for (auto bb : bbs) for (auto& i : *bb) i.dropAllReferences();
      for (auto bb : bbs) bb->eraseFromParent();
      
      outs() << "-------------------------------\n";
      outs() << *new_fn;
      outs() << "-------------------------------\n";
      outs() << *bb_call;
      
      return new_fn;
    }
    
    void printHeader() {
      outs() << "Candidate " << ID << ":\n";
      outs() << "  entry:\n  " << *entry << "\n";
      outs() << "  valid_ptrs:\n";
      for (auto p : valid_ptrs) outs() << "  " << *p << "\n";
      outs() << "  exits:\n";
      exits.each([&](Instruction* s, Instruction* e){
        outs() << "  " << *s << "\n     =>" << *e << "\n";
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
          os << "<font face='Inconsolata LGC' point-size='10'";
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
  
  
  struct MetadataVerifier : public InstVisitor<MetadataVerifier> {
    
    SmallPtrSet<MDNode*,32> mdvisited;
    
    void visitInstruction(Instruction &i) {
      for_each_op(o, i)
        if (auto m = dyn_cast<MDNode>(*o))
          visitMDNode(*m, i.getParent()->getParent());
    }
    
    void visitMDNode(MDNode &m, Function *fn) {
      if (!mdvisited.insert(&m)) return;
      
      for (unsigned i = 0, e = m.getNumOperands(); i != e; ++i) {
        Value *o = m.getOperand(i);
        
        if (!o)
          continue;
        if (isa<Constant>(o) || isa<MDString>(o))
          continue;
        if (MDNode *om = dyn_cast<MDNode>(o)) {
          visitMDNode(*om, fn);
          continue;
        }
        
        if (m.isFunctionLocal()) {
          // If this was an instruction, bb, or argument, verify that it is in the
          // function that we expect.
          Function *ActualF = 0;
          if (Instruction *I = dyn_cast<Instruction>(o))
            ActualF = I->getParent()->getParent();
          else if (BasicBlock *BB = dyn_cast<BasicBlock>(o))
            ActualF = BB->getParent();
          else if (Argument *A = dyn_cast<Argument>(o))
            ActualF = A->getParent();
          
          if (ActualF && ActualF != fn) {
            outs() << "Function-local metatdata in wrong function:\n";
            outs() << *o << "\n";
            outs() << m << "\n";
          } else {
            outs() << ".";
          }
        }
      }
    }
  };
  
  long CandidateRegion::id_counter = 0;
  
  bool ExtractorPass::runOnModule(Module& M) {
    outs() << "Running extractor...\n";
    bool changed = false;
    
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
    
    outs() << "task_fns.count => " << task_fns.size() << "\n";
    outs().flush();
//    std::vector<DelegateExtractor*> candidates;
    
    MetadataVerifier verifier;
    
    CandidateMap candidate_map;
    int ct = 0;
    
    UniqueQueue<Function*> worklist;
    for (auto fn : task_fns) worklist.push(fn);
    
    while (!worklist.empty()) {
      auto fn = worklist.pop();
      
      AnchorSet anchors;
      analyzeProvenance(*fn, anchors);
      
      
      std::map<Value*,CandidateRegion*> candidates;
      std::vector<CandidateRegion*> cnds;
      
      for (auto a : anchors) {
        auto p = getProvenance(a);
        if (candidate_map[a]) {
          outs() << "anchor already in another delegate:\n";
          outs() << "  anchor =>" << *a << "\n";
          outs() << "  other  =>" << *candidate_map[a]->entry << "\n";
        } else if (isGlobalPtr(p)) {
          auto r = new CandidateRegion(p, a, candidate_map);
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
          auto new_fn = cnd->extractRegion(ginfo, *layout);
//          CandidateRegion::dumpToDot(*fn, candidate_map, taskname+".after." + new_fn->getName());
          
          if (PrintDot) {
            CandidateRegion::dumpToDot(*new_fn, candidate_map, taskname+".d"+Twine(cnd->ID));
          }
          outs() << "verifying"; verifier.visit(new_fn); outs() << "done\n";
        }
        if (PrintDot) CandidateRegion::dumpToDot(*fn, candidate_map, taskname+".after");
        outs() << "verifying";
        verifier.visit(fn);
        outs() << "done\n";
      }
      
      for (auto c : cnds) delete c;
      
    }
    
    outs().flush();
    
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
