#include <convey/usr/cny_comp.h>
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned long long uint64;
extern long cpTestEx1();

int main(int argc, char *argv[])
{
  int ae_idx = 0;
  uint64 ret_l = 0;
  int index = 1;
  long i;
  long size;
  uint64  *a1, *a2, *a3, *ae_sum;
  uint64  a1_aligned, a2_aligned, a3_aligned;
  uint64 act_sum;
  uint64 exp_sum=0;

  if (argc == 1)
    size = 100;		// default size
  else if (argc == 2) {
    size = atoi(argv[1]);
    if (size > 0) {
      printf("Running UserApp.exe with size = %lld\n", size);
    } else {
      usage (argv[0]);
      return 0;
    }
  }
  else {
    usage (argv[0]);
    return 0;
  }

  printf("Getting signature\n");

  // Get PDK signature
  cny_image_t        sig2;
  cny_image_t        sig;
  if (cny$get_signature_fptr)
    (*cny$get_signature_fptr) ("pdk", &sig, &sig2);
  else 
    fprintf(stderr,"where is my get_sig ptr\n");
 
  // Sample program 1
  // 1.  Allocate coproc memory for 2 arrays of numbers 
  // 2.  Populate arrays with 64-bit integers
  // 3.  Call coproc routine to sum the numbers
  printf("Running Sample Program 1\n");

  // check interleave is binary
  if (cny_cp_interleave() == CNY_MI_3131) {
    printf("ERROR - interleave set to 3131, this personality requires binary interleave\n");
    exit (1);
  }

  // Allocate memory
  if (cny$cp_malloc_fptr)  {
    a1 = (uint64 *) (*cny$cp_malloc_fptr)(size*8 + 1024);
    a2 = (uint64 *) (*cny$cp_malloc_fptr)(size*8 + 1024);
    a3 = (uint64 *) (*cny$cp_malloc_fptr)(size*8 + 1024);
    ae_sum = (uint64 *) (*cny$cp_malloc_fptr)(4*8);
  }
  else 
    printf("malloc failed\n");

  // align array addresses on MC0 
  a1_aligned = (uint64)a1 & ~0x3ffLL;
  a2_aligned = (uint64)a2 & ~0x3ffLL;
  a3_aligned = (uint64)a3 & ~0x3ffLL;
  a1_aligned += 1LL << 10;
  a2_aligned += 1LL << 10;
  a3_aligned += 1LL << 10;

#if 0
  printf("a1    = %llx -> %llx (aligned for MCs)\n", a1, a1_aligned);
  printf("a2    = %llx -> %llx (aligned for MCs)\n", a2, a2_aligned);
  printf("a3    = %llx -> %llx (aligned for MCs)\n", a3, a3_aligned);
  printf("ae_sum   = %llx \n", ae_sum);
#endif

  a1 = (uint64 *) a1_aligned;
  a2 = (uint64 *) a2_aligned;
  a3 = (uint64 *) a3_aligned;

  // populate operand arrays, initialize a3
  for (i = 0; i < size; i++) {
    a1[i] = i;
    a2[i] = 2 * i;
  }

  act_sum = l_copcall_fmt(sig, cpTestEx1, "AAAAA", a1, a2, a3, size, ae_sum);

  for (i = 0; i < size; i++) {
    assert(a3[i] == a1[i] + a2[i]);
    exp_sum += a1[i] + a2[i];
    printf("a1[%d]=%lld + a2[%d]=%lld = a3[%d]=%lld\n", i, a1[i], i, a2[i], i, a3[i]);
  }

  assert(act_sum == exp_sum);

#if 1
  printf("Sample 1 test passed - sum=%lld\n", exp_sum);
#else
  if ((exp_sum==act_sum) && 
      (act_sum==ae_sum[0]) &&
      (act_sum==ae_sum[1]) &&
      (act_sum==ae_sum[2]) &&
      (act_sum==ae_sum[3]))
			
    printf("Sample 1 test passed - sum=%lld\n", exp_sum);
  else {
    printf("Sample 1 test failed - expected sum=%lld, actual sum=%lld\n", exp_sum, act_sum);
    printf("ae_sum[0] = %lld\n", ae_sum[0]);
    printf("ae_sum[1] = %lld\n", ae_sum[1]);
    printf("ae_sum[2] = %lld\n", ae_sum[2]);
    printf("ae_sum[3] = %lld\n", ae_sum[3]);
  }
#endif

  return 0;
}

// Print usage message and exit with error.
usage (p)
char *p;
{
    printf("usage: %s [count (default 100)] \n", p);
    exit (1);
}

