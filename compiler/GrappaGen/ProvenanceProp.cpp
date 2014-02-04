#include "Passes.h"
#include <llvm/Analysis/CFGPrinter.h>
#include <llvm/Support/GraphWriter.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/CallSite.h>

using namespace Grappa;

//////////////////////////////
// Register optional pass
static RegisterPass<ProvenanceProp> X( "provenance-prop", "Provenance Prop", false, false );
char ProvenanceProp::ID = 0;

MDNode* ProvenanceProp::INDETERMINATE = nullptr;

static cl::opt<bool> DisableANSI("no-color",
                              cl::desc("Disable ANSI colors."));

bool ProvenanceProp::doInitialization(Module& M) {
  outs() << "initialized ProvenanceProp\n";
  auto& C = M.getContext();
  INDETERMINATE = MDNode::get(C, MDString::get(C, "indeterminate"));
  return false;
}

MDNode* ProvenanceProp::provenance(Value* v) {
  switch (classify(v)) {
    case Indeterminate:
      return INDETERMINATE;
    case Unknown: {
      auto inst = cast<Instruction>(v);
      return inst->getMetadata("provenance");
    }
    default: {
      return MDNode::get(*ctx, v);
    }
  }
}

MDNode* getProvenance(Instruction* inst) {
  return inst->getMetadata("provenance");
}

void setProvenance(Instruction* inst, MDNode* m) {
  inst->setMetadata("provenance", m);
}

MDNode* ProvenanceProp::search(Value *v) {
  if (!v) return nullptr;
  
  // check if it's a potentially-valid pointer in its own right
  switch (classify(v)) {
    case Unknown:
      break;
    default:
      if (!prov.count(v))
        prov.insert({v, classify(v)});
  }
  
  // for instructions, inspect operands and recursively build up provenance
  if (auto i = dyn_cast<Instruction>(v)) {
    if (!getProvenance(i)) {
      
      SmallVector<Value*,8> vs;
      
      for (auto o = i->op_begin(); o != i->op_end(); o++) {
        auto m = search(*o);
        for (auto j = 0; j < m->getNumOperands(); j++) {
          auto mo = m->getOperand(j);
          switch (classify(mo)) {
            case Indeterminate:
              errs() << "indeterminate =>" << *mo << "\n";
            case Global:
            case Symmetric:
            case Stack:
              vs.push_back(mo);
              break;
            default:
              break;
          }
        }
      }
      
      if (!vs.empty()) {
        auto m = MDNode::get(*ctx, vs);
        setProvenance(i, m);
      }
    }
  }
  // else: we can compute provenance node directly (Const|Static)
  
  return provenance(v);
}

bool ProvenanceProp::runOnFunction(Function& F) {
  ctx = &F.getContext();
  
  for (auto& bb : F) {
    for (auto& inst : bb) {
      
      search(&inst);
      
      if (auto gep = dyn_cast<GetElementPtrInst>(&inst)) {
        if (gep->hasIndices()) {
          auto firstIdx = gep->getOperand(1);
          if (firstIdx == Constant::getNullValue(firstIdx->getType())) {
            // first index is 0, so must be a field offset, which is supposed to be local
            setProvenance(&inst, search(gep->getPointerOperand()));
          }
        }
      }
      
    }
  }
  
  ctx = nullptr;
  return false;
}

ProvenanceClass ProvenanceProp::classify(Value* v) {
  
//  if (v == ProvenanceProp::UNKNOWN)
//    return ProvenanceClass::Unknown;
//  
//  if (v == ProvenanceProp::INDETERMINATE)
//    return ProvenanceClass::Indeterminate;
  
  if (isa<GlobalVariable>(v) || isa<MDNode>(v) || isa<BasicBlock>(v))
    return ProvenanceClass::Static;
  
  // TODO: make a better decision about what to do with no-arg calls
  
  if (isa<CallInst>(v) || isa<InvokeInst>(v)) {
    auto cs = CallSite(v);
    
    if (auto fn = cs.getCalledFunction()) {
      if (fn->hasFnAttribute("unbound") || !cs->mayHaveSideEffects()) {
        if (fn->getArgumentList().size() == 0) {
          return Static;
        } else {
          return Unknown; // will inherit provenance of args
        }
      }
    }
    // else: indirect fn call, not handling these for now
    return Indeterminate;
  }
  
  if (auto c = dyn_cast<CallInst>(v)) {
    if (c->getNumArgOperands() == 0) {
      return ProvenanceClass::Static;
    } else if (c->mayHaveSideEffects()) {
      return ProvenanceClass::Indeterminate;
    }
  }
  
  if (isa<AllocaInst>(v) || isa<Argument>(v))
    return ProvenanceClass::Stack;
  
  if (isa<Constant>(v))
    return ProvenanceClass::Const;
  
  if (dyn_cast_addr<SYMMETRIC_SPACE>(v->getType()))
    return ProvenanceClass::Symmetric;

//  // count just 'load global**' as provenance base
//  // (seems too conservative, leaves out GEP-generated ones)
//  if (auto l = dyn_cast<LoadInst>(v)) {
//    if (auto ptrTy = dyn_cast<PointerType>(l->getType())) {
//      if (ptrTy->getPointerAddressSpace() == GLOBAL_SPACE) {
//        return ProvenanceClass::Global;
//      }
//    }
//  }
  
  // count any global* as a provenance base (requires merging global* sets)
  if (dyn_cast_addr<GLOBAL_SPACE>(v->getType()))
    return ProvenanceClass::Global;
  
  return ProvenanceClass::Unknown;
}

//Value* ProvenanceProp::meet(Value* a, Value* b) {
//  ProvenanceClass ac = prov[a]),
//                  bc = prov[b]);
//  
//  if (ac == Indeterminate || bc == Indeterminate) {
//    return INDETERMINATE;
//  } else if (ac == Global || bc == Global) {
//    if (ac == Global && bc == Global)
//      return (a == b) ? a : INDETERMINATE;
//    else if (ac == Global)
//      return a;
//    else
//      return b;
//  } else if (ac == Stack || bc == Stack) {
//    if (ac == Stack)
//      return a;
//    else
//      return b;
//  } else if (ac == Symmetric || bc == Symmetric) {
//    if (ac == Symmetric)
//      return a;
//    else
//      return b;
//  } else if (ac == Static || bc == Static) {
//    if (ac == Static)
//      return a;
//    else
//      return b;
//  } else if (ac == Const && bc == Const) {
//    return a;
//  } else {
//    return UNKNOWN;
//  }
//}

void ProvenanceProp::prettyPrint(Function& fn) {
  if (!DisableANSI) outs().changeColor(raw_ostream::YELLOW);
  outs() << "-------------------\n";
  if (!DisableANSI) outs().changeColor(raw_ostream::BLUE);
  outs() << *fn.getFunctionType();
  if (!DisableANSI) outs().resetColor();
  outs() << " {\n";
  
  for (auto& bb : fn) {
    
    outs() << bb.getName() << ":\n";
    
    for (auto& inst : bb) {
      // skip printing intrinsics (llvm.dbg, etc)
      if (isa<IntrinsicInst>(&inst)) continue;
      
      switch (prov[&inst]) {
        case ProvenanceClass::Unknown:
          outs() << "  ";
          if (!DisableANSI) outs().changeColor(raw_ostream::BLACK);
          break;
        case ProvenanceClass::Indeterminate:
          outs() << "!!";
          if (!DisableANSI) outs().changeColor(raw_ostream::RED);
          break;
        case ProvenanceClass::Static:
          outs() << "++";
          if (!DisableANSI) outs().changeColor(raw_ostream::GREEN);
          break;
        case ProvenanceClass::Symmetric:
          outs() << "<>";
          if (!DisableANSI) outs().changeColor(raw_ostream::CYAN);
          break;
        case ProvenanceClass::Global:
          outs() << "**";
          if (!DisableANSI) outs().changeColor(raw_ostream::BLUE);
          break;
        case ProvenanceClass::Const:
          outs() << "--";
          if (!DisableANSI) outs().changeColor(raw_ostream::YELLOW);
          break;
        case ProvenanceClass::Stack:
          outs() << "%%";
          if (!DisableANSI) outs().changeColor(raw_ostream::MAGENTA);
          break;
      }
      
      outs() << "  " << inst << "\n";
      if (!DisableANSI) outs().resetColor();
      
    }
  }
  
  outs() << "}\n";
}


//struct ProvenancePropPrinter {
//  Function *fn;
//  DenseMap<Value*,Value*>& provenance;
//};
//
//template <>
//struct GraphTraits<ProvenancePropPrinter> : public GraphTraits<const BasicBlock*> {
//  static NodeType *getEntryNode(ProvenancePropPrinter p) { return &p.fn->getEntryBlock(); }
//  
//  // nodes_iterator/begin/end - Allow iteration over all nodes in the graph
//  typedef Function::iterator nodes_iterator;
//  static nodes_iterator nodes_begin(ProvenancePropPrinter p) { return p.fn->begin(); }
//  static nodes_iterator nodes_end  (ProvenancePropPrinter p) { return p.fn->end(); }
//  static size_t         size       (ProvenancePropPrinter p) { return p.fn->size(); }
//};
//
//template<>
//struct DOTGraphTraits<ProvenancePropPrinter> : public DefaultDOTGraphTraits {
//  DOTGraphTraits(bool simple=false): DefaultDOTGraphTraits(simple) {}
//  
//  using FTraits = DOTGraphTraits<const Function*>;
//  
//  static std::string getGraphName(ProvenancePropPrinter f) { return "ProvenanceProp"; }
//  
//  std::string getNodeLabel(const BasicBlock *Node, ProvenancePropPrinter p) {
//    // return FTraits::getCompleteNodeLabel(Node, p.fn);
//    std::string str;
//    raw_string_ostream o(str);
//    
//    o << "<<table border='0' cellborder='0'>";
//    
//    for (auto& inst : *const_cast<BasicBlock*>(Node)) {
//      const char * color = "black";
//      if (p.provenance[&inst] == ProvenanceProp::UNKNOWN)
//        color = "black";
//      else if (p.provenance[&inst] == ProvenanceProp::INDETERMINATE)
//        color = "red";
//      else if (dyn_cast_addr<GLOBAL_SPACE>(inst.getType()))
//        color = "blue";
//      else if (dyn_cast_addr<SYMMETRIC_SPACE>(inst.getType()))
//        color = "cyan";
//      
//      o << "<tr><td align='left'><font color='" << color << "'>" << inst << "</font></td></tr>";
//    }
//    
//    o << "</table>>";
//    
//    return o.str();
//  }
//  
//  template< typename T >
//  static std::string getEdgeSourceLabel(const BasicBlock *Node, T I) {
//    return FTraits::getEdgeSourceLabel(Node, I);
//  }
//  
//  static std::string getNodeAttributes(const BasicBlock* Node, ProvenancePropPrinter p) {
//    return "";
//  }
//};
//
//void ProvenanceProp::viewGraph(Function *fn) {
//  ProvenancePropPrinter ppp = {fn,provenance};
//  ViewGraph(ppp, "provenance." + Twine(fn->getValueID()));
//}

