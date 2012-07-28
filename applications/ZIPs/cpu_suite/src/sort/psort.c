/* CVS info                         */
/* $Date: 2010/04/02 20:43:44 $     */
/* $Revision: 1.8 $                 */
/* $RCSfile: psort.c,v $            */
/* $State: Stab $:                */

#include "common_inc.h"
#include <stdio.h>
#include <stdlib.h>
#include "error_routines.h"

static char cvs_info[] = "BMARKGRP $Date: 2010/04/02 20:43:44 $ $Revision: 1.8 $ $RCSfile: psort.c,v $ $Name:  $";

typedef unsigned long SORTDTYPE;
typedef unsigned char uChar;
void PSORT(SORTDTYPE *arr, int n);
void psort(SORTDTYPE *arr, int n, uChar hb, uChar lb, int psz);
void ShellSort(SORTDTYPE *data, int n);

/*===========================================================================
**   The following 3 parameters may be modified to improve performance
**   during the optimized run. They specify when to switch to the shell sort
**   and what pocket sizes to use for each sort case. These values must be
**   modified only through the compiler -D flag and must be held constant
**   for each tuned run of the benchmark (in particular SmallSortSize must
**   be fixed for both the smaller and larger sorts)
**==========================================================================*/

#ifndef SmallSortSz
#define SmallSortSz  96
#endif
#ifndef PSZ1
#define    PSZ1       6
#endif
#ifndef PSZ3
#define    PSZ3       8
#endif

/*===========================================================================
**   End of modifiable parmeters
**===========================================================================*/

#define MAXLVL    ( (PSZ1 < PSZ3) ? ( (64+PSZ1-1)/PSZ1 ) : ((64+PSZ3-1)/PSZ3) )

#define TWOTO(A)  (((SORTDTYPE)1)<<(A))

void psort1(SORTDTYPE *arr);
void psort2(SORTDTYPE *data, int n, uChar lvl);
void SmallSort(SORTDTYPE *data, int n);

static uChar mlvl,lvl;
static SORTDTYPE sortMask = 0;

typedef unsigned int POCKETTYPE;

struct _psort {
  uChar sft;
  POCKETTYPE npocks;
  SORTDTYPE msk;
  int   *p;
} PStory[MAXLVL+2];

/* an in-place pocket sort */
void
PSORT(SORTDTYPE *arr, int n)
{

  int psz;
  if (n>TWOTO(20)) {
    psz = PSZ1;
  } else if (n>TWOTO(17)) {
    psz = 10;
  } else if (n>TWOTO(14)) {
    psz = PSZ3;
  } else {
    psz = 8;
  }
  psort(arr,n,(8*sizeof(SORTDTYPE)-1),0,psz);

}

/*
 * in-place pocket sort which sorts under a mask
 */


void
psort(SORTDTYPE *arr, int n, uChar hb, uChar lb, int psz)
{
  int *pdata;
  struct _psort *Pp;

  PStory[0].npocks=1;
  Pp = PStory+1;
  psz &= 31; /* limit pockets to 32 bits! */
  sortMask = (sortMask==0) ? (2*TWOTO(hb-lb)-1lu)<<lb : sortMask;

  mlvl = 0;
  hb -= lb; hb++;
  while (hb>=psz) {
    hb -= psz;
    Pp->npocks = TWOTO(psz);
    Pp->sft    = hb+lb;
    Pp->msk    = (TWOTO(psz)-1)&(sortMask>>(hb+lb));
    mlvl++; Pp++;
  }
  if (hb) {
    Pp->npocks = TWOTO(hb);
    Pp->sft    = lb;
    Pp->msk    = (TWOTO(hb)-1)&(sortMask>>lb);
    mlvl++; Pp++;
  }

#ifdef CPLUSPLUS
  pdata = new int[mlvl*TWOTO(psz+1)+4];
#else
  if ((pdata=(int *)malloc((mlvl*TWOTO(psz+1)+4)*sizeof(int)))==NULL) {
    fprintf(stderr,"psort: unable to allocate space\n");
    err_msg_exit("ERROR: psort unable to allocate space\n");
  }
#endif
  PStory[0].p = pdata;
  PStory[0].p[0] = n;
  {
    int i;
    for (i=1; i<=mlvl; i++)
      PStory[i].p = (pdata+4)+(i-1)*TWOTO(psz+1);
  }

  lvl=0;
  psort1(arr);
#ifdef CPLUSPLUS
  delete pdata;
#else
  free(pdata);
#endif
}

void
psort1(SORTDTYPE *arr)
{
  int j,s,t;
  s = 0;
  for (j=0; j<PStory[lvl].npocks; j++) {
    t = PStory[lvl].p[2*j];
    if (t>s+1) {
      if ((t-s)<SmallSortSz) {
	ShellSort(arr+s,t-s);
      } else {
	psort2(arr+s,t-s,lvl+1);
	if (++lvl < mlvl) psort1(arr+s);
	lvl--;
      }
    }
    s = t;
  }
}

void
psort2(SORTDTYPE *data, int n, uChar lvl)
{
  int k;
  SORTDTYPE x,y,m;

  uChar sft;
  POCKETTYPE npocks;
  SORTDTYPE msk;
  int   *pocket,*docket;

  {
    struct _psort *Pp;
    Pp = PStory+lvl;
    sft = Pp->sft;
    npocks = Pp->npocks;
    msk = Pp->msk;
    pocket = Pp->p;
  }

  /* run through the data */
  {
    int i;
    docket = pocket+1;
    for (i=0; i<2*npocks; i+=2) docket[i] = 0;
    i=0;
    while (i<n) {
      SORTDTYPE d0;
      d0 = (data[i++]>>sft)&msk;
      docket[2*d0]++;
    }
  }

  /* make the running sum */
  {
    int i,sum;
    int *pock = pocket;
    for (sum=i=0; i<npocks; i+=4,pock+=8) {
      int x0,x1,x2,x3;
      x0 = pock[1];
      x1 = pock[3];
      x2 = pock[5];
      x3 = pock[7];
      pock[0] = sum; sum += x0;
      pock[2] = sum; sum += x1;
      pock[4] = sum; sum += x2;
      pock[6] = sum; sum += x3;
    }
  }

  /* move the data */
  {
    SORTDTYPE StartPocket;
    x = data[0];
    StartPocket = 0;
    while (docket[StartPocket]==0) StartPocket+=2;
    if (docket[StartPocket]==n) return; /* nothing to do */
    k = 0;
    while (1) {
      int id,d;
      m  = (x>>sft)&msk;
      m <<= 1;
      id = pocket[m]++;
      d = --docket[m];
      y = data[id];
      data[id]=x;
      if (m==StartPocket) {
	if (k==n-1) return;
	if (d==0) {
	  /* must find unfinished pocket */
	  m=0;
	  while (((d=docket[m])==0)&&(m<npocks*2)) m+=2;
	  if (m==npocks*2) {
	    fprintf(stderr,"Error Error Captain Danger!\n");
	    return;
	  }
	  x=data[pocket[m]];
	  StartPocket=m;
	} else {
	  x=data[id+1];
	}
      } else {
	x=y;
      }
      k++;
    }
  }
}


void
ShellSort(SORTDTYPE *data, int n)
{
#define MaxShellLvl 3
  static uChar ShellSteps[] = { 1,  5, 17 };
  static short ShellBlock[] = { 1,  5, 85, SmallSortSz };
  int BlockSz = n;
  int l=MaxShellLvl;
#undef  MaxShellLvl

  if (n<=1) return;
  while (n<ShellBlock[l-1]) { l--; }
  while (l--) {
    int nStep;
    nStep = ShellSteps[l];

    {
      int block;
      for (block=0; block<n; block+=BlockSz) {
	int Limit,cut,i;
	for (cut=0; cut<nStep; cut++) {
	  SORTDTYPE *dp;
	  dp = data+block+cut;
	  {
	    int tmp;
	    tmp = n-block-cut;
	    Limit = (tmp<BlockSz) ? tmp : BlockSz;
	  }
	  for (i=0; i<Limit-nStep; i+=nStep) {
	    SORTDTYPE t0,t2;
	    int ii,j;
	    t2 = dp[i];
	    t0 = t2&sortMask;
	    ii = i;
	    for (j=i+nStep; j<Limit; j+=nStep) {
	      SORTDTYPE t1;
	      t1 = dp[j];
	      t1 &= sortMask;
	      if (t1<t0) {
		t0 = t1; ii = j;
	      }
	    }
	    if (ii!=i) {
	      SORTDTYPE t1; t1 = dp[ii]; dp[ii]=t2; dp[i]=t1;
	    }
	  }
	}
      }
    }
    BlockSz = ShellSteps[l];
  }

  /* insertion on the mostly sorted list */
  {
    int i;
    for (i=1; i<n; i++) {
      int j,k;
      SORTDTYPE t0,t1,t2;
      t2 = data[i];
      t0 = t2&sortMask;
      j = i-1;
      while ((j>=0)&&((data[j]&sortMask)>t0)) j--;
      k=i-1;
      t1 = data[k];
      while (k>j) {
	(data+1)[k] = t1;
	t1 = data[--k];
      }
      (data+1)[k] = t2;
    }
  }
}

/*
Emacs reads the stuff in this comment to set local variables
Local Variables:
compile-command: "make psort.o"
End:
*/
