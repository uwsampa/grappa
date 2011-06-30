#include "ga.h"
#include "macdecls.h"
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <assert.h>

// #define min(x,y) (((x)<=(y))?(x):(y))

#define maxatom  286
#define maxnbfn  (15*maxatom)
#define mxiter   30
#define maxnnbfn ((maxnbfn)*(maxnbfn+1)/2)
#define pi       3.141592653589793d0)
#define tol      (0.5e-3)
#define tol2e    (1.0e-6)

#define ichunk 20

#ifdef __cplusplus
extern "C" {
#endif
  extern int armci_me;
  void nga_get_(Integer *g_a,
		Integer *lo,
		Integer *hi,
		void    *buf,
		Integer *ld);

  void g_(double *gg, Integer *i, Integer *j, Integer *k, Integer *l);
  double contract_matrices_(Integer *g_a, Integer *g_b);
  void new_nga_acc_(Integer *g_a,
		    Integer *lo,
		    Integer *hi,
		    void    *buf,
		    Integer *ld,
		    void    *alpha);

  void nga_acc_(Integer *g_a,
		Integer *lo,
		Integer *hi,
		void    *buf,
		Integer *ld,
		void    *alpha);

  void ctwoel_(Integer *g_schwarz, Integer *g_dens, Integer *g_fock,
	       double *schwmax, double *etwo,
	       long long *pnbfn, long long *icut1, long long *icut2,
	       long long *icut3, long long *icut4);

#ifdef __cplusplus
}
#endif


static void cpptwoel(Integer g_schwarz, Integer g_dens, Integer g_fock,
		     double schwmax, Integer nbfn,
		     long long *icut1, long long *icut2,
		     long long *icut3, long long *icut4);

void ctwoel_(Integer *g_schwarz, Integer *g_dens, Integer *g_fock,
	     double *schwmax, double *etwo,
	     long long *pnbfn, long long *icut1, long long *icut2,
	     long long *icut3, long long *icut4) {
  const Integer nbfn = (Integer)*pnbfn;
  long long ijcnt,klcnt,ijklcnt;

  ijcnt = *icut1;
  klcnt = *icut2;
  ijklcnt = *icut3;

  cpptwoel(*g_schwarz, *g_dens, *g_fock, *schwmax, nbfn, 
	   icut1, icut2, icut3, icut4);

  *etwo = 0.5*contract_matrices_(g_fock,g_dens);

  ijcnt = *icut1 - ijcnt;
  klcnt = *icut2 - klcnt;
  ijklcnt = (*icut3) - ijklcnt;
  *icut4 = *icut3;
  if (*icut3 <= 0L) {
    /*     no integrals may be calculated if there is no work for  */
    /*     this node (ichunk too big), or, something is wrong */
    printf("no two-electron integrals computed by node %d\n", GA_Nodeid());
    fflush(stdout);
  }
  return;
}

/*************************************************************************
 *
 * the code involving task pool stuff follows
 *
 *************************************************************************/

#include <UniformTaskCollectionSplit.h>
#include <algorithm>

using namespace std;
using namespace tascel;

static int next_4chunk(long itask, Integer nbfn, 
		       Integer lo[4], Integer hi[4], 
		       Integer *ilo, Integer *jlo,
		       Integer *klo, Integer *llo) {
  const long one=1;
  const int zero=0;
  long imax;
  Integer itmp;
  long long total_ntsks;
  imax = nbfn/ichunk;
  if (nbfn - ichunk*imax > 0) {
    imax = imax + 1;
  }
  total_ntsks = imax*imax*imax*imax;

/*   printf("%d: itask=%d total_ntsks=%lf\n", armci_me, (int)itask, (double)total_ntsks); */
  if (itask < 0) {
    printf("next_4chunk: itask negative: %d imax:%d nbfn:%d ichunk:%d\n",
	   (int) itask, (int)imax, (int)nbfn, (int)ichunk);
    printf("probable GA integer precision problem if %lf > 2^31\n", 
	   pow(imax*1.0, 4));
    fflush(stdout);
    assert(0);
  }
  if (itask < total_ntsks) {
    itmp = itask;
    *ilo = itmp % imax;
    itmp /= imax;
    *jlo = itmp %imax;
    itmp /= imax;
    *klo = itmp % imax;
    itmp /= imax;
    assert(itmp < imax);
    *llo = itmp;

    lo[0] = *ilo*ichunk;
    lo[1] = *jlo*ichunk;
    lo[2] = *klo*ichunk;
    lo[3] = *llo*ichunk;
    hi[0] = min((*ilo+1)*ichunk,nbfn) - 1;
    hi[1] = min((*jlo+1)*ichunk,nbfn) - 1;
    hi[2] = min((*klo+1)*ichunk,nbfn) - 1;
    hi[3] = min((*llo+1)*ichunk,nbfn) - 1;

    return 1;
  }
  else {
    return 0;
  }
}


static void clean_chunk(double *chunk) {
  int i;
  for(i=0; i<ichunk*ichunk; i++) {
    chunk[i] = 0.0;
  }
}

typedef struct {
  Integer lo[4], hi[4], it, jt, kt, lt;
} task_dscr_t;

typedef struct {
  double f_ij[ichunk][ichunk],d_kl[ichunk][ichunk];
  double f_ik[ichunk][ichunk],d_jl[ichunk][ichunk];
  double s_ij[ichunk][ichunk],s_kl[ichunk][ichunk];
} task_bufs_t;

typedef struct {
  Integer nbfn;
  double schwmax;
  int cnt1, cnt2;
  long long *icut1, *icut2, *icut3, *icut4;
  Integer g_schwarz, g_dens, g_fock;
} task_plo_t;

static void twoel_task(UniformTaskCollection *utc, void *bigd, int bigd_len, 
		       void *pldata, int pldata_len, vector<void *> data_bufs);

static void cpptwoel(Integer g_schwarz, Integer g_dens, Integer g_fock,
		     double schwmax, Integer nbfn,
		     long long *icut1, long long *icut2,
		     long long *icut3, long long *icut4) {
  int dotask;
  int g_counter, ione = 1;
  task_dscr_t tdscr;
  task_plo_t tplo;

  {
    tplo.nbfn = nbfn;
    tplo.schwmax = schwmax;
    tplo.icut1 = icut1;
    tplo.icut2 = icut2;
    tplo.icut3 = icut3;
    tplo.icut4 = icut4;
    tplo.g_schwarz = g_schwarz;
    tplo.g_dens = g_dens;
    tplo.g_fock = g_fock;

    TslFuncRegTbl frt;
    TslFunc tf = frt.add(twoel_task);

    const long imax = nbfn/ichunk + ((nbfn%ichunk) ? 1 : 0);
    const long total_ntasks = imax*imax*imax*imax;
    const long ntasks_per_proc = (long)ceil(1.0*total_ntasks/GA_Nnodes());
    const long tasklo = ntasks_per_proc * GA_Nodeid();
    const long taskhi = min(tasklo+ntasks_per_proc,total_ntasks)-1;

    UniformTaskCollectionSplit utc(tf, frt, sizeof(task_dscr_t), ntasks_per_proc, &tplo, sizeof(tplo));
    for(long i=tasklo; i<=taskhi; i++) {
      next_4chunk(i, nbfn, tdscr.lo, tdscr.hi,
		  &tdscr.it, &tdscr.jt, &tdscr.kt, &tdscr.lt);
      utc.addTask(&tdscr, sizeof(tdscr));
    }
    GA_Sync();
    utc.process();
  }
}

static task_bufs_t tbufs;

static void twoel_task(UniformTaskCollection *utc, void *_bigd, int bigd_len, 
		       void *pldata, int pldata_len, vector<void *> data_bufs) {
  task_dscr_t *ptdscr = (task_dscr_t*)_bigd;
  task_bufs_t *ptbufs = &tbufs;
  task_plo_t *ptplo = (task_plo_t*)pldata;
 
  assert(_bigd!=NULL);
  assert(bigd_len == sizeof(task_dscr_t));
  assert(pldata!=NULL);
  assert(pldata_len == sizeof(task_plo_t));
  assert(data_bufs.size()==0);

  int ich;
  Integer lch;
  Integer lo_ik[2],hi_ik[2],lo_jl[2],hi_jl[2];
  Integer i,j,k,l,iloc,jloc,kloc,lloc,ld;
  double gg;
  Integer ione = 1;
  double one = 1.0;
  Integer nbfn;
  Integer *lo, *hi, it, jt, kt, lt;
  long long *icut1, *icut2, *icut3, *icut4;
  Integer g_schwarz, g_dens, g_fock;
  double schwmax;

  lo = ptdscr->lo;
  hi = ptdscr->hi;
  it = ptdscr->it;
  jt = ptdscr->jt;
  kt = ptdscr->kt;
  lt = ptdscr->lt;
  nbfn = ptplo->nbfn;
  schwmax = ptplo->schwmax;
  icut1 = ptplo->icut1;
  icut2 = ptplo->icut2;
  icut3 = ptplo->icut3;
  icut4 = ptplo->icut4;
  g_schwarz = ptplo->g_schwarz;
  g_dens = ptplo->g_dens;
  g_fock = ptplo->g_fock;  

  ld = maxnbfn;
  ich = lch = ichunk;

  lo_ik[0] = lo[0];
  lo_ik[1] = lo[2];
  hi_ik[0] = hi[0];
  hi_ik[1] = hi[2];
  lo_jl[0] = lo[1];
  lo_jl[1] = lo[3];
  hi_jl[0] = hi[1];
  hi_jl[1] = hi[3];
  assert(ich == hi[0]-lo[0]+1);
  assert(ich == hi[2]-lo[2]+1);
  assert(ich == hi_jl[0]-lo_jl[0]+1);
  
  {
    /*lkji*/
    int _lo[4] = {lo[3], lo[2], lo[1], lo[0]};
    int _hi[4] = {hi[3], hi[2], hi[1], hi[0]};
    int _lo_jl[2] = {lo_jl[1], lo_jl[0]};
    int _hi_jl[2] = {hi_jl[1], hi_jl[0]};
    NGA_Get(g_schwarz, &_lo[2], &_hi[2], ptbufs->s_ij, &ich);
    NGA_Get(g_schwarz, &_lo[0], &_hi[0], ptbufs->s_kl, &ich);
    NGA_Get(g_dens,&_lo[0],&_hi[0], ptbufs->d_kl,&ich);
    NGA_Get(g_dens,_lo_jl,_hi_jl, ptbufs->d_jl,&ich);
  }
    
  clean_chunk((double*)ptbufs->f_ij);
  clean_chunk((double *)ptbufs->f_ik);
  for(i = lo[0]; i<=hi[0]; i++) {
    iloc = i-lo[0];
    for(j = lo[1]; j<= hi[1]; j++) {
      jloc = j-lo[1];
      if (ptbufs->s_ij[jloc][iloc]*(schwmax) < tol2e) {
	*icut1 = *icut1 + (hi[2]-lo[2]+1)*(hi[3]-lo[3]+1);
      }
      else {
	/* *cnt1+=1;*/
	for(k = lo[2]; k<=hi[2]; k++) {
	  kloc = k-lo[2];
	  for(l = lo[3]; l<=hi[3]; l++) {
	    lloc = l-lo[3];
	    if (ptbufs->s_ij[jloc][iloc]*ptbufs->s_kl[lloc][kloc] < tol2e) {
	      *icut2 = *icut2 + 1;
	    }
	    else  {
	      Integer _i=i+1, _j=j+1, _k=k+1, _l=l+1;
	      g_(&gg, &_i, &_j, &_k, &_l);
	      /* *cnt2 += 1;*/
	      ptbufs->f_ij[jloc][iloc] = ptbufs->f_ij[jloc][iloc] + gg*ptbufs->d_kl[lloc][kloc];
	      ptbufs->f_ik[kloc][iloc] = ptbufs->f_ik[kloc][iloc] - 0.5*gg*ptbufs->d_jl[lloc][jloc];
	      *icut3 = *icut3 + 1;
	    }
	  }
	}
      }
    }
  }
  {
    /*lkji*/
    int _lo[2] = {lo[1], lo[0]};
    int _hi[2] = {hi[1], hi[0]};
    int _lo_ik[2] = {lo_ik[1], lo_ik[0]};
    int _hi_ik[2] = {hi_ik[1], hi_ik[0]};
    NGA_Acc(g_fock,_lo,_hi, ptbufs->f_ij,&ich,&one);
    NGA_Acc(g_fock,_lo_ik,_hi_ik, ptbufs->f_ik,&ich,&one);
  }
}

