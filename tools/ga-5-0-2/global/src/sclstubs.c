#if HAVE_CONFIG_H
#   include "config.h"
#endif

#include "global.h"
#include "globalp.h"

#define _MAX_PROB_SIZE_ 10000 /* 100x100 */

void FATR ga_lu_solve_alt_(Integer *tran, Integer * g_a, Integer * g_b) {
  gai_error("ga_lu_solve:scalapack not interfaced",0L);
}

void FATR ga_lu_solve_(char *tran, Integer * g_a, Integer * g_b) {

  Integer dimA1, dimA2, typeA;
  Integer dimB1, dimB2, typeB;

  /** check GA info for input arrays */
  gai_check_handle(g_a, "ga_lu_solve: a");
  gai_check_handle(g_b, "ga_lu_solve: b");
  gai_inquire(g_a, &typeA, &dimA1, &dimA2);
  gai_inquire(g_b, &typeB, &dimB1, &dimB2);
  
  if( (dimA1*dimA2 > _MAX_PROB_SIZE_) || (dimB1*dimB2 > _MAX_PROB_SIZE_) )
    gai_error("ga_lu_solve:Array size too large. Use scalapack for optimum performance. configure --with-scalapack or --with-scalapack-i8 for ga_lu_solve to use Scalapack interface",0L);

  gai_lu_solve_seq(tran, g_a, g_b);
}

Integer FATR ga_llt_solve_(Integer * g_a, Integer * g_b) {
  gai_error("ga_llt_solve:scalapack not interfaced",0L);
  return 0;
}

Integer FATR ga_solve_(Integer * g_a, Integer * g_b) {
  gai_error("ga_solve:scalapack not interfaced",0L);
  return 0;
}

Integer FATR ga_spd_invert_(Integer * g_a) {
  gai_error("ga_spd_invert:scalapack not interfaced",0L);
  return 0;
}

