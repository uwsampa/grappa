
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
#include <llvm/Analysis/AliasSetTracker.h>
#include <llvm/IR/Verifier.h>

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
static cl::opt<int> OnlyLine("only-line", cl::desc("Print dot just for a specific line."));

static cl::opt<bool> DoExtractor("grappa-extractor",
                                 cl::desc("Run pass to automatically extract delegates."));
static cl::opt<bool> DisableAsync("disable-async",
                                 cl::desc("Disable detection/creationg of async delegates."));

using InstructionSet = SmallPtrSet<Instruction*,16>;

using RegionSet = SmallSetVector<Instruction*,64>;

int poorMansInlineCost(Function* fn) {
  return fn->size();
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

Value* search(Value* v) {
  if (auto inst = dyn_cast<Instruction>(v))
    if (auto prov = getProvenance(inst))
      return prov;
  
  if (auto gep = dyn_cast<GetElementPtrInst>(v)) {
    if (!gep->isInBounds()) return v;
    if (gep->hasIndices()) {
      if (gep->getPointerAddressSpace() == GLOBAL_SPACE) {
        auto idx = gep->getOperand(1);
        if (idx != ConstantInt::get(idx->getType(), 0)) {
          setProvenance(gep, v);
          return v;
        }
      }
    }
    auto p = search(gep->getPointerOperand());
    if (p) setProvenance(gep, p);
    return p;
  }
  if (auto c = dyn_cast<CastInst>(v)) {
    auto vv = search(c->getOperand(0));
    if (isa<PointerType>(vv->getType())) {
      setProvenance(c, vv);
      return vv;
    } else {
      setProvenance(c, v);
      return v;
    }
  }
  if (isa<Constant>(v)) {
    return v;
  }
  
  if (auto c = dyn_cast<CallInst>(v)) {
    if (auto prov = getProvenance(c)) {
      if (isa<PointerType>(c->getType())) {
        setProvenance(c, prov);
        return prov;
      }
    }
  }
  
  if (auto i = dyn_cast<Instruction>(v)) {
    setProvenance(i, v);
  }
  return v;
}

#define X(expr) if (!v) return false; else return (expr);

bool isGlobalPtr(Value* v)    { X(dyn_cast_addr<GLOBAL_SPACE>(v->getType())); }
bool isSymmetricPtr(Value* v) { X(dyn_cast_addr<SYMMETRIC_SPACE>(v->getType())); }
bool isStatic(Value* v)       { X(isa<GlobalVariable>(v)); }
bool isConst(Value* v)        { X(isa<Constant>(v) || isa<BasicBlock>(v)); }
bool isStack(Value* v)        { X(isa<AllocaInst>(v) || isa<Argument>(v)); }

#undef X

bool isAnchor(Instruction* inst, int space = -1) {
  if (!inst->mayReadOrWriteMemory()) return false;
  
  Value* prov = getProvenance(inst);
  if (prov) {
    if (space == GLOBAL_SPACE) {
      return isGlobalPtr(prov);
    } else {
      return isGlobalPtr(prov) || isStack(prov);
    }
  }
  return false;
}

void analyzeProvenance(Function& fn, AnchorSet& anchors) {
  for (auto& bb: fn) {
    for (auto& i : bb) {
      
      Value *prov = getProvenance(&i);
      
      if (!prov) {
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
//              setProvenance(m, prov);
              
            }
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


void remap(Instruction* inst, ValueToValueMapTy& vmap,
           SmallVectorImpl<Instruction*>& to_delete) {
  
  auto map_value = [&vmap](Value *v) {
    auto v_it = vmap.find(v);
    if (v_it != vmap.end() && v_it->second) {
      return static_cast<Value*>(v_it->second);
    } else {
      return v;
    }
  };
  
  //////////////////////////////////////////////////
  // RemapInstruction() breaks 'CompileUnit' info,
  // causing DwardDebug pass to crash...
  // RemapInstruction(inst, vmap, RF_IgnoreMissingEntries);
  
  // poor-man's RemapInstruction (just do operands)
  for_each_op(op, *inst) {
    *op = map_value(*op);
  }
  
  if (auto bc = dyn_cast<BitCastInst>(inst)) {
    if (bc->getType()->getPointerAddressSpace() !=
        bc->getSrcTy()->getPointerAddressSpace()) {
      auto new_bc = new BitCastInst(bc->getOperand(0),
                                    PointerType::get(bc->getType()->getPointerElementType(),
                                                     bc->getSrcTy()->getPointerAddressSpace()),
                                    inst->getName()+".fix", inst);
      vmap[bc] = new_bc;
      to_delete.push_back(bc);
    }
  } else if (auto gep = dyn_cast<GetElementPtrInst>(inst)) {
    if (gep->getPointerAddressSpace() != gep->getType()->getPointerAddressSpace()) {
      SmallVector<Value*,4> idxs;
      for (auto i=1; i<gep->getNumOperands(); i++)
        idxs.push_back(map_value(gep->getOperand(i)));
      auto new_gep = GetElementPtrInst::Create(map_value(gep->getPointerOperand()), idxs,
                                               gep->getName()+"fix", gep);
      if (gep->isInBounds()) new_gep->setIsInBounds();
      vmap[gep] = new_gep;
      to_delete.push_back(gep);
    }
  } else if (auto store = dyn_cast<StoreInst>(inst)) {
    auto ptr_elt_ty = store->getPointerOperand()->getType()->getPointerElementType();
    if (ptr_elt_ty != store->getValueOperand()->getType()) {
      auto val = store->getValueOperand();
      IRBuilder<> b(store);
      auto cast_val = SmartCast(b, val, ptr_elt_ty,
                                val->getName()+".recast");
      store->setOperand(0, cast_val);
    }

  }
  
}

CallInst* globalizeCall(Function* old_fn, AddrSpaceCastInst* cast,
                        CallInst* call, DataLayout* layout,
                        ValueToValueMapTy& vmap,
                        std::set<int>* lines) {
  auto mod = old_fn->getParent();
  auto ptr = cast->getOperand(0);
  auto xptr_ty = ptr->getType();
  
  outs() << "!! globalizing <" << call->getDebugLoc().getLine() << ">" << *call << "\n";
  if (lines) lines->insert(call->getDebugLoc().getLine());
  
  auto name = old_fn->getName() + (xptr_ty->getPointerAddressSpace() == GLOBAL_SPACE
                                   ? ".global" : ".symmetric");
  
  auto new_fn = mod->getFunction(name.str());
  
  if (!new_fn) {
    
    int arg_idx = -1;
    for (int i=0; i < call->getNumArgOperands(); i++)
      if (call->getArgOperand(i) == cast)
        arg_idx = i;
    if (arg_idx == -1) return nullptr;
    
    if (!call->getCalledFunction()) return nullptr;
    
    auto fn_ty = old_fn->getFunctionType();
    outs() << "old_fn.ty => " << *old_fn->getType() << "\n";
    
    SmallVector<Type*,8> arg_tys;
    int i=0;
    for (auto arg_it = old_fn->arg_begin();
         arg_it != old_fn->arg_end();
         arg_it++, i++) {
      if (i == arg_idx) arg_tys.push_back(ptr->getType());
      else arg_tys.push_back(arg_it->getType());
    }
    
    auto ret_ty = fn_ty->getReturnType();
    if (auto ptr_ty = dyn_cast<PointerType>(ret_ty)) {
      if (ptr_ty->getAddressSpace() == 0) {
        ret_ty = PointerType::get(ptr_ty->getElementType(), GLOBAL_SPACE);
      }
    }
    
    auto new_fn_ty = FunctionType::get(ret_ty, arg_tys, fn_ty->isVarArg());
    
    outs() << "new_fn.ty => " << *new_fn_ty << "\n";
    
    new_fn = Function::Create(new_fn_ty, old_fn->getLinkage(),
                                   name, old_fn->getParent());
    
    auto OldAttrs = old_fn->getAttributes();
    new_fn->setAttributes(new_fn->getAttributes()
                           .addAttributes(new_fn->getContext(),
                                          AttributeSet::FunctionIndex,
                                          OldAttrs.getFnAttributes()));

    
    IRBuilder<> b(BasicBlock::Create(old_fn->getContext(), "entry", new_fn));
    
    Argument* ptr_arg;
    {
      auto a = new_fn->arg_begin();
      for (int i=0; i<arg_idx; i++) a++;
      ptr_arg = a;
    }
    auto recast = b.CreateAddrSpaceCast(ptr_arg, cast->getType(), "recast");
    
    SmallVector<Value*,8> args;
    i = 0;
    for (auto a = new_fn->arg_begin(); a != new_fn->arg_end(); a++, i++) {
      if (i == arg_idx) {
        args.push_back(recast);
      } else {
        args.push_back(a);
      }
    }
    
    auto new_call = b.CreateCall(old_fn, args);
    
    if (new_call->getType()->isVoidTy())
      b.CreateRetVoid();
    else
      b.CreateRet(new_call);
    
    InlineFunctionInfo info(nullptr, layout);
    auto inlined = InlineFunction(new_call, info);
    if (!inlined) return nullptr;
    
    AnchorSet anchors;
    analyzeProvenance(*new_fn, anchors);
    
    
    ValueToValueMapTy vmap;
    vmap[recast] = ptr_arg;
    
    // remap instructions/types to be global
    SmallVector<Instruction*, 64> to_delete;
    for (auto& bb : *new_fn) {
      for (auto it = bb.begin(); it != bb.end();) {
        Instruction *inst = it++;
        remap(inst, vmap, to_delete);
      }
    }
    
    for (auto inst : to_delete) inst->eraseFromParent();
  }
  
  ///////////////////////////////////////////
  // update old call to use the new function
  if (new_fn->getReturnType() == call->getType()) {
    call->setCalledFunction(new_fn);
    for_each_op(op, *call) if (*op == cast) *op = ptr;
  } else {
    SmallVector<Value*,8> args;
    for (int i=0; i<call->getNumArgOperands(); i++) {
      Value *a = call->getArgOperand(i);
      args.push_back( a == cast ? ptr : a );
    }
    IRBuilder<> b(call);
    auto c = b.CreateCall(new_fn, args);
    SmallVector<Instruction*,16> to_delete;
    vmap[call] = c;
    outs() << "new_call =>" << *c << "\n";
    for_each_use(u, *call) {
      if (auto inst = dyn_cast<Instruction>(*u)) {
        remap(inst, vmap, to_delete);
      }
    }
    call->eraseFromParent();
    call = c;
  }
  
  return call;
}

int preFixup(Function* fn, DataLayout* layout) {
  SmallVector<AddrSpaceCastInst*,32> casts;
  
  for (auto& bb : *fn) {
    for (auto it = bb.begin(); it != bb.end(); ) {
      Instruction *inst = it++;
      
      if (isa<AddrSpaceCastInst>(inst) && isGlobalPtr(inst->getOperand(0))) {
        casts.push_back(cast<AddrSpaceCastInst>(inst));
      } else {
        for_each_op(op, *inst) {
          // find addrspacecasts hiding in constexprs & unpack them
          if (auto c = dyn_cast<ConstantExpr>(*op)) {
            if (c->isCast() && strcmp(c->getOpcodeName(), "addrspacecast") == 0) {
              auto ac = new AddrSpaceCastInst(c->getOperand(0), c->getType(),
                                              "cast.unpacked", inst);
              *op = ac;
              casts.push_back(ac);
            }
          }
        }
      }
    }
  }
  
  ValueToValueMapTy vmap;
  
  for (auto c : casts) {
    outs() << "~~~~~~~~~~~\n" << *c << "\n";
    Value *ptr = c->getOperand(0);
    
    SmallVector<Value*,16> cast_uses;
    for_each_use(u, *c) cast_uses.push_back(*u);
    
    ///////////////////////////////////////////////////
    // factor out bitcast if element type changes, too
    auto src_space = c->getSrcTy()->getPointerAddressSpace();
    auto src_elt_ty = c->getSrcTy()->getPointerElementType();
    auto dst_elt_ty = c->getType()->getPointerElementType();
    if (src_elt_ty != dst_elt_ty) {
      ptr = new BitCastInst(c->getOperand(0),
                            PointerType::get(dst_elt_ty, src_space),
                            c->getName()+".precast", c);
      c->setOperand(0, ptr);
      outs() << "****" << *ptr << "\n****" << *c << "\n";
    }
    
    // will remap addrspacecast -> original global ptr
    vmap[c] = ptr;
    
    // find any calls using the cast value (so we can inline them below)
    
    for (auto v : cast_uses) {
      outs() << ">>>> " << *v << "\n";
      if (auto call = dyn_cast<CallInst>(v)) {
        auto called_fn = call->getCalledFunction();
        if (!called_fn) continue;
        DEBUG(outs() << "++" << *call << "\n");
        
        call = globalizeCall(called_fn, c, call, layout, vmap);
        
        if (poorMansInlineCost(called_fn) < 25) {
          InlineFunctionInfo info;
          InlineFunction(call, info);
        }
        
//        assertN(new_fn, "unable to globalize!", *call, *new_fn);
        
      }
    }
    
  }
  
  // recompute provenance with new inlined instructions
  AnchorSet anchors;
  analyzeProvenance(*fn, anchors);
  
  // remap instructions/types to be global
  SmallVector<Instruction*, 64> to_delete;
  for (auto& bb : *fn) {
    for (auto it = bb.begin(); it != bb.end();) {
      Instruction *inst = it++;
      remap(inst, vmap, to_delete);
    }
  }
  for (auto inst : to_delete) inst->eraseFromParent();
  
  for (auto c : casts) c->eraseFromParent();
  
  return casts.size();
}

namespace Grappa {
  
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
      using Exit = std::pair<WeakVH,WeakVH>;
      
    public:
      
      int score;
      
      SmallSetVector<Exit,8> s;
      
      ExitMap() = default;
      
      ExitMap(ExitMap const& o): score(o.score) {
        for (auto e : o.s) s.insert(e);
      }
      
      void operator=(const ExitMap& o) {
        clear();
        score = o.score;
        for (auto e : o.s) s.insert(e);
      }
      
      void clear() { s.clear(); score = 0; }
      
      bool isVoidRetExit() {
        if (DisableAsync) return false;
        
        bool all_void = true;
        
        each([&](Instruction* s, Instruction* e){
          auto ret = dyn_cast<ReturnInst>(e);
          if (ret == nullptr || ret->getReturnValue()) { all_void = false; }
        });
        
        return all_void;
      }
      
      template< typename F >
      void each(F yield) const {
        SmallVector<Exit,8> v;
        for (auto p : s) v.push_back(p);
        
        for (auto p : v) {
          if (auto before = dyn_cast<Instruction>(p.first)) {
            yield(before, before->getNextNode());
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
        s.insert({succ, pred});
      }
      
      void remove(Instruction* before, Instruction* after) {
        auto bb_after = after->getParent();
        for (auto p : s) {
          if (p.first == bb_after || p.first == before) {
            s.remove(p);
            return;
          }
        }
        assertN(false, "trying to remove non-existent exit", *before, *after);
      }
      
      void add(Instruction* before) {
        assert(before);
        s.insert({before, nullptr});
      }
      
      bool isExitStart(Instruction* i) const {
        if (s.count(Exit{i,nullptr})) {
          return true;
        } else if (isa<TerminatorInst>(i)) {
          auto pred = i->getParent();
          for (auto p : s)
            if (p.second == pred)
              return true;
        }
        return false;
      }
      
      bool isExitEnd(BasicBlock* b) const {
        for (auto p : s) {
          if (p.first == b) return true;
        }
        return false;
      }
      
      unsigned size() { return s.size(); }
      
      void fromRegion(RegionSet& region, int pre_score){
        clear();
        score = pre_score;
        
        UniqueQueue<Instruction*> q;
        q.push(region[0]);
        
        while (!q.empty()) {
          auto j = q.pop();
          auto jb = j->getParent();
          if (!region.count(j)) {
            this->add(j->getPrevNode());
          } else {
            if (isa<TerminatorInst>(j)) {
              for_each(sb, jb, succ) {
                auto js = (*sb)->begin();
                if (!region.count(js)) {
                  // outs() << "---- " << j->getName() << " => " << (*sb)->getName() << "\n";
                  this->add(j, *sb);
                } else {
                  q.push(js);
                }
              }
            } else {
              q.push(j->getNextNode());
            }
          }
        }
        
        if (isVoidRetExit()) score -= MESSAGE_COST;
      }

    };
    
    ExitMap exits, max_extent;
    
    WeakVH target_ptr;
    SmallSet<Value*,4> valid_ptrs;
    
    CandidateMap& owner;
    
    GlobalPtrInfo& ginfo;
    DataLayout& layout;
    AliasAnalysis& alias;
    
    InstructionSet hoists;
    SmallPtrSet<AllocaInst*,4> to_localize;
    
    CandidateRegion(Value* target_ptr, Instruction* entry, CandidateMap& owner, GlobalPtrInfo& ginfo, DataLayout& layout, AliasAnalysis& alias):
      ID(id_counter++), entry(entry), target_ptr(target_ptr), owner(owner), ginfo(ginfo), layout(layout), alias(alias) {
        valid_ptrs.insert(target_ptr);
      }
    
    int cost() { return exits.score; }
    
    void addPtr(Value* ptr) { valid_ptrs.insert(ptr); }
    
    void switchExits(ExitMap& emap) {
      visit([&](BasicBlock::iterator i){ owner[i] = nullptr; });
      
      exits = emap;
      
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
      
      visit([&](BasicBlock::iterator i){ owner[i] = this; });

      computeInputsOutputs();
    }
    
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
    
    bool hoistable(Instruction *i, RegionSet& region, AliasSetTracker& aliases,
                   InstructionSet& unhoistable, InstructionSet& tomove,
                   SmallPtrSet<AllocaInst*,4>& allocas_to_localize) {
      if (unhoistable.count(i)) return false;
      if (i->mayHaveSideEffects()) return false;
      auto prov = getProvenance(i);
      if (i == entry || (prov && prov == getProvenance(entry))) return false;
      
      auto& analysis = aliases.getAliasAnalysis();
      Value *ptr = nullptr;
      
      if (auto l = dyn_cast<LoadInst>(i))       ptr = l->getPointerOperand();
      else if (auto s = dyn_cast<StoreInst>(i)) ptr = s->getPointerOperand();
      
      if (ptr) {
        auto& alias_set = aliases.getAliasSetForPointer(ptr,
                           analysis.getTypeStoreSize(ptr->getType()),
                           i->getMetadata(LLVMContext::MD_tbaa));
        
        // only potentially a problem if something in the set modifies it
        if (alias_set.isMod()) {
          auto ptr_space = ptr->getType()->getPointerAddressSpace();
          for (auto a : alias_set) {
            auto av = a.getValue();
            assert(isa<PointerType>(av->getType()));
            
            if (av == ptr) continue;
            
            if (av->getType()->getPointerAddressSpace() == ptr_space) {
              if (auto ai = dyn_cast<Instruction>(av)) {
                if (region.count(ai)) {
                  auto pa = dyn_cast<AllocaInst>(getProvenance(ai));
                  if (!pa || !allocas_to_localize.count(pa)) {
                    outs() << "unhoistable_cause =>" << *ai << "\n";
                    unhoistable.insert(i);
                    return false;
                  }
                }
              }
            }
          }
//          // (I think we know it's false right away because we only added
//          //  instructions in the region)
//          unhoistable.insert(i);
//          return false;
        }
        
      }
      
      // alias set doesn't contain
      // we can safely hoist this if each of its dependent instruction is hoistable
      // are any of its dependent values unhoistable?
      for_each_op(op, *i) {
        if (auto iop = dyn_cast<Instruction>(*op)) {
          if (region.count(iop)) {
            if (!hoistable(iop, region, aliases, unhoistable, tomove, allocas_to_localize)) {
              outs() << "unhoistable_cause =>" << *iop << "\n";
              unhoistable.insert(iop);
              return false;
            } else {
              tomove.insert(iop);
            }
          }
        }
      }
      
      tomove.insert(i);
      aliases.remove(i);
      return true;
    }
    
    void expandRegion(bool allow_localize = true) {
      AliasSetTracker aliases(alias);
      outs() << "--------------- <line " << entry->getDebugLoc().getLine() << ">\n";
      
      RegionSet region;
      int anchor_ct = 0;
      int best_score = INT_MAX;
      ExitMap emap, beforeLocalize;
      
      auto computeScore = [this](InstructionSet& tomove, RegionSet& region, int anchor_ct){
        ValueSet inputs, outputs;
        for (auto r : region) {
          if (!tomove.count(r)) computeInOut(r, region, inputs, outputs);
        }
        
        int sz = 0;
        for (auto& s : {inputs, outputs}) {
          for (auto v : s) {
            sz += layout.getTypeAllocSize(v->getType());
          }
        }
        return sz // size of continuation/return message
            // messages saved by subsuming multiple anchors
          - (2 * MESSAGE_COST * (anchor_ct-1))
            // messages needed to hop around to each destination
          + (MESSAGE_COST * (valid_ptrs.size()-1));
      };
      
      SmallSetVector<BasicBlock*,4> deferred;
      SmallSetVector<Instruction*,4> frontier;
      frontier.insert(entry);
      
      hoists.clear();
      
      InstructionSet unhoistable, tomove;
      
      SmallPtrSet<AllocaInst*,4> allocas_to_localize;
      
      while (!frontier.empty()) {
        auto i = frontier.pop_back_val();
        bool valid = validInRegion(i);
        
        if (!valid) {
          DEBUG(outs() << "invalid =>" << *i << "\n");
        }
        
        bool hoisting = false, localizing = false;
        
        if (!valid && i->mayReadOrWriteMemory()) {
          if ( hoistable(i, region, aliases, unhoistable, tomove, allocas_to_localize) ) {
            outs() << "hoist =>" << *i << "\n";
            hoists.insert(i);
            hoisting = true;
          } else if (allow_localize) {
            if (auto p = getProvenance(i) ) {
              if (auto a = dyn_cast<AllocaInst>(p)) {
                outs() << "localize =>" << *i << "\n";
                to_localize.insert(a);
                allocas_to_localize.insert(a);
                localizing = true;
              }
            }
          }
        }
        
        if (valid || localizing) aliases.add(i);
        
        if (valid || hoisting || localizing) {
          region.insert(i);
          
          if (!hoisting) {
            if (isAnchor(i)) {
              anchor_ct++;
            }
          }
          
          int score = computeScore(tomove, region, anchor_ct);
          
          if (!hoisting) outs() << format("(%d,%4d) ", anchor_ct, score) << *i;
          
          if (score < best_score) {
            outs() << "  !! best!";
            best_score = score;
            emap.fromRegion(region, score);
          }
            
          outs() << "\n";
          
          auto bb = i->getParent();
          if (isa<TerminatorInst>(i)) {
            for_each(sb, bb, succ) {
              deferred.insert(*sb);
            }
          } else {
            frontier.insert(i->getNextNode());
          }
          
        }
        
        if (frontier.empty()) for (auto bb : deferred) {
          bool valid = true;
          for_each(pb, bb, pred) valid &= (region.count((*pb)->getTerminator()) > 0);
          if (valid) {
            if (!region.count(bb->begin()))
              frontier.insert(bb->begin());
            deferred.remove(bb);
            break;
          }
        }
      } // while (!frontier.empty())
      
      if (!allocas_to_localize.empty()) {
        bool all_good = true;
        UniqueQueue<Value*> to_check;
        for (auto a : allocas_to_localize) to_check.push(a);
        
        while (!to_check.empty()) {
          auto a = to_check.pop();
          // if any uses aren't in the region, can't localize, so give up
          for_each_use(u, *a) {
            auto inst = cast<Instruction>(*u);
            if (!region.count(inst)) {
              if (inst->mayReadOrWriteMemory()) {
                outs() << "!! use of alloca outside region:" << *inst << "\n";
                all_good = false;
                break;
              } else {
                to_check.push(inst);
              }
            }
          }
          if (!all_good) break;
        }
        if (!all_good) {
          // try again, this time without attempting to localize
          outs() << "!! localizing failed :(\n";
          expandRegion(false);
          return;
        } else {
          // just ensured that allocas were okay to move into this region,
          // so have to stick with it now...
          outs() << "!! localizing!\n";
          int score = computeScore(tomove, region, anchor_ct);
          max_extent.fromRegion(region, score);
          switchExits(max_extent);
        }
      } else {
        to_localize.clear();
        int score = computeScore(tomove, region, anchor_ct);
        max_extent.fromRegion(region, score);
        
        if (max_extent.score <= emap.score) {
          outs() << "---- chose max_extent\n";
          switchExits(max_extent);
        } else {
          switchExits(emap);
        }
        
      }
    }
    
    bool validInRegion(Instruction* i) {
#define returnInvalid { outs() << "invalid." << __LINE__ << *i << "\n"; return false; }
      auto validPtr = [&](Value *p){
        return valid_ptrs.count(p) || isSymmetricPtr(p) || isStatic(p) || isConst(p);
      };
      
      if (i->mayReadOrWriteMemory() || isa<AddrSpaceCastInst>(i)) {
        if (auto p = getProvenance(i)) {
          if (validPtr(p)) {
            return true;
          }
        } else if (isa<CallInst>(i)) { // || isa<InvokeInst>(i)) {
          // do nothing for now
          auto cs = CallSite(i);
          if (auto fn = cs.getCalledFunction()) {
            if (fn->hasFnAttribute("anywhere") || fn->doesNotAccessMemory()) {
              return true;
            }
            
            // TODO: is it okay to allow these? should we mark things we want to allow?
            // if (cs->mayThrow()) return false;
            
            // call inherits 'provenance' from pointer args, then
            for (auto o = cs.arg_begin(); o != cs.arg_end(); o++)
              if (isa<PointerType>((*o)->getType())) {
                DEBUG(outs() << "!! " << *(*o)->getType() << "\n  " << **o << "\n");
                if (!validPtr(*o)) {
                  returnInvalid;
                }
              }
            
            return true;
          }
        } else {
          errs() << "!! no provenance:" << *i;
        }
        returnInvalid;
      } else if (isa<ReturnInst>(i) || isa<InvokeInst>(i)){
        returnInvalid;
      } else {
        return true;
      }
#undef returnInvalid
    }
    
    /////////////////////////
    // find inputs/outputs
    template< typename RegionSet >
    void computeInOut(Instruction* it, const RegionSet& rs, ValueSet& inputs, ValueSet& outputs) {
      auto definedInRegion = [&](Value* v) {
        if (auto i = dyn_cast<Instruction>(v))
          if (rs.count(i))
            return true;
        return false;
      };
      auto definedInCaller = [&](Value* v) {
        if (isConst(v) || isStatic(v)) return false;
        if (isStack(v)) return true;
        if (auto i = dyn_cast<Instruction>(v))
          if (rs.count(i) == 0)
            return true;
        return false;
      };
      
      for_each_op(o, *it)  if (definedInCaller(*o)) inputs.insert(*o);
      for_each_use(u, *it) if (!definedInRegion(*u)) { outputs.insert(it); break; }
    }
    
    void computeInputsOutputs() {
      auto& ctx = entry->getContext();
      ValueSet inputs, outputs;
      
      SmallSet<Instruction*,64> rs;
      for (auto p : owner) if (p.second == this) rs.insert(p.first);
      
      visit([&](BasicBlock::iterator it){
        computeInOut(it, rs, inputs, outputs);
      });
      
      /////////////////////////////////////////////
      // create struct types for inputs & outputs
      SmallVector<Type*,8> in_types, out_types;
      for (auto& p : inputs)  {  in_types.push_back(p->getType()); }
      for (auto& p : outputs) { out_types.push_back(p->getType()); }
      
      ty_input = StructType::get(ctx, in_types);
      ty_output = StructType::get(ctx, out_types);
    }
    
    void doHoist(Instruction *i, Instruction *before, RegionSet& region) {
      i->removeFromParent();
      i->insertBefore(before);
      for_each_op(o, *i) {
        if (auto oi = dyn_cast<Instruction>(*o)) {
          if (region.count(oi)) {
            doHoist(oi, i, region);
          }
        }
      }
    }
    
    Function* extractRegion(bool async, Value* gce = nullptr) {
      auto name = "d" + Twine(ID) + "_l" + Twine(entry->getDebugLoc().getLine());
      DEBUG(outs() << "//////////////////\n// extracting " << name << "\n");
      DEBUG(outs() << "target_ptr =>" << *target_ptr << "\n");
      
      auto mod = entry->getParent()->getParent()->getParent();
      auto& ctx = entry->getContext();
      auto ty_i16 = Type::getInt16Ty(ctx);
      auto ty_void_ptr = Type::getInt8PtrTy(ctx);
      auto ty_void_gptr = Type::getInt8PtrTy(ctx, GLOBAL_SPACE);
      auto i64 = [&](int64_t v) { return ConstantInt::get(Type::getInt64Ty(ctx), v); };
      
      if (async) outs() << "---- extracting async (" << name << ")\n";
      
      // get set of instructions in region (makes hoist easier)
      RegionSet region;
      visit([&](BasicBlock::iterator it){ region.insert(it); });
      
      // move hoists before entry
      for (auto i : region) {
        if (hoists.count(i)) {
          doHoist(i, entry, region);
        }
      }
      
      // localize
      UniqueQueue<Instruction*> todo;
      for (auto a : to_localize) todo.push(a);
      Instruction* insertpt = entry;
      while (!todo.empty()) {
        auto inst = todo.pop();
        if (!region.count(inst)) {
          for_each_use(u, *inst) todo.push(cast<Instruction>(*u));
          inst->removeFromParent();
          inst->insertAfter(insertpt);
          insertpt = inst;
        }
      }
      
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
      
      if (BasicBlock::iterator(entry) != bb_in->begin()
          || bb_in == &old_fn->getEntryBlock()) {
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
        assert(isa<Instruction>(before));
        assert(isa<Instruction>(after));
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
          assert(isa<BasicBlock>(bb_after));
          bool found = false;
          for_each(sb, bb_exit, succ) {
            if (*sb == bb_after) {
              found = true;
              break;
            }
          }
          if (!found) {
            outs() << "^^^^^^^^^^^^^^^^^^^^\n";
            outs() << *bb_exit;
            outs() << *bb_after->getType() << "\n";
//            outs() << *bb_after;
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
      
      if (async) assert(outputs.size() == 0);
      
      /////////////////////////////////////////////
      // create struct types for inputs & outputs
      SmallVector<Type*,8> in_types, out_types;
      for (auto& p : inputs)  {  in_types.push_back(p->getType()); }
      for (auto& p : outputs) { out_types.push_back(p->getType()); }
      
      auto in_struct_ty = StructType::get(ctx, in_types);
      auto out_struct_ty = StructType::get(ctx, out_types);
      
//      assert2(in_struct_ty == ty_input, "different input types", *ty_input, *in_struct_ty);
//      if (out_struct_ty != ty_output) for (auto v : outputs) outs() << "-- " << *v << "\n";
//      assert2(out_struct_ty == ty_output, "different output types", *ty_output, *out_struct_ty);
      
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
        clone_map[v] = CreateAlignedLoad(b, in_arg, i, "in." + v->getName());
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
        CreateAlignedStore(b, v, out_arg, i, "out."+v->getName());
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
        CreateAlignedStore(b, inputs[i], in_alloca, i, name+".gep.in");
      }

      auto target_core = b.CreateCall(ginfo.fn("get_core"), (Value*[]){
        b.CreateBitCast(target_ptr, ty_void_gptr)
      }, name+".target_core");
      
      CallInst *call;
      if (async) {
        size_t sz = layout.getTypeAllocSize(in_struct_ty);
        Function *on_async_fn = ginfo.fn("on_async");
        if      (sz <= 16)  { on_async_fn = ginfo.fn("on_async_16"); errs() << "---- on_async_16\n"; }
        else if (sz <= 32)  { on_async_fn = ginfo.fn("on_async_32"); errs() << "---- on_async_32\n"; }
        else if (sz <= 64)  { on_async_fn = ginfo.fn("on_async_64"); errs() << "---- on_async_64\n"; }
        else if (sz <= 128) { on_async_fn = ginfo.fn("on_async_128"); errs() << "---- on_async_128\n"; }
        else { errs() << "---- on_async (generic)\n"; }
        
        auto in_void = b.CreateBitCast(in_alloca, ty_void_ptr);
        call = b.CreateCall(on_async_fn, { target_core, new_fn, in_void, i64(sz), gce },
                            name+".call_on_async");
      } else {
        call = b.CreateCall(ginfo.fn("on"), {
          target_core, new_fn,
          b.CreateBitCast(in_alloca, ty_void_ptr),
          i64(layout.getTypeAllocSize(in_struct_ty)),
          b.CreateBitCast(out_alloca, ty_void_ptr),
          i64(layout.getTypeAllocSize(out_struct_ty))
        }, name+".call_on");
      }
      
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
        auto ld = CreateAlignedLoad(b, out_alloca, i, "out."+v->getName());
        v->replaceAllUsesWith(ld);
      }
      
      //////////////////////////////////////////////////////////////////////////
      // fixup global* uses in new_fn
      // (note: assuming any global* accesses are valid)
      // also strip provenance metadata so it doesn't confuse 'fixupFunction()'
      SmallDenseMap<Value*,Value*> lptrs;
      
      auto new_tgt = clone_map[target_ptr];
      
      for (auto bb = new_fn->begin(); bb != new_fn->end(); bb++) {
        for (auto inst = bb->begin(); inst != bb->end(); ) {
          Instruction *orig = inst++;
          auto prov = getProvenance(orig);
          if (prov == new_tgt) {
            setProvenance(orig, nullptr);
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
      
      // for (auto bb : bbs) {
      //   for (auto& i : *bb) {
      //     for_each_use(u, i) {
      //       if (auto ui = dyn_cast<Instruction>(*u)) {
      //         if (!bbs.count(ui->getParent())) {
      //           errs() << "ui =>" << *ui << "\n";
      //           errs() << *ui->getParent() << "\n";
      //         }
      //       }
      //     }
      //   }
      // }
      
      for (auto bb : bbs) {
        for (auto u = bb->use_begin(), ue = bb->use_end(); u != ue; u++) {
          if (auto uu = dyn_cast<Instruction>(*u)) {
            if (bbs.count(uu->getParent()) == 0) {
              auto uubb = uu->getParent();
              assertN(false, "!! use escaped", *uu, "in => "+uubb->getName(), bb->getName());
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
                assert(uubb->getParent() == old_fn);
                assertN(false, "!! use escaped", i, "---- use:", *uu, "----", *uubb);
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
      
      //////////////////////////
      // recurse for multi-hop
      if (valid_ptrs.size() > 1) {
        // find hop anchor
        Instruction* next_entry = nullptr;
        for_each(i, new_fn, inst) {
          Instruction* inst = &*i;
          if (isAnchor(inst,GLOBAL_SPACE)) {
            next_entry = inst;
            break;
          }
        }
        
        outs() << "next_hop.entry =>" << *next_entry << "\n";
        
        auto hop = make_unique<CandidateRegion>(getProvenance(next_entry), next_entry,
                                                owner, ginfo, layout, alias);
        hop->expandRegion();
        
        assert(hop->exits.isVoidRetExit());
        auto next_hop = hop->extractRegion(true, gce);
        outs() << "++++ created next hop\n";
      }
      
      return new_fn;
    }
    
    void printHeader() {
      outs() << "\n##############\n# Candidate " << ID << ":\n";
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
      
//      DEBUG({
        outs() << "  exits:\n";
        exits.each([&](Instruction* s, Instruction* e){
          outs() << "  " << *s << "\n     =>" << *e << "\n";
        });
//        outs() << "\n";
//      });
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
          os << "<font face='InconsolataLGC' point-size='11'";
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
        
        for (auto fn : (StringRef[]){
          "@grappa_get_core",
          "@grappa_get_pointer",
          "@grappa_get_pointer_symmetric",
          "@grappa_get",
          "@grappa_put",
          "@grappa_on",
          "@grappa_on_async"
        }) {
          auto fns = (fn+"(").str();
          auto fnc = ("<font color='green'>"+fn+"</font>").str();
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
      o.SetBuffered();
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
      o << "  fontname=\"InconsolataLGC\";\n";
      o << "  label=\"" << demangle(F.getName()) << "\"";
      o << "  node[shape=record];\n";
      
      for (auto& bb : F) {
        dotBB(o, candidates, &bb);
      }
      
      o << "}\n";
      
      o.close();
    }
    
  };
  
  
  int ExtractorPass::fixupFunction(Function* fn, std::set<int>* lines) {
    
    int fixed_up = preFixup(fn, layout);
    
    GlobalPtrInfo::LocalPtrMap lptrs;
    
    //////////////////////////////////////////////////////
    // find all global/symmetric accesses and fix them up
    for (auto& bb : *fn ) {
      for (auto inst = bb.begin(); inst != bb.end(); ) {
        Instruction *orig = inst++;
        Value* ptr = nullptr;
        if (auto l = dyn_cast<LoadInst>(orig))  ptr = l->getPointerOperand();
        if (auto l = dyn_cast<StoreInst>(orig)) ptr = l->getPointerOperand();
        if (auto l = dyn_cast<AddrSpaceCastInst>(orig)) ptr = l->getOperand(0);
        
        if (!ptr) continue;
        if (isGlobalPtr(ptr)) {
          if (auto c = dyn_cast<AddrSpaceCastInst>(orig)) {
//            assertN(c->getNumUses() == 0, "addrspacecast slipped in", *c);
//            c->eraseFromParent();
            
          } else if (auto gptr = ginfo.ptr_operand<GLOBAL_SPACE>(orig)) {
            if (lines) lines->insert(orig->getDebugLoc().getLine());
            ginfo.replace_global_access(gptr, nullptr, orig, lptrs, *layout);
            fixed_up++;
          }
        } else if (isSymmetricPtr(ptr)) {
          if (auto sptr = ginfo.ptr_operand<SYMMETRIC_SPACE>(orig)) {
            if (lines) lines->insert(orig->getDebugLoc().getLine());
            ginfo.replace_with_local<SYMMETRIC_SPACE>(sptr, orig, lptrs);
            fixed_up++;
          } else {
            assertN(false, "huh?", *orig, *ptr);
          }
        }
        
        auto ctx = &orig->getContext();
        
        //////////////////////////////////////////////////////
        // put/get any local ptrs with global provenance
        // (use provenance ptr's 'core', but pass raw pointer
        // -- can't compute global pointer over here)
        auto prov = getProvenance(orig);
        if (isGlobalPtr(prov) && !isGlobalPtr(ptr)) {
          assertN(orig->mayReadOrWriteMemory(), "attempting to globalize a non-ptr", *orig);
          IRBuilder<> b(orig);
          auto v_prov = b.CreateBitCast(prov, void_gptr_ty);
          auto core = b.CreateCall(ginfo.fn("get_core"), { v_prov }, "core");
          if (lines) lines->insert(orig->getDebugLoc().getLine());
          ginfo.replace_global_access(ptr, core, orig, lptrs, *layout);
        }
        
      }
    }
    
    return fixed_up;
  }
  
  long CandidateRegion::id_counter = 0;
  
  
  bool ExtractorPass::runOnModule(Module& M) {
    
    layout = new DataLayout(&M);
    
//    if (! ginfo.init(M) ) return false;
    bool found_functions = ginfo.init(M);
    if (!found_functions)
      outs() << "Didn't find Grappa primitives, disabling extraction.\n";
    
    //////////////////////////
    // Find 'task' functions
    for (auto& F : M) {
      if (F.hasFnAttribute("async")) {
        async_fns.insert({&F, nullptr});
      }
    }
    
    /////////////////////////////////////////////////
    // find async's with GlobalCompletionEvent info
    auto async_md = M.getNamedMetadata("grappa.asyncs");
    if (async_md) {
      for (int i=0; i<async_md->getNumOperands(); i++) {
        auto md = cast<MDNode>(async_md->getOperand(i));
        auto fn = cast<Function>(md->getOperand(0));
        auto gce = md->getOperand(1);
        async_fns[fn] = gce;
      }
    }
    
    CandidateMap cmap;
    int ct = 0;
    
    UniqueQueue<Function*> worklist;
    for (auto p : async_fns) {
      auto fn = p.first;
      worklist.push(fn);
    }
    
    struct DbgRemover : public InstVisitor<DbgRemover> {
      void visitIntrinsicInst(IntrinsicInst& i) {
        for (auto prefix : {"llvm.dbg.", "llvm.lifetime."}) {
          if (i.getCalledFunction()->getName().startswith(prefix)) {
            i.eraseFromParent();
          }
        }
      }
    } dbg_remover;
    
    AliasAnalysis aliases = getAnalysis<AliasAnalysis>();
    
    while (!worklist.empty()) {

      auto fn = worklist.pop();
      
      auto taskname = "task" + Twine(ct);
      
      bool changed = false;
      
      // Recursively process called functions
      for_each(it, *fn, inst) {
        auto inst = &*it;
        if (isa<IntrinsicInst>(inst)) continue;
        else if (isa<CallInst>(inst) || isa<InvokeInst>(inst)) {
          CallSite cs(inst);
          if (cs.getCalledFunction()) worklist.push(cs.getCalledFunction());
        }
      }
      
      preFixup(fn, layout);
      
      AnchorSet anchors;
      analyzeProvenance(*fn, anchors);
      
      dbg_remover.visit(fn);
      
      std::set<int> lines;
      
      if ( DoExtractor ) {
        // Get rid of debug info that causes problems with extractor
        
        std::map<Instruction*,unique_ptr<CandidateRegion>> cnds;
        
        /////////////////////
        /// Compute regions
        for (auto a : anchors) {
          
          if (!a->mayReadOrWriteMemory() || !isGlobalPtr(getProvenance(a))) continue;
          
          int line = a->getDebugLoc().getLine();
          lines.insert(line);
          
          outs() << "anchor:" << line << " =>" << *a << "\n";
          
          auto p = getProvenance(a);
          if (cmap[a]) {
            DEBUG({
              outs() << "anchor already in another delegate:\n";
              outs() << "  anchor =>" << *a << "\n";
              outs() << "  other  =>" << *cmap[a]->entry << "\n";
            });
          } else if (isGlobalPtr(p)) {
            
            cnds[a] = make_unique<CandidateRegion>(p, a, cmap, ginfo, *layout, aliases);
            auto& r = *cnds[a];
            
            r.expandRegion();
            
            ////////////////////////////
            // find async opportunities
//            if (!DisableAsync && r.max_extent.isVoidRetExit()) {
//              assert(layout->getTypeAllocSize(r.ty_output) == 0);
//                // if (async_fns[fn]) {
//                r.switchExits(r.max_extent);
//                outs() << "!! grappa_on_async candidate\n";
//                // }
//            }
            if (r.exits.isVoidRetExit()) outs() << "!! grappa_on_async candidate\n";

            r.printHeader();
            
            r.visit([&](BasicBlock::iterator i){
              if (cmap[i] != &r) {
                errs() << "!! bad visit: " << *i << "\n";
                assert(false);
              }
            });
            
          }
        }
        
        //////////////////////////////////////////
        // find & evaluate multihop opportunities
        SmallVector<std::pair<Instruction*,Instruction*>,8> pairs;
        
        for (auto& p : cnds) {
          auto& r = *p.second;
          r.max_extent.each([&](Instruction* before, Instruction* after){
            outs() << "---- after =>" << *after << "\n";
            if (cnds.count(after)) {
              auto& o = *cnds[after];
              outs() << "++++ chaining opportunity!\n" << *r.entry << "\n" << *o.entry << "\n";
              pairs.push_back({r.entry, o.entry});
            }
          });
        }
        
        for (auto p : pairs) {
          auto& r = *cnds[p.first];
          auto& o = *cnds[p.second];
          
          CandidateMap comb_map;
          auto comb = make_unique<CandidateRegion>(r.target_ptr, r.entry, cmap,
                                                   ginfo, *layout, aliases);
          comb->addPtr(getProvenance(o.entry));
          
          comb->expandRegion();
          
//          if (PrintDot) CandidateRegion::dumpToDot(*fn, comb_map,
//                                                   taskname+".comb"+Twine(comb->ID));
          
          outs() << "first.score    => " << r.cost() << "\n";
          outs() << "second.score   => " << o.cost() << "\n";
          outs() << "comb.score     => " << comb->cost() << "\n";
          
          if (comb->cost() < (r.cost() + o.cost())) {
            outs() << "++++ multihop wins!\n";
            cnds.erase(r.entry);
            cnds.erase(o.entry);
            cnds[comb->entry] = std::move(comb);
          }
        }
        
        if (cnds.size() > 0) {
          changed = true;
          if (PrintDot && (OnlyLine == 0 || lines.count(OnlyLine)))
            CandidateRegion::dumpToDot(*fn, cmap, taskname);
        }
        
        if (found_functions && cnds.size() > 0) {
          for (auto& p : cnds) {
            auto& r = *p.second;
            
            bool async = !DisableAsync && r.exits.isVoidRetExit();
            
            auto new_fn = r.extractRegion(async, async_fns[fn]);
            
//            if (PrintDot && fn->getName().startswith("async"))
            if (PrintDot && (OnlyLine == 0 || lines.count(OnlyLine)))
              CandidateRegion::dumpToDot(*new_fn, cmap, taskname+".d"+Twine(r.ID));
          }
        }
      } // if ( DoExtractor )
      
      if (found_functions) {
        // insert put/get & get local ptrs for symmetric addrs
        int nfixed = fixupFunction(fn, &lines);
        
        if (nfixed || changed) {
          changed = true;
          if (PrintDot && (OnlyLine == 0 || lines.count(OnlyLine)))
            CandidateRegion::dumpToDot(*fn, cmap, taskname+".after");
        }
      }
      
      if (changed) ct++;
    }
    
    
    outs() << "%%%%%%%%%%%%%%%%%%%%%%%%%%\n";
    
    ////////////////////////////////////////////////
    // fixup any remaining global/symmetric things
    if (found_functions) for (auto& F : M) {
      auto ct = fixupFunction(&F);
      if (ct) outs() << "^^^^^^^^^^^\n" << F.getName() << "\n-----------\n";
    }
    
    
    assert(!verifyModule(M, &outs()));
    
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
