// Originally from GraphCT, version 0.6.0
// License: GeorgiaTech

#include <stdio.h>
#include <stdlib.h>
#ifdef __MTA__
#include <sys/mta_task.h>
#include <machine/mtaops.h>
#include <machine/runtime.h>
#else
#include "compat/xmt-ops.h"
#endif
#include "defs.h"

graphint connectedComponents(graph *g) {
  graphint count = 0;
  graphint nchanged;
  
  OMP("omp parallel") {
    const graphint NV = g->numVertices;
    const graphint NE = g->numEdges;
    const graphint * restrict sV = g->startVertex;
    const graphint * restrict eV = g->endVertex;
    color_t * restrict D = g->marks;
    
    OMP("omp for")
    for (color_t i = 0; i < NV; ++i) {
      D[i] = i;
    }
    
    while (1) {
      OMP("omp single") nchanged = 0;
      OMP("omp barrier");
      MTA("mta assert nodep") OMP("omp for reduction(+:nchanged)")
      for (graphint k = 0; k < NE; ++k) {
        const graphint i = sV[k];
        const graphint j = eV[k];
        if (D[i] < D[j] && D[j] == D[D[j]])	{
          D[D[j]] = D[i];
          ++nchanged;
        }
      }
      if (!nchanged) break;
      MTA("mta assert nodep") OMP("omp for")
      for (graphint i = 0; i < NV; ++i) {
        while (D[i] != D[D[i]]) {
          D[i] = D[D[i]];
        }
      }
    }
    
    MTA("mta assert nodep") OMP("omp for reduction(+:count)")
    for (graphint i = 0; i < NV; ++i) {
      while (D[i] != D[D[i]]) {
        D[i] = D[D[i]];
      }
      if (D[i] == i) ++count;
    }
  }
  
  return count;
}
