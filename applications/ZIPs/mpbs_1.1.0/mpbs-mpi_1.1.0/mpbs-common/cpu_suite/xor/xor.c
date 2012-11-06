/* CVS info                         */
/* $Date: 2010/04/02 20:46:58 $     */
/* $Revision: 1.8 $                 */
/* $RCSfile: xor.c,v $              */
/* $State: Stab $:                */

#include "common_inc.h"
#include "bench.h"
#include "error_routines.h"

int bad_check;

void no_opt( void * );

/* begin tunable */

#define A_PAD                             96

/* end tunable   */

/* Array size in words */
#define A_SIZE                  (1L<<25)

static char cvs_info[] = "BMARKGRP $Date: 2010/04/02 20:46:58 $ $Revision: 1.8 $ $RCSfile: xor.c,v $ $Name:  $";


static uint64 a[A_SIZE+A_PAD], b[A_SIZE+A_PAD], c[A_SIZE+A_PAD];

static void ixor (int64 array_size);
static void init_abc (brand_t br, int64 array_size );
static void check (int64 array_size);

void xor (int64 op, brand_t br, int64 array_size)

{
  switch (op) {
  case 1:
    init_abc(br, array_size);
    break;
  case 2:
    ixor(array_size);
    break;
  case 3:
    check(array_size);
    break;
  }

}

static void ixor (int64 array_size)

{
  int64 i;

  // Prevent compiler from optimizing loop away
  if ( (b[0]&0xff) == 0) no_opt( b );

  for (i = 0; i < array_size; i++) {
    a[i] = b[i] ^ c[i];
  }
}

static void init_abc ( brand_t br, int64 array_size)

{
  int64 i;

  for (i = 0; i < array_size; i++) {
    a[i] = 0;
    b[i] = brand(&br);
    c[i] = i ^ b[i];
  }
}

static void check (int64 array_size )

{
  int64 i;
  int max_errs = 10;

  for (i = 0; i < array_size; i++) {
    if ( (max_errs-- > 0) && (a[i] != i) ){
      err_msg_ret("ERROR: a[%ld]=%ld\n", i, a[i]);
      bad_check = 1;
    }
  }

}


