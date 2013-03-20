#include "globals.h"

/* Scalable Data Generator parameters - defaults */

void getUserParameters(int scale)
{
  /* SCALE */                             /* Binary scaling heuristic  */
  A                  = 0.55;              /* RMAT parameters           */
  B                  = 0.1;
  C                  = 0.1;
  D                  = 0.25;
  numVertices        =  (1 << scale);     /* Total num of vertices     */
  numEdges           =  8 * numVertices;  /* Total num of edges        */
  maxWeight          =  (1 << scale);     /* Max integer weight        */
  K4approx           =  8;                /* Scale factor for K4       */
  subGraphPathLength =  3;                /* Max path length for K3    */
}
