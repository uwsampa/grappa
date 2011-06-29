#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <vector>

#include "Comm.h"
#include "FuncReg.h"

using namespace std;
using namespace tascel;
using namespace tascel::comm;


TslFuncRegTbl::TslFuncRegTbl() {}

TslFuncRegTbl::~TslFuncRegTbl() {}

TslFunc
TslFuncRegTbl::add(TslFunc_t f) {
  barrier();
  ftbl.push_back(f);
  return ftbl.size() - 1;
}

TslFunc_t
TslFuncRegTbl::get(TslFunc fn) const {
  return ftbl.at(fn);
}

