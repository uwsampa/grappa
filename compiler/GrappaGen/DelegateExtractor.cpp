#undef DEBUG
#include "DelegateExtractor.hpp"

using namespace llvm;

/// definedInRegion - Return true if the specified value is defined in the
/// extracted region.
static bool definedInRegion(const SetVector<BasicBlock *> &Blocks, Value *V) {
  if (Instruction *I = dyn_cast<Instruction>(V))
    if (Blocks.count(I->getParent()))
      return true;
  return false;
}

/// definedInCaller - Return true if the specified value is defined in the
/// function being code extracted, but not in the region being extracted.
/// These values must be passed in as live-ins to the function.
static bool definedInCaller(const SetVector<BasicBlock *> &Blocks, Value *V) {
  if (isa<Argument>(V)) return true;
  if (Instruction *I = dyn_cast<Instruction>(V))
    if (!Blocks.count(I->getParent()))
      return true;
  return false;
}

template< typename InsertType >
GetElementPtrInst* struct_elt_ptr(Value *struct_ptr, int idx, const Twine& name,
                                  InsertType* insert) {
  auto i32_ty = Type::getInt32Ty(insert->getContext());
  return GetElementPtrInst::Create(struct_ptr, (Value*[]){
    Constant::getNullValue(i32_ty),
    ConstantInt::get(i32_ty, idx)
  }, name, insert);
};

#define void_ty      Type::getVoidTy(mod.getContext())
#define i64_ty       Type::getInt64Ty(mod.getContext())
#define void_ptr_ty  Type::getInt8PtrTy(mod.getContext(), 0)
#define void_gptr_ty Type::getInt8PtrTy(mod.getContext(), GlobalPtrInfo::SPACE)

DelegateExtractor::DelegateExtractor(Module& mod, GlobalPtrInfo& ginfo):
  mod(mod), ginfo(ginfo)
{
  layout = new DataLayout(&mod);
}

void DelegateExtractor::findInputsOutputsUses(ValueSet& inputs, ValueSet& outputs,
                                              GlobalPtrMap& gptrs) const {
  // map loaded value back to its pointer
  std::map<Value*,Value*> gvals;
  
  for (auto bb : bbs) {
    for (auto& ii : *bb) {
      
      if (auto gld = dyn_cast_global<LoadInst>(&ii)) {
        auto p = gld->getPointerOperand();
        if (gptrs.count(p) == 0) gptrs[p] = {};
        gptrs[p].loads++;
        gvals[gld] = p;
      } else if (auto gi = dyn_cast_global<StoreInst>(&ii)) {
        auto p = gi->getPointerOperand();
        if (gptrs.count(p) == 0) gptrs[p] = {};
        gptrs[p].stores++;
      }
      
      for (auto op = ii.op_begin(), opend = ii.op_end(); op != opend; op++) {
        if (definedInCaller(bbs, *op)) inputs.insert(*op);
        if (gvals.count(*op) > 0) {
          gptrs[gvals[*op]].uses++;
        }
      }
      for (auto use = ii.use_begin(), useend = ii.use_end(); use != useend; use++) {
        if (!definedInRegion(bbs, *use)) {
          outputs.insert(&ii);
          break;
        }
      }
    }
  }
}


Function* DelegateExtractor::extractFunction() {
  assert( gptrs.size() == 1 );
  auto gptr = gptrs[0];
  
  auto bbin = bbs[0];
  
  // because of how we construct regions, should not begin with Phi
  // (should start with global load/store...)
  assert( ! isa<PHINode>(*bbin->begin()) );
  
  Instruction* first = bbin->begin();
  auto& dl = first->getDebugLoc();
  outs() << "\n-------------------\nextracted delegate: "
         << "(line " << dl.getLine() << ")\n"
         << "block:" << *bbin;
  
  ValueSet inputs, outputs;
  GlobalPtrMap gptrs;
  findInputsOutputsUses(inputs, outputs, gptrs);
  
  outs() << "inputs:\n";
  for (auto in : inputs) {
    outs() << "  " << in->getName() << " (" << layout->getTypeAllocSize(in->getType()) << ")\n";
  }
  outs() << "outputs:\n";
  for (auto out : outputs) {
    outs() << "  " << out->getName() << " (" << layout->getTypeAllocSize(out->getType()) << ")\n";
  }
  
  DEBUG( outs() << "\nins:"; for (auto v : inputs) errs() << "\n  " << *v; errs() << "\n"
    << "outs:"; for (auto v : outputs) errs() << "\n  " << *v; errs() << "\n"
    << "gptrs:\n"; for (auto v : gptrs) {
      auto u = v.second;
      errs() << "  " << v.first->getName()
    << " { loads:" << u.loads << ", stores:" << u.stores << ", uses:" << u.uses << " }\n";
  } errs() << "\n" );
  
  // create struct types for inputs & outputs
  SmallVector<Type*,8> in_types, out_types;
  for (auto& p : inputs)  {  in_types.push_back(p->getType()); }
  for (auto& p : outputs) { out_types.push_back(p->getType()); }
  
  auto in_struct_ty = StructType::get(mod.getContext(), in_types);
  auto out_struct_ty = StructType::get(mod.getContext(), out_types);
  
  // create function shell
  auto new_fn_ty = FunctionType::get(void_ty, (Type*[]){ void_ptr_ty, void_ptr_ty }, false);
  auto new_fn = Function::Create(new_fn_ty, GlobalValue::InternalLinkage, "delegate." + bbin->getName(), &mod);
  
  auto& ctx = mod.getContext();
  
  auto entrybb = BasicBlock::Create(ctx, "d.entry", new_fn);
  
  auto i64_num = [=](long v) { return ConstantInt::get(i64_ty, v); };
  
  ValueToValueMapTy clone_map;
  //      std::map<Value*,Value*> arg_map;
  
  Function::arg_iterator argi = new_fn->arg_begin();
  auto in_arg_ = argi++;
  auto out_arg_ = argi++;
  
  DEBUG( errs() << "in_arg_:" << *in_arg_ << "\n" );
  DEBUG( errs() << "out_arg_:" << *out_arg_ << "\n" );
  
  auto in_arg = new BitCastInst(in_arg_, in_struct_ty->getPointerTo(), "bc.in", entrybb);
  auto out_arg = new BitCastInst(out_arg_, out_struct_ty->getPointerTo(), "bc.out", entrybb);
  
  // copy delegate block into new fn & remap values to use args
  for (auto bb : bbs) {
    clone_map[bb] = CloneBasicBlock(bb, clone_map, ".clone", new_fn);
  }
  
  auto newbb = dyn_cast<BasicBlock>(clone_map[bbin]);
  
  // jump from entrybb to block cloned from inblock
  BranchInst::Create(newbb, entrybb);
  
  
  std::map<Value*,Value*> remaps;
  
  for (int i = 0; i < outputs.size(); i++) {
    auto v = outputs[i];
    auto ge = struct_elt_ptr(out_arg, i, "out.clr", entrybb);
    new StoreInst(Constant::getNullValue(v->getType()), ge, false, entrybb);
  }
  
  for (int i = 0; i < inputs.size(); i++) {
    auto v = inputs[i];
    auto gep = struct_elt_ptr(in_arg, i, "d.ge.in." + v->getName(), entrybb);
    auto in_val = new LoadInst(gep, "d.in." + v->getName(), entrybb);
    
    DEBUG( errs() << "in_val: " << *in_val << " (parent:" << in_val->getParent()->getName() << ", fn:" << in_val->getParent()->getParent()->getName() << ")\n" );
    
    Value *final_in = in_val;
    
    // if it's a global pointer, get local address for it
    if (auto in_gptr_ty = dyn_cast_global(in_val->getType())) {
      auto ptr_ty = in_gptr_ty->getElementType()->getPointerTo();
      auto bc = new BitCastInst(in_val, void_gptr_ty, "getptr.bc", entrybb);
      auto vptr = CallInst::Create(ginfo.get_pointer_fn, (Value*[]){ bc }, "getptr", entrybb);
      auto ptr = new BitCastInst(vptr, ptr_ty, "localptr", entrybb);
      final_in = ptr;
    }
    
    clone_map[v] = final_in;
  }
  
  auto old_fn = bbin->getParent();
  
  auto retbb = BasicBlock::Create(ctx, "ret." + bbin->getName(), new_fn);
  
  auto retTy = Type::getInt16Ty(ctx);
  
  // create PHI for selecting which return value to use
  // make sure it's the first thing in the BB
  auto retphi = PHINode::Create(retTy, exits.size(), "d.retphi", retbb);
  
  // return from end of created block
  auto newret = ReturnInst::Create(ctx, retphi, retbb);
  
  // store outputs before return
  for (int i = 0; i < outputs.size(); i++) {
    assert(clone_map.count(outputs[i]) > 0);
    auto v = clone_map[outputs[i]];
    auto ep = struct_elt_ptr(out_arg, i, "d.out.gep." + v->getName(), newret);
    new StoreInst(v, ep, true, newret);
  }
  
  //////////////
  // emit call
  
//  assert(bbin->getTerminator()->getNumSuccessors() == 1);
  
  auto prevbb = bbin->getUniquePredecessor();
  assert(prevbb != nullptr);
  
  // create new bb to replace old block and call
  auto callbb = BasicBlock::Create(bbin->getContext(), "d.call.blk", old_fn, bbin);
  
  prevbb->getTerminator()->replaceUsesOfWith(bbin, callbb);
  
  // hook inblock into new fn
  //      inblock->moveBefore(retbb);
//  auto nextbb = bbin->getTerminator()->getSuccessor(0);
//  newbb->getTerminator()->replaceUsesOfWith(nextbb, retbb);
  
  prevbb->getTerminator()->replaceUsesOfWith(bbin, callbb);
  
  // see switch
//  BranchInst::Create(nextbb, callbb);

  auto call_pt = callbb->getTerminator();
  
  // allocate space for in/out structs near top of function
  auto in_struct_alloca = new AllocaInst(in_struct_ty, 0, "d.in_struct", old_fn->begin()->begin());
  auto out_struct_alloca = new AllocaInst(out_struct_ty, 0, "d.out_struct", old_fn->begin()->begin());
  
  // copy inputs into struct
  for (int i = 0; i < inputs.size(); i++) {
    auto v = inputs[i];
    auto gep = struct_elt_ptr(in_struct_alloca, i, "dc.in", call_pt);
    new StoreInst(v, gep, false, call_pt);
  }
  
  //////////////////////////////
  // for debug:
  // init out struct to 0
  for (int i = 0; i < outputs.size(); i++) {
    auto v = outputs[i];
    auto gep = struct_elt_ptr(out_struct_alloca, i, "dc.out.dbg", call_pt);
    new StoreInst(Constant::getNullValue(v->getType()), gep, false, call_pt);
  }
  //////////////////////////////
  
  auto target_core = CallInst::Create(ginfo.get_core_fn, (Value*[]){
    new BitCastInst(gptr, void_gptr_ty, "", call_pt)
  }, "", call_pt);
  
  auto calli = CallInst::Create(ginfo.call_on_fn, (Value*[]){
    target_core,
    new_fn,
    new BitCastInst(in_struct_alloca, void_ptr_ty, "", call_pt),
    i64_num(layout->getTypeAllocSize(in_struct_ty)),
    new BitCastInst(out_struct_alloca, void_ptr_ty, "", call_pt),
    i64_num(layout->getTypeAllocSize(out_struct_ty))
  }, "", call_pt);
  
  ////////////////////////////
  // switch among exit blocks
  
  auto switchi = SwitchInst::Create(calli, nullptr, exits.size(), call_pt);
  
  unsigned exit_id = 0;
  for (auto& e : exits) {
    auto retcode = ConstantInt::get(retTy, exit_id);
    
    // bb in delegate that we're coming from
    assert(clone_map.count(e.second) > 0);
    auto predbb = dyn_cast<BasicBlock>(clone_map[e.second]);
    assert(predbb && predbb->getParent() == new_fn);
    
    // hook up exit from region with phi node in return block
    retphi->setIncomingBlock(exit_id, predbb);
    retphi->setIncomingValue(exit_id, retcode);
    
    // jump to old exit block when call returns the code for it
    auto targetbb = e.first;
    switchi->addCase(retcode, targetbb);
    
    // in extracted delegate, remap branches outside to retbb
    clone_map[targetbb] = retbb;
    
    exit_id++;
  }
  
  // use clone_map to remap values in new function
  // (including branching to new retbb instead of exit blocks)
  for (auto& bb : *new_fn) {
    for (auto& inst : bb) {
      for (int i = 0; i < inst.getNumOperands(); i++) {
        auto o = inst.getOperand(i);
        if (clone_map.count(o) > 0) {
          inst.setOperand(i, clone_map[o]);
        }
      }
    }
  }
  
  // load outputs
  for (int i = 0; i < outputs.size(); i++) {
    auto v = outputs[i];
    auto gep = struct_elt_ptr(out_struct_alloca, i, "dc.out." + v->getName(), call_pt);
    auto ld = new LoadInst(gep, "d.call.out." + v->getName(), call_pt);
    
    DEBUG( errs() << "replacing output:\n" << *v << "\nwith:\n" << *ld << "\nin:\n" );
    std::vector<User*> Users(v->use_begin(), v->use_end());
    for (unsigned u = 0, e = Users.size(); u != e; ++u) {
      Instruction *inst = cast<Instruction>(Users[u]);
      DEBUG( errs() << *inst << "  (parent: " << inst->getParent()->getParent()->getName() << ") " );
      if (inst->getParent()->getParent() != new_fn) {
        inst->replaceUsesOfWith(v, ld);
        DEBUG( errs() << " !!" );
      } else {
        DEBUG( errs() << " ??" );
      }
      DEBUG( errs() << "\n" );
    }
  }
  
  DEBUG(
    errs() << "----------------\nconstructed delegate fn:\n" << *new_fn;
    errs() << "----------------\ncall site:\n" << *callbb;
    errs() << "@bh inblock preds:\n";
    for (auto p = pred_begin(bbin), pe = pred_end(bbin); p != pe; ++p) {
      errs() << (*p)->getName() << "\n";
    }
  );
  
  outs() << "-------------------\n";
  return new_fn;
}

bool DelegateExtractor::valid_in_delegate(Instruction* inst, Value* gptr, ValueSet& available_vals) {
  if (auto gld = dyn_cast_global<LoadInst>(inst)) {
    if (gld->getPointerOperand() == gptr) {
      available_vals.insert(gld);
      DEBUG( outs() << "load to same gptr: ok\n" );
      return true;
    }
  } else if (auto g = dyn_cast_global<StoreInst>(inst)) {
    if (g->getPointerOperand() == gptr) {
      DEBUG( outs() << "store to same gptr: great!\n" );
      return true;
    } else {
      DEBUG( outs() << "store to different gptr: not supported yet.\n" );
      return false;
    }
  } else if (isa<LoadInst>(inst) || isa<StoreInst>(inst)) {
    DEBUG( outs() << "load/store to normal memory: " << *inst << "\n" );
    return false;
  } else if (auto ge = dyn_cast<GetElementPtrInst>(inst)) {
    // TODO: fix this, some GEP's should be alright...
    return false;
  } else if ( isa<TerminatorInst>(inst)|| isa<PHINode>(inst) ) {
    return true;
  } else if ( isa<InvokeInst>(inst)) {
    return false;
  } else if (auto call = dyn_cast<CallInst>(inst)) {
    auto fn = call->getCalledFunction();
    if (fn->getName() == "llvm.dbg.value") return true;
    // TODO: detect if function is pure / inline it and see??
    return false;
  } else {
    return true;
    //          bool is_bin_op = isa<BinaryOperator>(inst);
    //
    //          if (is_bin_op) outs() << "checking ops: " << *inst << "\n";
    //
    //          bool valid = std::all_of(inst->op_begin(), inst->op_end(), [&](Value* op){
    //            if (is_bin_op) {
    //              outs() << *op << "\n  avail:" << available_vals.count(op) << ", const:" << dyn_cast<Constant>(op) << "\n";
    //            }
    //            if (available_vals.count(op) > 0 || isa<Constant>(op)) {
    //              return true;
    //            } else {
    //              return false;
    //            }
    //          });
    //          available_vals.insert(inst);
    //          return valid;
  }
  return false;
};


BasicBlock* DelegateExtractor::findStart(BasicBlock *bb) {
  Value *gptr = nullptr;
  for (auto i = bb->begin(); i != bb->end(); i++) {
    if (auto gld = dyn_cast_global<LoadInst>(i)) {
      gptr = gld->getPointerOperand();
    } else if (auto g = dyn_cast_global<StoreInst>(i)) {
      gptr = g->getPointerOperand();
    }
    if (gptr) {
      gptrs.insert(gptr);
      
      if (i != bb->begin()) {
        bb = bb->splitBasicBlock(i);
        //            bbs.insert(bb);
      }
      
      return bb;
    }
  }
  return nullptr;
}


bool DelegateExtractor::expand(BasicBlock *bb) {
  assert(gptrs.size() == 1);
  Value* gptr = *gptrs.begin();
  
  ValueSet vals;
  
  auto i = bb->begin();
  
  while ( i != bb->end() && DelegateExtractor::valid_in_delegate(i,gptr,vals) ) i++;
  
  if (i == bb->begin()) {
    return false;
  } else {
    bbs.insert(bb);
  }
  
  if (i != bb->end()) {
    // stopped in middle of bb
    auto newbb = bb->splitBasicBlock(i);
    exits.insert({newbb, bb});
    
  } else {
    // recurse on successors
    for (auto s = succ_begin(bb), se = succ_end(bb); s != se; s++) {
      DEBUG( errs() << ">>>>>>" << **s );
      if ( !expand(*s) && !bbs.count(*s) ) {
        exits.insert({*s, bb});
      } // else: exits added by recursive `expand` call
    }
  }
  
  return true;
}

void DelegateExtractor::print(raw_ostream& o) const {
  o << "///////////////////\n";
  auto loc = (*bbs.begin())->begin()->getDebugLoc();
  o << "// Region @ " << loc.getLine() << "\n";
  o << "gptrs:\n";
  for (auto p : gptrs) o << *p << "\n";
  o << "~~~~~~~~~~~~~~~~~~~";
  for (auto bb : bbs) o << *bb;
  o << "^^^^^^^^^^^^^^^^^^^\n";
}

