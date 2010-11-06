/*--- Chris S.: June 10, 1999   */
/*---  reads in a generator type as an integer */
/*---adding 'int gentype' and read in + error handling */
/*--- generator numeral range is fix-set to 0-5 ?  */
/*--- should 5 be change to some variable MAX_GEN_NUMBER ? */
#include <stdio.h>
#include <stdlib.h>
#include "cputime.h"
#include "sprng_interface.h"

#define TIMING_TRIAL_SIZE 1000000
#define PARAM 0

main()
{

  int i;
  int *gen;
  double temp1, temp2, temp3, temp4;
  double temp_mult = TIMING_TRIAL_SIZE/1.0e6;
/*---   */
  int gentype;
  
  scanf("%d\n", &gentype);
  if((gentype < 0) || (gentype > 5))  /*--- range to be adjusted if needed */
  {
    printf("\nGenerator numeral out of range.\n\t...Exiting\n");    
    exit(1);
  }
/*---   */  


  gen = init_rng(gentype,0,1,0,PARAM);
/*--- Printing generator and stream information */
/*  print_rng(gen); */
  
  temp1 = cputime();

  for(i=0; i<TIMING_TRIAL_SIZE; i++)
    get_rn_int(gen);
  
  temp2 = cputime();
  

  for(i=0; i<TIMING_TRIAL_SIZE; i++)
    get_rn_flt(gen);
  
  temp3 = cputime();
  

  for(i=0; i<TIMING_TRIAL_SIZE; i++)
    get_rn_dbl(gen);
  
  temp4 = cputime();
  
  if(temp2-temp1 < 1.0e-15 || temp3-temp2 < 1.0e-15 ||  temp4-temp3 < 1.0e-15)
  {
    printf("Timing Information not available/not accurate enough.\n\t...Exiting\n");
    exit(1);
  }
  
  /* The next line is just used to ensure that the optimization does not remove the call to the RNG. Nothing is really printed.             */
  fprintf(stderr,"Last random number generated\n", get_rn_dbl(gen));

  printf("\nUser + System time Information (Note: MRS = Million Random Numbers Per Second)\n");
  printf("\tInteger generator:\t Time = %7.3f seconds => %8.4f MRS\n", 
	 temp2-temp1, temp_mult/(temp2-temp1));
  printf("\tFloat generator:\t Time = %7.3f seconds => %8.4f MRS\n", 
	 temp3-temp2, temp_mult/(temp3-temp2));
  printf("\tDouble generator:\t Time = %7.3f seconds => %8.4f MRS\n", 
	 temp4-temp3, temp_mult/(temp4-temp3));
  putchar('\n');
  
}

