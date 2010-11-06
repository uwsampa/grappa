#include <stdio.h>
#include <stdlib.h>
#include "cputime.h"

#define TIMING_TRIAL_SIZE 1000000

main()
{
  int i;
  double temp1, temp2, rn;
  double temp_mult = TIMING_TRIAL_SIZE/1.0e6;
  
  temp1 = cputime();

  for(i=0; i<TIMING_TRIAL_SIZE; i++)
    rn = drand48();
  
  temp2 = cputime();
  

  if(temp2-temp1 < 1.0e-15 )
  {
    printf("Timing Information not available/not accurate enough.\n\t...Exiting\n");
    exit(1);
  }
  
  /* The next line is just used to ensure that the optimization does not remove the call to the RNG. Nothing is really printed.             */
  fprintf(stderr,"Last random number generated\n", rn);
  
  printf("\nUser + System time Information (Note: MRS = Million Random Numbers Per Second)\n");
  printf("\tDRAND48:\t Time = %7.3f seconds => %8.4f MRS\n", 
	 temp2-temp1, temp_mult/(temp2-temp1));
  putchar('\n');
  
}

