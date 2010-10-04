#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include "graph.h"

V* G;
E* El;
int n_edge;

double phan1, phan2;
double time1, time2, time3;
double time4, time5;

extern double timer();

/*usage: cmd 0 <n_vertices> <n_edges>*/
int main(int argc, char ** argv)
{ int n,m,opt,i,count;

  opt = atoi(argv[1]);
  time4=timer();
  switch(opt) {
    case 0:
       n = atoi(argv[2]);
       m = atoi(argv[3]);
       rand_graph(n, m, NULL, &El);
       break;
    default:;
  }
  time5=timer();
  printf("Generate Graph time = %lf\n", time5 - time4);

  printf("The first edge is <%d,%d>\n", El[0].v1, El[0].v2);
  printf("The last edge is <%d,%d>\n", El[n-1].v1, El[n-1].v2);

  MTA_DEBUG_REG("accurate\n");
//  MTA_DEBUG_REG("counter phantom\n");
  count = conn_comp_SV(El,n,m);
//  MTA_DEBUG_REG("counter phantom\n");

  printf(" Found %d connected comps\n",count);
  printf("Init  time = %lf\n", time2 - time1);
  printf("Biconn time = %lf\n", time3 - time2);

  printf("\n");
  printf("Total time      = %lf\n", time3 - time1);
  printf("CPU utilization = %lf\n", 1.0 - (phan2 - phan1) / (time3 - time1));

  if (El) free(El);
  return 0;
}
