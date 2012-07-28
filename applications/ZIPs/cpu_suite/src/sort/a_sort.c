/* CVS info                         */
/* $Date: 2010/04/02 20:43:18 $     */
/* $Revision: 1.6 $                 */
/* $RCSfile: a_sort.c,v $           */
/* $State: Stab $:                */

#include "bench.h"

/* Log base 2 of data array size */

#ifndef LG
#define LG                            25
#endif

#define MEMSZ                    (1<<(LG+1))

static char cvs_info[] = "BMARKGRP $Date: 2010/04/02 20:43:18 $ $Revision: 1.6 $ $RCSfile: a_sort.c,v $ $Name:  $";

static uint64 data[MEMSZ];

uint64 a_sort (int64 op, brand_t*  br, int64 data_size);
static uint64 init (brand_t* br, int64 data_size);
static uint64 a_sort_16 (brand_t* br, int64 data_size);
static uint64 check (brand_t* br, int64 data_size);
static uint64 print_some (brand_t* br, int64 ldata_size);
       void   PSORT(unsigned long *arr, int n);

uint64 a_sort (int64 op, brand_t*  br, int64 data_size)

{
  uint64 checksum;

  switch (op){
  case 1:
    checksum = init( br, data_size);
    break;
  case 2:
    checksum = a_sort_16( br, data_size);
    break;
  case 3:
    checksum = check( br, data_size);
    break;
  case 4:
    checksum = print_some( br, data_size);
    break;
  }
  return checksum;
}


static uint64 a_sort_16 (brand_t* br, int64 data_size)

{
  uint64 checksum = 0;

  PSORT(data, (int)data_size);

  return checksum;
}

static uint64 init (brand_t* br, int64 data_size)

{
  int64 i;
  uint64 checksum = 0;

  for(i=0; i<  data_size; i++){
    data[i] = (uint64) brand(br);
    checksum ^= data[i];
  }

  return checksum;
}

static uint64 check (brand_t* br, int64 data_size)

{
  int64 i;
  uint64 checksum = 0;

  for(i=0; i< data_size-1  ; i++){
    if(data[i] > data[i+1]){
#ifndef __BMK_SHORT_FORMAT
      printf("ERROR: data not sorted!\n");
      printf("data[%ld]=%#018lx\n", i, data[i]);
      printf("data[%ld]=%#018lx\n", i+1, data[i+1]);
      exit(EXIT_FAILURE);
#else
      set_status( 2 );  // Checked and incorrect
#endif
    }
    checksum ^= data[i];
  }
  checksum ^= data[data_size-1];

  return checksum;
}

static uint64 print_some (brand_t* br, int64 data_size)

{
  int64 i;
  uint64 checksum = 0;
#ifndef __BMK_SHORT_FORMAT
  printf("First 5 and last 5 of last sort\n");

  for(i=0; i< 5; i++)
      printf("data[%ld]\t\t = %#018lx\n", i, data[i]);
  printf("\n");
  for(i=data_size-5; i< data_size; i++)
      printf("data[%ld]\t = %#018lx\n", i, data[i]);
    printf("\n\n");
#endif

  return checksum;
}
