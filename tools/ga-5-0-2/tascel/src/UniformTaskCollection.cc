#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <cstdio>
#include <cstring>
#include <vector>

#include <armci.h>

#include "Comm.h"
#include "FuncReg.h"
#include "massert.h"
#include "UniformTaskCollection.h"

using namespace std;
using namespace tascel;
using namespace comm;


UniformTaskCollection::UniformTaskCollection(TslFunc _tfn, const TslFuncRegTbl &_frt,
    int _tsk_size, int _ntsks, void *_pldata, int _pldata_len)
  : max_ntsks(_ntsks)
  , tsk_size(_tsk_size)
  , frt(_frt)
  , tfn(_tfn)
  , pldata_len(_pldata_len) {
  pldata = new char[pldata_len];
  memcpy(pldata, _pldata, pldata_len);
}

/* virtual */
UniformTaskCollection::~UniformTaskCollection() {
  delete [] pldata;
}

