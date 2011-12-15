#include "globals.h"

void getUserParameters(int64_t scale, int64_t edgefactor)
{
  /* SCALE */                             /* Binary scaling heuristic  */
  A                  = 0.55;              /* RMAT parameters           */
  B                  = 0.1;
  C                  = 0.1;
  D                  = 0.25;
  numVertices        =  (1 << scale);     /* Total num of vertices     */
  numEdges           =  edgefactor * numVertices;  /* Total num of edges        */
  maxWeight          =  (1 << scale);     /* Max integer weight        */
  K4approx           =  8;                /* Scale factor for K4       */
  subGraphPathLength =  3;                /* Max path length for K3    */
}
