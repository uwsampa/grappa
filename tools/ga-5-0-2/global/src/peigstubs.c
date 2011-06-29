#if HAVE_CONFIG_H
#   include "config.h"
#endif

#include "global.h"
#include "globalp.h"

#if NOFORT
void FATR ga_diag_seq_(Integer *g_a, Integer *g_s, Integer *g_v, 
		       DoublePrecision *eval) {
  gai_error("ga_diag_seq:peigs not interfaced",0L);
}

void FATR ga_diag_std_seq_(Integer * g_a, Integer * g_v, 
			   DoublePrecision *eval) {
  gai_error("ga_diag_std_seq:peigs not interfaced",0L);
}
#endif

void FATR ga_diag_(Integer * g_a, Integer * g_s, Integer * g_v, 
		   DoublePrecision *eval) {
    gai_error("ga_diag:peigs not interfaced",0L);
}

void FATR ga_diag_std_(Integer * g_a, Integer * g_v, DoublePrecision *eval) {
  gai_error("ga_diag:peigs not interfaced",0L);
}

void FATR ga_diag_reuse_(Integer * reuse, Integer * g_a, Integer * g_s, 
		   Integer * g_v, DoublePrecision *eval) {
  gai_error("ga_diag_reuse:peigs not interfaced",0L);
}
