#include "Passes.h"
#include <llvm/Analysis/CFGPrinter.h>
#include <llvm/Support/GraphWriter.h>
#include <llvm/Support/CommandLine.h>

using namespace Grappa;

//////////////////////////////
// Register optional pass
static RegisterPass<ProvenanceProp> X( "provenance-prop", "Provenance Prop", false, false );
char ProvenanceProp::ID = 0;

static cl::opt<bool> DisableANSI("no-color",
                              cl::desc("Disable ANSI colors."));

Value* ProvenanceProp::search(Value *val, int depth) {
  if (depth > 20) return UNKNOWN;
  if (!val) return UNKNOWN;
  
  // see if we already know the answer
  if (provenance.count(val)) {
    return provenance[val];
  }
  
  // check if it's a potentially-valid pointer in its own right
  switch (classify(val)) {
    case Unknown:
      break; // need to recurse
    default:
      return (provenance[val] = val);
  }
  
  // now start inspecting the instructions and recursing
  auto i = dyn_cast<Instruction>(val);
  
  assert(i && "val not an instruction?");
  
  
  if (i->getNumOperands() == 0) return INDETERMINATE;
  
  //  if (isa<GetElementPtrInst>(i) || isa<CallInst>(i) || isa<InvokeInst>(i)) {
  auto r = search(i->getOperand(0), depth+1);
  for (int o = 1; o < i->getNumOperands(); o++) {
    r = meet(r, search(i->getOperand(o), depth+1));
  }
  return (provenance[i] = r);
//  } else if (auto c = dyn_cast<CastInst>(i)) {
//    return (provenance[i] = search(c->getOperand(0), depth+1));
//  } else if (auto ld = dyn_cast<LoadInst>(i)) {
//    return (provenance[i] = search(ld->getPointerOperand(), depth+1));
//  } else if (auto st = dyn_cast<StoreInst>(i)) {
//    return (provenance[i] = search(st->getPointerOperand(), depth+1));
//  } else if (isa<BinaryOperator>(i) || isa<CmpInst>(i)) {
//    return (provenance[i] = meet(
//             search(i->getOperand(0), depth+1),
//             search(i->getOperand(1), depth+1))
//           );
//  }
//
//  return UNKNOWN;
}


bool ProvenanceProp::runOnFunction(Function& F) {
  
  for (auto& bb : F) {
    for (auto& inst : bb) {
      if (provenance.size() > 1<<12) goto done;
      search(&inst);
    }
  }
  
done:
  return false;
}

ProvenanceClass ProvenanceProp::classify(Value* v) {
  
  if (v == ProvenanceProp::UNKNOWN)
    return ProvenanceClass::Unknown;
  
  if (v == ProvenanceProp::INDETERMINATE)
    return ProvenanceClass::Indeterminate;
  
  if (isa<GlobalVariable>(v) || isa<Argument>(v) || isa<MDNode>(v) || isa<BasicBlock>(v))
    return ProvenanceClass::Static;
  
  if (isa<AllocaInst>(v))
    return ProvenanceClass::Stack;
  
  if (isa<Constant>(v))
    return ProvenanceClass::Const;
  
  if (dyn_cast_addr<SYMMETRIC_SPACE>(v->getType()))
    return ProvenanceClass::Symmetric;
  
  if (dyn_cast_addr<GLOBAL_SPACE>(v->getType()))
    return ProvenanceClass::Global;
  
  return ProvenanceClass::Unknown;
}

Value* ProvenanceProp::meet(Value* a, Value* b) {
  ProvenanceClass ac = classify(provenance[a]),
                  bc = classify(provenance[b]);
  
  if (ac == Indeterminate || bc == Indeterminate) {
    return INDETERMINATE;
  } else if (ac == Global || bc == Global) {
    if (ac == Global && bc == Global)
      return (a == b) ? a : INDETERMINATE;
    else if (ac == Global)
      return a;
    else
      return b;
  } else if (ac == Stack || bc == Stack) {
    if (ac == Stack)
      return a;
    else
      return b;
  } else if (ac == Symmetric || bc == Symmetric) {
    if (ac == Symmetric)
      return a;
    else
      return b;
  } else if (ac == Static || bc == Static) {
    if (ac == Static)
      return a;
    else
      return b;
  } else if (ac == Const && bc == Const) {
    return a;
  } else {
    return UNKNOWN;
  }
}

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
      
      switch (classify(provenance[&inst])) {
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


struct ProvenancePropPrinter {
  Function *fn;
  DenseMap<Value*,Value*>& provenance;
};

template <>
struct GraphTraits<ProvenancePropPrinter> : public GraphTraits<const BasicBlock*> {
  static NodeType *getEntryNode(ProvenancePropPrinter p) { return &p.fn->getEntryBlock(); }
  
  // nodes_iterator/begin/end - Allow iteration over all nodes in the graph
  typedef Function::iterator nodes_iterator;
  static nodes_iterator nodes_begin(ProvenancePropPrinter p) { return p.fn->begin(); }
  static nodes_iterator nodes_end  (ProvenancePropPrinter p) { return p.fn->end(); }
  static size_t         size       (ProvenancePropPrinter p) { return p.fn->size(); }
};

template<>
struct DOTGraphTraits<ProvenancePropPrinter> : public DefaultDOTGraphTraits {
  DOTGraphTraits(bool simple=false): DefaultDOTGraphTraits(simple) {}
  
  using FTraits = DOTGraphTraits<const Function*>;
  
  static std::string getGraphName(ProvenancePropPrinter f) { return "ProvenanceProp"; }
  
  std::string getNodeLabel(const BasicBlock *Node, ProvenancePropPrinter p) {
    // return FTraits::getCompleteNodeLabel(Node, p.fn);
    std::string str;
    raw_string_ostream o(str);
    
    o << "<<table border='0' cellborder='0'>";
    
    for (auto& inst : *const_cast<BasicBlock*>(Node)) {
      const char * color = "black";
      if (p.provenance[&inst] == ProvenanceProp::UNKNOWN)
        color = "black";
      else if (p.provenance[&inst] == ProvenanceProp::INDETERMINATE)
        color = "red";
      else if (dyn_cast_addr<GLOBAL_SPACE>(inst.getType()))
        color = "blue";
      else if (dyn_cast_addr<SYMMETRIC_SPACE>(inst.getType()))
        color = "cyan";
      
      o << "<tr><td align='left'><font color='" << color << "'>" << inst << "</font></td></tr>";
    }
    
    o << "</table>>";
    
    return o.str();
  }
  
  template< typename T >
  static std::string getEdgeSourceLabel(const BasicBlock *Node, T I) {
    return FTraits::getEdgeSourceLabel(Node, I);
  }
  
  static std::string getNodeAttributes(const BasicBlock* Node, ProvenancePropPrinter p) {
    return "";
  }
};

void ProvenanceProp::viewGraph(Function *fn) {
  ProvenancePropPrinter ppp = {fn,provenance};
  ViewGraph(ppp, "provenance." + Twine(fn->getValueID()));
}

