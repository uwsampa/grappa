#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <cmath>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <vector>

#include <armci.h>

#include "Comm.h"
#include "FuncReg.h"
#include "massert.h"
#include "Sleep.h"
#include "UniformTaskCollectionSplit.h"

using namespace std;
using namespace tascel;
using namespace tascel::comm;


UniformTaskCollectionSplit::UniformTaskCollectionSplit(TslFunc _tfn,
    const TslFuncRegTbl &_frt, int _tsk_size, int _ntsks,
    void *pldata, int pldata_len)
  : UniformTaskCollection(_tfn, _frt, _tsk_size, _ntsks, pldata, pldata_len)
  , sq(_tsk_size, _ntsks) {
}

/*virtual */
UniformTaskCollectionSplit::~UniformTaskCollectionSplit() { }

void
UniformTaskCollectionSplit::process() {
  int p;
  bool got_work;
  barrier();
  TslFunc_t fn = frt.get(tfn);
  char buf[tsk_size];
  int tasksDone = 0, stealAttempts = 0, steals = 0;

  while (!sq.hasTerminated())  {
    while (sq.getTask(buf, tsk_size)) {
      tasksDone += 1;
      fn(this, buf, tsk_size, pldata, pldata_len, vector<void *>());
    }
    sq.td_progress();
    got_work = false;
    while(got_work == false && !sq.hasTerminated()) {
      do {
	p = rand() % nproc();
      }
      while (p == me());
      got_work = sq.steal(p);
      stealAttempts += 1;
    }
    if(got_work) {
      steals += 1;
    }
  }
  //printf("%d: processed %d tasks attempts=%d steals=%d\n", me(), tasksDone, stealAttempts, steals);
  barrier();
}

void
UniformTaskCollectionSplit::addTask(void *data, int dlen) {
  massert(dlen == tsk_size);
  sq.addTask(data, dlen);
  return;

error:
  throw TSL_ERR;
}

