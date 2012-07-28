#include "hpc_output.h"
#include "utils.h"
#include "types.h"

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <sys/sysinfo.h>

extern char *pt_readm;
extern char *pt_wrtem;
extern char *pt_nsrt;
extern char *pt_rundir;
extern char *pt_logpath;
extern char *pt_idir;
extern char *pt_odir;

extern char mem_unit_size[];

extern uint64 mem_total;
extern uint64 mem_free;

extern uint32 num_c_cpus;
extern uint32 num_a_cpus;

extern uint32 iflag;
extern uint32 vflag;
extern uint32 hflag;
extern uint32 dfflag;
extern uint32 drflag;

extern uint64 t_fsize;

extern char *pt_ofpath;
extern char *pt_bsort;

extern int id;
extern int npes;

extern uint64 tiltiters;
extern uint64 bmmiters;
extern uint64 gmpiters;
extern uint64 popcntiters;
extern uint64 kiters;
extern uint64 kbufsize;
extern uint64 mmbs2iters;

extern uint64 rs;
extern uint64 rb;
extern uint64 nb;
extern uint64 nf;

extern uint64 bsize;


extern uint64 file_start;
extern uint64 file_stop;
extern uint64 bstripe_start;
extern uint64 bstripe_stop;

extern FILE *pt_log;

#define  NUMARGS  24
#define  GiBYTE   1073741824L


int cmdline_common( int na, char *args[] )
/********1*****|***2*********3*********4*********5*********6*********7**!

description:   This function parses the command line and validates
               options.

 invoked by:   routine            description
               --------------     --------------------------------------
               main             - int (mpbs.src)
                                  This is the main driving program for
                                  the MPBS system. 

    invokes:   routine            description
               ---------------    --------------------------------------
               help             - void (utils.src)
                                  Prints help information for command
                                  line parameters.  Exits program. 

  variables:   varible            description
               --------------     --------------------------------------
               na               - int
                                  Number of command line options passed
                                  in from main.

               args             - char **
                                  Array of command line options passed
                                  passed in from main.

               arg_count        - int
                                  Number of command line arguments for
                                  the function to process.

               parms            - parinfo *
                                  This structure holds command line
                                  flags with corresponding default
                                  values.
	      
               sfs              - int
                                  Integer to specify the sorted file size
				  in GiB.  Overrides all other options.

*********1*****|***2*********3*********4*********5*********6*********7**/
{
   /*
    * variable declarations
    */
   char buf1[256];
   char buf2[256];

   int arg_count;
   int found_odir = 0;
   int found_nsrt = 0;
   int len1       = 0;
   int len2       = 0;
   int sfs        = 0;
   
   float pm       = 0;

   /* factor is multiplied by memory usage per PE to account for 
    * the non-in place parallel sort. 
    */
   int factor     = 2;

   typedef struct {
     char flag[10];
     char def[10];
   } parinfo;

   int is_valid;

   /*
    * define pointer and make 10 records
    */
   parinfo *parms;

   /*
    * allocate memory for default parameter information
    */
   parms = (parinfo *)malloc( NUMARGS * sizeof( parinfo ) );

   /*
    * set default parameters
    */
   /* number of files */
   strcpy( parms[0].flag, "-nf" );
   strcpy( parms[0].def, "npes" );

   /* number of buckets */
   strcpy( parms[1].flag, "-nb" );
   strcpy( parms[1].def, "4" );

   /* records per bucket */
   strcpy( parms[2].flag, "-rb" );
   strcpy( parms[2].def, "64" );

   /* record size */
   strcpy( parms[3].flag, "-rs" );
   strcpy( parms[3].def, "8" );

   /* bucket size */
   strcpy( parms[4].flag, "-ss" );
   strcpy( parms[4].def, "qsort" );

   /* read method */
   strcpy( parms[5].flag, "-rm" );
   strcpy( parms[5].def, "files" );

   /* bucket file data directory */
   strcpy( parms[6].flag, "-id" );
   strcpy( parms[6].def, "." );

   /* output file directory */
   strcpy( parms[7].flag, "-od" );
   strcpy( parms[7].def, "./" );

   /* sorted output file name */
   strcpy( parms[8].flag, "-n" );
   strcpy( parms[8].def, "sorted" );

   /* system information */
   strcpy( parms[9].flag, "-i" );
   strcpy( parms[9].def, "" );

   /* program version */
   strcpy( parms[10].flag, "-v" );
   strcpy( parms[10].def, "" );

   /* program help */
   strcpy( parms[11].flag, "-h" );
   strcpy( parms[11].def, "" );

   /* specify sorted file size */
   strcpy( parms[12].flag, "-sfs" );
   strcpy( parms[12].def, "" );
   
   /* delete files on output */
   strcpy( parms[13].flag, "-df" );
   strcpy( parms[13].def, "");

   /* dry run, output header then exit */
   strcpy( parms[14].flag, "-dr" );
   strcpy( parms[14].def, "");

   /* set parameters based on filling PE memory */
   strcpy( parms[15].flag, "-pm" );
   strcpy( parms[15].def, "");

   /* do a netstress/popcount kernel during the workflow */
   strcpy( parms[16].flag, "-k" );
   strcpy( parms[16].def, "");

   /* Add a TILT kernel to the workflow */
   strcpy( parms[17].flag, "-tilt");
   strcpy( parms[17].def, "");

   /* Add a BMM kernel to the workflow */
   strcpy( parms[18].flag, "-bmm");
   strcpy( parms[18].def, "");

   /* Add a GMP kernel to the workflow */
   strcpy( parms[19].flag, "-gmp");
   strcpy( parms[19].def, "");

   /* Add a POPCNT kernel to the workflow */
   strcpy( parms[20].flag, "-popcnt");
   strcpy( parms[20].def, "");
   
   /* Power of 2 for buffer size for communication kernel. */
   strcpy( parms[21].flag, "-kbuf" );
   strcpy( parms[21].def, "");

   /* Add an MMBS2 kernel to the workflow */
   strcpy( parms[22].flag, "-mmbs2" );
   strcpy( parms[22].def, "");
 
   /* read method */
   strcpy( parms[23].flag, "-wm" );
   strcpy( parms[23].def, "ttb" );
   /*
    * start of command line processing
    */
   arg_count = na - 1;  /* adjust arg count for args[0] */
   while( arg_count > 0 )
   {
      if( !strcmp( args[ na - arg_count ], "-i" ) )
      {
         /*
          * decrement count to look at next
          */
         arg_count--;
         iflag = 1;
      }
      else if( !strcmp( args[ na - arg_count ], "-v" ) )
      {
         /*
          * decrement count to look at next
          */
         arg_count--;
         vflag = 1;
      }
      else if( !strcmp( args[ na - arg_count ], "-h" ) )
      {
         /*
          * decrement count to look at next
          */
         arg_count--;
         hflag = 1;
      }
      else if( !strcmp( args[ na - arg_count ], "-df" ) )
      {
	arg_count--;
	dfflag = 1;
      }
      else if( !strcmp( args[ na - arg_count ], "-dr") ) {
	arg_count--;
	drflag = 1;
      }
      else if( !strcmp( args[ na - arg_count ], "-nf" ) )
      {
         /*
          * must look at next array element
          */
         arg_count--;
         if( arg_count != 0 && is_integer(args[ na - arg_count] ) )
         {
            if( strcpy( parms[0].def, args[ na - arg_count ] ) )
            {
               /*
                * must decrement the arg number since successful
                */
               arg_count--;
            }
         }
         else
         {
	   printf_single( id, "ERROR: Number of files not given!\n" );
	   return 0;
         }
      }
      else if( !strcmp( args[ na - arg_count ], "-nb" ) )
      {
         /*
          * must look at next array element
          */
         arg_count--;
         if( arg_count != 0 && is_integer(args[ na - arg_count] ) )
         {
            if( strcpy( parms[1].def, args[ na - arg_count ] ) )
            {
               /*
                * must decrement the arg number since successful
                */
               arg_count--;
            }
         }
         else {
	   printf_single( id, "ERROR: Number of buckets not given!\n" );
           return 0;
         }
      }
      else if( !strcmp( args[ na - arg_count ], "-rb" ) )
      {
         /*
          * must look at next array element
          */
         arg_count--;
         if( arg_count != 0 && is_integer(args[ na - arg_count] )  )
         {
            if( strcpy( parms[2].def, args[ na - arg_count ] ) )
            {
               /*
                * must decrement the arg number since successful
                */
               arg_count--;
            }
         }
         else {
	   printf_single( id, "ERROR: Number of records per bucket not given!\n" );
	   return 0;
         }
      }
      else if( !strcmp( args[ na - arg_count ], "-rs" ) )
      {
         /*
          * must look at next array element
          */
         arg_count--;
         if( arg_count != 0 && is_integer(args[ na - arg_count] ))
         {
            if( strcpy( parms[3].def, args[ na - arg_count ] ) )
            {
               /*
                * must decrement the arg number since successful
                */
               arg_count--;
            }
         }
         else {
	   printf_single( id, "ERROR: Record size not given!\n" );
	   return 0;
         }
      }
      else if( !strcmp( args[ na - arg_count ], "-ss" ) )
      {
         /*
          * must look at next array element
          */
         arg_count--;

         /*
          * allocate array for sort method string 
          */
         if( arg_count == 0 || parms[4].def[0] == '-' ) {
	   printf_single( id, "ERROR: Serial sort method not set!\n" );
	   return 0;
         }
         /*
          * put bucket directory in array
          */
         strcpy( parms[4].def, args[ na - arg_count ] );

         /*
          * check sort method string against valid options
          */
         if( !strcmp( parms[4].def, "insert" ) ||
             !strcmp( parms[4].def, "select" ) ||
             !strcmp( parms[4].def, "bubble" ) ||
             !strcmp( parms[4].def, "radix"  ) ||
             !strcmp( parms[4].def, "qsort"  )    )
         {
           ;
         } 
	 else {
	   printf_single( id, "ERROR: Serial sort method, %s, is not valid!\n", 
			  parms[4].def);
	   return 0;
         }

         /*
          * must decrement to leave while loop
          */
         arg_count--;
      }
      else if( !strcmp( args[ na - arg_count ], "-rm" ) )
      {
         /*
          * must look at next array element
          */
         arg_count--;

	 /* Check to see if the argument is missing or if the string given as 
	  * the argument starts with a '-', which indicates a malformed command
	  * line.
          */
	 if( arg_count == 0 || parms[5].def[0] == '-' ) {
	   printf_single( id, "ERROR: Read file method not set!\n" );
	   return 0;
         }

         /*
          * put read method arg in string 
          */
         strcpy( parms[5].def, args[ na - arg_count ] );

	 /*
          * check sort method string against valid options
          */
         if( !strcmp( parms[5].def, "stripe" ) ||
             !strcmp( parms[5].def, "files"  )   )
         {
           ;
         }
         else {
           printf_single(id, "ERROR: Read file method %s is not valid!\n",
			 parms[5].def );
	   return 0;
         }

         /*
          * must decrement to leave while loop
          */
         arg_count--;
      }
      else if( !strcmp( args[ na - arg_count ], "-id" ) )
      {
         /*
          * must look at next array element
          */
         arg_count--;

         /*
          * allocate array for directory path
          */
         if( arg_count != 0 )
         {
	   pt_idir = (char *)malloc( (strlen(args[ na - arg_count ])+1) *
				     sizeof( char ) );
	   if( !pt_idir )
	     perror( "malloc" );
         }
         else {
	   printf_single( id, "ERROR: Data input directory path not given!\n" );
	   return 0;
         }

         /*
          * put bucket directory in array
          */
         strcpy( pt_idir, args[ na - arg_count ] );

         /*
          * check to see if string given for directory starts with
          * a '-' which means that it is possibly an option
          */
         if( pt_idir[0] == '-' )
         {
	   printf_single( id, "ERROR: Data input directory begins with a '-'.  "
			  "Missing argument?\n" );
	   return 0;
         }

         /*
          * must decrement to leave while loop
          */
         arg_count--;
      }
      else if( !strcmp( args[ na - arg_count ], "-od" ) )
      {
         /*
          * must look at next array element
          */
         arg_count--;

         /*
          * allocate array for directory path
          */
         if( arg_count == 0 )
         {
	   printf_single( id, "ERROR: Output directory path not given!\n" );
	   return 0;
         }

         /*
          * put bucket directory in array
          */
         strcpy( buf1, args[ na - arg_count ] );

         /*
          * check to see if string given for directory starts with
          * a '-' which means that it is possibly an option
          */
         if( buf1[0] == '-' )
         {
	   printf_single( id, "ERROR: Output directory begins with a '-'.  "
			  "Missing argument?\n" );
	   return 0;
         }

         /* set found directory path flag */
         found_odir = 1;

         /*
          * get length of output directory path
          */
         len1 = strlen( buf1 );

	 /*
          * if '/' not at the end of the string
          * then one must be added
          */
         if( buf1[len1-1] != '/' )
         {
            pt_odir = (char *)malloc( (len1+2)*sizeof(char) );
            strcpy( pt_odir, buf1 );
            pt_odir[len1]   = '/';
            pt_odir[len1+1] = '\0';
         }
         else
         {
            pt_odir = (char *)malloc( (len1+1)*sizeof(char) );
            strcpy( pt_odir, buf1 );
            pt_odir[len1] = '\0';
         }

         /*
          * must decrement to leave while loop
          */
         arg_count--;
      }
      else if( !strcmp( args[ na - arg_count ], "-n" ) )
      {
         /*
          * must look at next array element
          */
         arg_count--;

         /*
          * allocate array for read method string
          */
	 if( arg_count == 0) {
	   printf_single( id, "ERROR: Output file name not set!\n" );
	   return 0;
         }

         /*
          * put read method arg in string
          */
         strcpy( buf2, args[ na - arg_count ] );

         /*
          * check to see if string given for directory starts with
          * a '-' which means that it is possibly an option
          */
        if( buf2[0] == '-' ) {
	  printf_single( id, "ERROR: Output file name begins with a '-'.  "
			 "Missing argument?\n" );
	  return 0;
	}
	
	/* set found user given name of output file */
	found_nsrt = 1;

	/*
	 * get length of given output file name
	 */
	len2 = strlen( buf2 );
	
	/*
	 * copy output file name final string location
	 */
	pt_nsrt = (char *)malloc( (len2+1)*sizeof(char) );
	strcpy( pt_nsrt, buf2 );
	pt_nsrt[len2] = '\0';
	
	/*
	 * must decrement to leave while loop
	 */
	arg_count--;
      }
      else if( !strcmp( args[ na - arg_count ], "-sfs" ) )
      {
         /*
          * must look at next array element
          */
         arg_count--;
         if( arg_count != 0  && is_integer(args[ na - arg_count] ) )
         {
            if( strcpy( parms[12].def, args[ na - arg_count ] ) )
            {
               /*
                * must decrement the arg number since successful
                */
               arg_count--;
            }
         }
         else {
	   printf_single( id, "ERROR: Sorted file size not given!\n" );
	   return 0;
         }
      }     
      else if( !strcmp( args[ na - arg_count ], "-pm" ) )
      {
         /*
          * must look at next array element
          */
         arg_count--;
         if( arg_count != 0  && is_integer(args[ na - arg_count] ) )
         {
            if( strcpy( parms[15].def, args[ na - arg_count ] ) )
            {
               /*
                * must decrement the arg number since successful
                */
               arg_count--;
            }
         }
         else
         {
	   printf_single( id, "ERROR: Node memory size not given!\n" );
	   return 0;
         }
      }
      else if( !strcmp( args[ na - arg_count ], "-tilt" ) )
      {
         /*
          * must look at next array element
          */
         arg_count--;
         if( arg_count != 0  && is_integer(args[ na - arg_count] ) )
         {
            if( strcpy( parms[17].def, args[ na - arg_count ] ) )
            {
               /*
                * must decrement the arg number since successful
                */
               arg_count--;
            }
         }
         else
         {
	   printf_single( id, "ERROR: Number of TILT iterations not given!\n" );
	   return 0;
         }
      }
      else if( !strcmp( args[ na - arg_count ], "-bmm" ) )
      {
         /*
          * must look at next array element
          */
         arg_count--;
         if( arg_count != 0  && is_integer(args[ na - arg_count] )  )
         {
            if( strcpy( parms[18].def, args[ na - arg_count ] ) )
            {
               /*
                * must decrement the arg number since successful
                */
               arg_count--;
            }
         }
         else
         {
	   printf_single( id, "ERROR: Number of BMM iterations not given!\n" );
	   return 0;
         }
      }
      else if( !strcmp( args[ na - arg_count ], "-gmp" ) )
      {
         /*
          * must look at next array element
          */
         arg_count--;
         if( arg_count != 0  && is_integer(args[ na - arg_count] ) )
         {
            if( strcpy( parms[19].def, args[ na - arg_count ] ) )
            {
               /*
                * must decrement the arg number since successful
                */
               arg_count--;
            }
         }
         else
         {
	   printf_single( id, "ERROR: Number of GMP iterations not given!\n" );
	   return 0;
         }
      }
      else if( !strcmp( args[ na - arg_count ], "-popcnt" ) )
      {
         /*
          * must look at next array element
          */
         arg_count--;
         if( arg_count != 0  && is_integer(args[ na - arg_count] )  )
         {
            if( strcpy( parms[20].def, args[ na - arg_count ] ) )
            {
               /*
                * must decrement the arg number since successful
                */
               arg_count--;
            }
         }
         else
         {
	   printf_single( id, "ERROR: Number of POPCNT iterations not given!\n" );
	   return 0;
         }
      }
      else if( !strcmp( args[ na - arg_count ], "-k" ) )
      {
         /*
          * must look at next array element
          */
         arg_count--;
         if( arg_count != 0  && is_integer(args[ na - arg_count] )  )
         {
            if( strcpy( parms[16].def, args[ na - arg_count ] ) )
            {
               /*
                * must decrement the arg number since successful
                */
               arg_count--;
            }
         }
         else
         {
	   printf_single( id, "ERROR: Number of kernel iterations not given!\n" );
	   return 0;
         }
      }
      else if( !strcmp( args[ na - arg_count ], "-kbuf" ) )
      {
         /*
          * must look at next array element
          */
         arg_count--;
         if( arg_count != 0  && is_integer(args[ na - arg_count] ) )
         {
            if( strcpy( parms[21].def, args[ na - arg_count ] ) )
            {
               /*
                * must decrement the arg number since successful
                */
               arg_count--;
            }
         }
         else
         {
	   printf_single( id, "ERROR: -kbuf requires an argument!\n" );
	   return 0;
         }
      }
     else if( !strcmp( args[ na - arg_count ], "-mmbs2" ) )
     {
       /*
	* must look at next array element
	*/
       arg_count--;
       if( arg_count != 0  && is_integer(args[ na - arg_count] ) ) {
	 if( strcpy( parms[22].def, args[ na - arg_count ] ) ) {
	   /*
	    * must decrement the arg number since successful
	    */
	   arg_count--;
	 }
       }
       else {
	 printf_single( id, "ERROR: Number of MMBS2 iterations not given!\n" );
	 return 0;
       }
     }
      else if( !strcmp( args[ na - arg_count ], "-rm" ) )
      {
         /*
          * must look at next array element
          */
         arg_count--;

         /*
          * allocate array for read method string
          */
         if( arg_count == 0 ) {
	   printf_single( id, "ERROR: Read file method not set!\n" );
	   return 0;
         }

         /*
          * put read method arg in string 
          */
         strcpy( parms[5].def, args[ na - arg_count ] );

         /*
          * check to see if string given for directory starts with
          * a '-' which means that it is possibly an option
          */
         if( parms[5].def[0] == '-' )
         {
	   printf_single( id, "ERROR: Read file method not set!\n");
           return 0;
         }

         /*
          * check sort method string against valid options
          */
         if( !strcmp( parms[5].def, "stripe" ) ||
             !strcmp( parms[5].def, "files"  )   )
         {
           ;
         }
         else {
	   printf_single( id, "ERROR: Read file method %s is not valid!\n",
			  parms[5].def );
	   return 0;
         }

         /*
          * must decrement to leave while loop
          */
         arg_count--;
      }
      else if( !strcmp( args[ na - arg_count ], "-wm" ) )
      {
         /*
          * must look at next array element
          */
         arg_count--;

         /*
          * allocate array for read method string
          */
         if( arg_count == 0 || parms[23].def[0] == '-' ) {
	   printf_single( id, "ERROR: Write file method not set!\n" );
	   return 0;
         }

         /*
          * put read method arg in string
          */
         strcpy( parms[23].def, args[ na - arg_count ] );

         /*
          * check write method string against valid options
          */
         if( !strcmp( parms[23].def, "ttb"    ) ||
             !strcmp( parms[23].def, "rand"   ) ||
             !strcmp( parms[23].def, "stripe" )    )
         {
           ;
         }
         else {
	   printf_single( id, "ERROR: Write file method %s is not valid!\n",
			  parms[23].def );
	   return 0;
         }

         /*
          * must decrement to leave while loop
          */
         arg_count--;
      }
      else {
	// If I don't recognize it, then skip it.  Better way to handle this is to create a list
	// of the arguments that aren't recognized by the system, then pass them back to be handled
	// by the specific command line handler.
	arg_count--;
      }
   }  /* while loop */

   /* if any of the optional flags are set, then
    * return to main
    */
   if( iflag || vflag || hflag )
      return 1;

   /* validate parms */
   /* check nf */
   if( !strcmp( parms[0].def, "npes" ) )
      nf = npes;
   else
   {
     nf = get_num( parms[0].def, NULL  );
   }

   /* check nb */
   nb = get_num( parms[1].def, NULL );
   if( nb == 1   || nb == 2   || nb == 4    || nb == 8    ||
       nb == 16  || nb == 32  || nb == 64   || nb == 128  ||
       nb == 256 || nb == 512 || nb == 1024 || nb == 2048   )
   {
      ;
   } 
   else {
     printf_single( id, "ERROR: Bucket size not valid.\n");
     return 0;
   }

   /* check rb */
   rb = get_num(parms[2].def, NULL);
   
   /* check rs */
   rs = get_num( parms[3].def, NULL );
   if( rs != 8 && rs != 16 ) {
     printf_single( id, "ERROR: Record size must be either 8 or 16\n" );
     return 0;
   }
   
   if( (sfs = atoi( parms[12].def )) > 0) {
     // For -sfs we want 2 * npes as default (guidance from Jim) 
     if( nf == npes )
       nf = 2*npes;

     rb = (sfs * GiBYTE) / (nf * rs * nb);
   }
   
   if( sfs == 0 && (pm = atof( parms[15].def )) > 0) {
     if( nf == npes )
       nf = 2*npes;
     
     rb = (pm * npes * GiBYTE) / (nf * rs);
     
     // Account for the out of place parallel sort.
     rb /= factor;
   }
   
   /* Set number of tilt iterations. */
   tiltiters = get_num( parms[17].def, NULL);
    
   /* Set number of bmm iterations. */
   bmmiters = get_num( parms[18].def, NULL);
      
   /* Set number of GMP iterations. */
   gmpiters = get_num( parms[19].def, NULL);
  
   /* Set number of POPCNT iterations. */
   popcntiters = get_num( parms[20].def, NULL);
   
   /* Set number of collective communication kernel iterations. */
   kiters = get_num( parms[16].def, NULL);
      
   /* Set number of collective communication kernel iterations. */
   kbufsize = get_num( parms[21].def, NULL);

   /*
    * If only the number of iterations is specified use a default size of 1MiB.
    * Conversely, if only a buffer size is specified, then do a single iteration.
    */
   if(kbufsize > 0) {
     kbufsize = 1L << kbufsize;
   }

   if(kiters && !kbufsize) {
     kbufsize = 1L << 20;
   }
   else if(!kiters && kbufsize) {
     kiters = 1;
   }

   /* Set number of MMBS2 iterations. */
   mmbs2iters = get_num( parms[22].def, NULL);
   
#ifdef _CRAYC
   if(mmbs2iters > 0) {
     printf_single( id, "MMBS2 kernel is not supported when mpbs is compiled with craycc.\n");
     return 0;
   }
   
#endif
   
   /* check bucket sort method */
   if( !pt_bsort ) {
     pt_bsort=(char *)malloc( strlen( parms[4].def ) * sizeof( char ) );
     if( !pt_bsort )
       perror( "malloc" );
     strcpy( pt_bsort, parms[4].def );
   }
   
   /* check read data method */
   if( !pt_readm ) {
     pt_readm=(char *)malloc( strlen( parms[5].def ) * sizeof( char ) );
     if( !pt_readm )
       perror( "malloc" );
     strcpy( pt_readm, parms[5].def );
   }
   
   /* check write data method */
   if( !pt_wrtem ) {
     pt_wrtem=(char *)malloc( strlen( parms[23].def ) * sizeof( char ) );
     if( !pt_wrtem )
       perror( "malloc" );
     strcpy( pt_wrtem, parms[23].def );
   }
   
   /*
    * check input directory
    */
   if( !pt_idir ) {
     pt_idir=(char *)malloc( strlen( parms[6].def ) * sizeof( char ) );
     if( !pt_idir )
       perror( "malloc" );
     strcpy( pt_idir, parms[6].def );
   }
   
   /*
    * check output directory
    */
   if( !found_odir ) {
     pt_odir=(char *)malloc( strlen( parms[7].def ) * sizeof( char ) );
     if( !pt_odir )
       perror( "malloc" );
     strcpy( pt_odir, parms[7].def );
   }
   
   /*
    * check file name
    */
   if( !found_nsrt ) {
     pt_nsrt=(char *)malloc( strlen( parms[8].def ) * sizeof( char ) );
     if( !pt_nsrt )
       perror( "malloc" );
     strcpy( pt_nsrt, parms[8].def );
   }
   
   /*
    * build file name path
    */
   pt_ofpath = newstr( pt_odir, pt_nsrt ); 
   
   return 1;
}
 
void header_common( uint32 np )
/********1*****|***2*********3*********4*********5*********6*********7**!

description:   This function prints the output header.

 invoked by:   routine            description
               --------------     --------------------------------------
               main             - int (mpbs.src)
                                  This is the main driving program for
                                  the MPBS system.

    invokes:   routine            description
               ---------------    --------------------------------------
               len_i            - int (utils.src)
                                  Calculates the length of an integer.

               get_netinfo      - net_info (utils.src)
                                  This function will return host network
                                  information.

  variables:   varible            description
               --------------     --------------------------------------
               np               - uint32
                                  Number of processes.

               start            - times
                                  Starting time structure.

               i                - uint32
                                  General loop variable.

               l_hostname       - uint32
                                  Length of hostname.

               l_device         - uint32
                                  Length of ethX device name.

               l_ip             - uint32
                                  Length of ip address.

               l_user_name      - uint32
                                  Length of user sid.

               l_cwd            - uint32
                                  Length of current working directory. 

               l_idir           - uint32
                                  Length of input directory.

               l_odir           - uint32
                                  Length of output directory.

               l_ctme           - uint32
                                  Length of the time and date string.

               l_osname         - uint32
                                  Length of os name string.

               l_release        - uint32
                                  Length of kernel version string.

               up_time          - uint32
                                  Seconds since boot.

               secs             - uint32
                                  Uptime clock seconds 0-59s. 

               mins             - unit32
                                  Uptime clock minutes 0-59m.

               hours            - uint32
                                  Uptime hours 0-23h.

               days             - uint32
                                  Uptime days.

               almem            - uint32
                                  Allocated memory in bytes.

               lalmem           - uint32
                                  Length of almem number.  Number
                                  of characters.

               hostname[]       - char
                                  Allocated to 36 characters.  Holds
                                  hostname.

               ctme[]           - char
                                  Holds date and time.

               os_name[]        - char
                                  Holds os name.

               release[]        - char
                                  Holds kernel version. 

               user_name        - char *
                                  Holds current user sid.

               msize[]          - char
                                  Memory unit size. 

               sys_info         - struct utsname
                                  Holds system name and release 
                                  information.

               info             - struct sysinfo *
                                  See man sysinfo.

               my_net           - net_info
                                  Type that include network device name
                                  pair with the device's ip address.

*********1*****|***2*********3*********4*********5*********6*********7**/
{
   /*
    * variable declaration section
    */
   uint32 i = 0;

   uint32 l_hostname;
   uint32 l_device;
   uint32 l_ip;
   uint32 l_user_name;
   uint32 l_rundir; 
   uint32 l_idir;
   uint32 l_odir;
   uint32 l_ctme;
   uint32 l_osname;
   uint32 l_release;

   uint32 up_time;
   uint32 secs;
   uint32 mins;
   uint32 hours;
   uint32 days;

   uint32 almem  = 0;
   uint32 lalmem = 0;

   char hostname[36];
   char ctme[36];
   char os_name[256];
   char release[256];
   char *user_name     = NULL;
   char msize[4];

   float f_per_p = (float)nf/np;

   struct utsname sys_info;
   struct sysinfo info;
   net_info my_net; 

   /*
    * get osname
    */
   uname(&sys_info);
   strcpy(os_name, sys_info.sysname);
   strcpy(release, sys_info.release);

   /*
    * get system uptime 
    */
   if( sysinfo(&info) < 0 )
       up_time = 0;

   /*
    * break down uptime to days, hours, mins and secs
    */
   up_time = info.uptime;
   secs  = up_time % 60;
   mins  = (up_time / 60) % 60;
   hours = (up_time / 3600) % 24;
   days  = (up_time / 86400);

   /*
    * get the username
    */
   user_name = getenv( "USER" );

   /*
    * get the hostname
    * check for error from gethostname function
    * if there is a problem then localhost is used
    */
   if( gethostname( hostname, sizeof( hostname ) ) == 1 )
   {
      fprintf( stderr,"%s: gethostname(2)\n", strerror(errno) );
      strcpy( hostname, "localhost" );
   }

   /*
    * loop over net device - ip address pairs until a device that
    * starts with 'e' is found.  Assumed to be an ethX.
    */
   do
   {
      my_net = get_netinfo( i );

      if( my_net.if_name[ 0 ] == 'e' )
         break;

      i++;
   } while( i < 4 );

   time_t now = time(NULL);
   strcpy( ctme, ctime( &now ) );

   /*
    * get the string lengths
    */
   l_hostname  = strlen( hostname       );
   l_device    = strlen( my_net.if_name );
   l_ip        = strlen( my_net.ip      );
   l_user_name = strlen( user_name      );
   l_rundir    = strlen( pt_rundir      );
   l_idir      = strlen( pt_idir        );
   l_odir      = strlen( pt_odir        );
   l_osname    = strlen( os_name        );
   l_release   = strlen( release        );
   l_ctme      = (uint32)strlen( ctme      );

   ctme[ l_ctme - 1 ] = '\0'; 

   /*
    * write to stdout and log file
    */
   printf( "***************************************************************************\n");
   fprintf(pt_log,"***************************************************************************\n");

   printf( "* Entering the MPBS system v"                              );
   fprintf(pt_log,"* Entering the MPBS system v"                       );

   printf( PACKAGE_VERSION                                             );
   fprintf(pt_log, PACKAGE_VERSION                                     );

   printf( ": %s\n", ctme                                                );
   fprintf(pt_log, ": %s\n", ctme                                        );

#ifdef _USE_O_DIRECT
   printf( "*\n");
   fprintf(pt_log,"*\n");

   printf( "* USING DIRECT IO with page size: %4d\n", 
	   PAGE_SIZE);
   fprintf( pt_log, "* USING DIRECT IO with page size: %4d\n",
	    PAGE_SIZE);
#endif

   printf( "*\n");
   fprintf(pt_log,"*\n");

   printf( "*           system: %s\n", hostname);
   fprintf(pt_log, "*           system: %s\n", hostname);

   printf( "*        user name: %s\n", user_name);
   fprintf(pt_log, "*        user name: %s\n", user_name);

   printf( "*\n");
   fprintf(pt_log,"*\n");

   printf( "*          os name: %s\n", os_name);
   fprintf(pt_log,"*          os name: %s\n", os_name);

   printf( "*          release: %s\n", release                            );
   fprintf(pt_log, "*          release: %s\n", release                    );

   printf( "*    system uptime: %u=%u-%2.2u:%2.2u:%2.2u\n", up_time, days,
                                                      hours, mins, secs);
   fprintf(pt_log,"*    system uptime: %u=%u-%2.2u:%2.2u:%2.2u\n",up_time, days,
                                                        hours, mins, secs);

   printf( "*\n");
   fprintf(pt_log,"*\n");

   printf( "*    num conf cpus: %d\n", num_c_cpus                          );
   fprintf(pt_log, "*    num conf cpus: %d\n", num_c_cpus                  );

   printf( "*   num avail cpus: %d\n", num_a_cpus                          );
   fprintf(pt_log, "*   num avail cpus: %d\n", num_a_cpus                  );

   printf( "*     total memory: %u %s\n", mem_total, mem_unit_size         );
   fprintf(pt_log, "*     total memory: %u %s\n", mem_total, mem_unit_size );

   printf( "*      free memory: %u %s\n", mem_free, mem_unit_size          );
   fprintf(pt_log, "*      free memory: %u %s\n", mem_free, mem_unit_size  );
   
   printf( "*\n" );
   fprintf(pt_log,"*\n" );
   
   printf( "*        processes: %d\n", np                                  );
   fprintf(pt_log, "*        processes: %d\n", np                          );

   printf( "*            files: %lu\n", nf                          );
   fprintf(pt_log,"*            files: %lu\n", nf                   );

   printf( "*       files/proc: %0.1f\n", f_per_p                  );
   fprintf(pt_log,"*       files/proc: %0.1f\n", f_per_p           );

   printf( "*    file creation: %s\n", pt_wrtem);
   fprintf( pt_log,"*    file creation: %s\n", pt_wrtem);
  
   lprintf_single( id, pt_log, "*      read method: %s\n", pt_readm);

   printf( "*\n" );
   fprintf(pt_log,"*\n" );

   printf( "*      record size: %lu bytes\n", rs                    );
   fprintf(pt_log,"*      record size: %lu bytes\n", rs             );

   printf( "*  recs per bucket: %lu\n", rb                    );
   fprintf(pt_log,"*  recs per bucket: %lu\n", rb             );

   char *units = bytes_to_human(bsize);
   printf( "*      bucket size: %s\n", units                 );
   fprintf(pt_log,"*      bucket size: %s\n", units          );
   free(units);

   units = bytes_to_human(bsize*nf);
   printf( "*      stripe size: %s\n", units              );
   fprintf(pt_log,"*      stripe size: %s\n", units       );
   free(units);

   int factor = 2;

   units = bytes_to_human(((double)bsize*nf*factor)/npes);
   printf( "*    memory per PE: %s\n", units              );
   fprintf(pt_log,"*    memory per PE: %s\n", units       );
   free(units);

   printf( "* buckets per file: %lu\n", nb                    );
   fprintf(pt_log,"* buckets per file: %lu\n", nb             );

   units = bytes_to_human(((double)t_fsize)/nf);
   printf( "*        file size: %s\n", units);
   fprintf(pt_log, "*        file size: %s\n", units);
   free(units);

   units = bytes_to_human((double)t_fsize);
   printf( "*  total file size: %s\n", units           );
   fprintf(pt_log,"*  total file size: %s\n", units       );
   free(units);

   printf( "*\n");
   fprintf(pt_log,"*\n");

   printf( "*    execution dir: %s\n", pt_rundir                           );
   fprintf(pt_log,"*    execution dir: %s\n", pt_rundir                    );

   printf( "*   input data dir: %s/\n", pt_idir                             );
   fprintf(pt_log,"*   input data dir: %s/\n", pt_idir                      );

   printf( "*  output data dir: %s\n", pt_odir                             );
   fprintf(pt_log, "*  output data dir: %s\n", pt_odir                     );
   
   printf( "*\n");
   fprintf(pt_log,"*\n");
   
   if(tiltiters) {
     lprintf_single(id, pt_log, "* TILT kernel iterations: %lu\n", tiltiters);
   }
   
   if(bmmiters) {
     lprintf_single(id, pt_log, "* BMM kernel iterations: %lu\n", bmmiters);
   }

   if(gmpiters) {
     lprintf_single(id, pt_log, "* GMP kernel iterations: %lu\n", gmpiters);
   }
  
   if(popcntiters) {
     lprintf_single(id, pt_log, "* POPCOUNT kernel iterations: %lu\n", 
		    popcntiters);
   }
   
   if(mmbs2iters) {
     lprintf_single(id, pt_log, "* MMBS2 kernel iterations: %lu\n", 
		    mmbs2iters);
   }

   if(kiters) {
     lprintf_single(id, pt_log, "* Collective communication kernel iterations: %lu\n", kiters);
     units = bytes_to_human(kbufsize);
     lprintf_single(id, pt_log, "* Collective communication kernel max buffer size: %s\n", units);
     free(units);
   }

   printf( "***********************************************************");
   fprintf(pt_log,"***********************************************************");

   printf( "****************\n");
   fprintf(pt_log,"****************\n");

   return;
}

/**
 * Prints the output summary at conclusion of the program. 
 */
void trailer_common( timer *t ) {
  char *ewall;
  char *ecpu;
  
  uint32 len_ctme;
  
  char current_time[36];                
  
  ewall = dhms( t->accum_wall );  // Elapsed wall clock time.
  ecpu  = dhms( t->accum_cpus );  // Elapsed CPU + sys time.
  
  time_t now = time(NULL);
  strcpy( current_time, ctime( &now ) );
  
  len_ctme = (uint32)strlen( current_time );
  
  current_time[ len_ctme - 1 ] = '\0';
  
  /*
   * output timing summary; write to stdout and log file
   */
  lprintf_single( id, pt_log, "***********************************************"
		  "****************************\n" );
  lprintf_single( id, pt_log,"* MPBS timing summary:\n" );
  lprintf_single( id, pt_log,"*       CPU time elapsed: %s\n", ecpu );
  lprintf_single( id, pt_log,"*      Wall time elapsed: %s\n", ewall );
  lprintf_single( id, pt_log,"*\n" );
  lprintf_single( id, pt_log,"*Leaving the MPBS system: v%s\n", PACKAGE_VERSION );
  lprintf_single( id, pt_log,"*           Current time: %s\n", current_time );
  lprintf_single( id, pt_log, "***********************************************"
		  "****************************\n" );
  return;
}



void print_sys_stats( )
/********1*****|***2*********3*********4*********5*********6*********7**!
description:   Gets system statistics by opening the /proc/cpuinfo if
               on a Linux based machine.  The statistics to be
               gathered are:
  
               - get the number of sockets
               - get the number of cores per socket
               - get the number of threads per core

               number of logical cpus = Sockets * CpS * TpC

 invoked by:   routine            description
               --------------     --------------------------------------
               main             - int (mpbs.src)
                                  This is the main driving program for
                                  the MPBS system. 

    invokes:   routine            description
               ---------------    --------------------------------------
               get_num_sockets  - uint32 (utils.src)
                                  Counts the number of processor
                                  sockets on the node.

               get_num_cores_
               per_socket       - uint32 (utils.src)
                                  Determines the number of cores per
                                  socket.

               get_num_
               logical_cpus     - uint32 (utils.src)
                                  Counts the number of processors which
                                  are seen by the Linux kernel as
                                  logical processors.  This is a thread
                                  per core count.

  variables:   varible            description
               --------------     --------------------------------------
               cpu_info_file    - const char *
                                  Path to cpu information file on Linux
                                  systems.

               num_sockets      - uint32
                                  Number of processor sockets on node.

               num_cores_
               socket           - uint32
                                  Number of processor cores on each
                                  cpu chip/socket.

               num_logical_
               cpus             - uint32
                                  Number of logical processors which is
                                  number_sockets * number_cores_socket
                                  * number_threads_per_core.

               num_threads
               _per_core        - uint32
                                  Number of threads per core which is the
                                  number of logical cpus per core.

*********1*****|***2*********3*********4*********5*********6*********7**/
{
   /*
    * variable declaration section
    */
   const char *cpu_info_file = "/proc/cpuinfo";

   uint32 num_sockets          = 0;
   uint32 num_cores_socket     = 0;
   uint32 num_logical_cpus     = 0;
   uint32 num_threads_per_core = 0;

   uint64 mem_free;
   uint64 mem_total;
   
   get_mem_info(&mem_free, &mem_total);

   /*
    * get the number of sockets
    */ 
   num_sockets = get_num_sockets( cpu_info_file ); 

   /*
    * get the number of cores per socket
    */
   num_cores_socket = get_num_cores_per_socket( cpu_info_file ); 

   /*
    * get the number of logical processors
    */
   num_logical_cpus = get_num_logical_cpus( cpu_info_file );

   /*
    * calculate the threads per core
    */
   num_threads_per_core = num_logical_cpus /\
                             ( num_sockets * num_cores_socket );

   printf( "sockets: %d\n", num_sockets );
   printf( "cores per socket: %d\n", num_cores_socket );
   printf( "threads per core: %d\n", num_threads_per_core );
   printf( "logical cpus: %d\n", num_logical_cpus );

   printf( "\n" );
   printf( "total memory: %d kB\n", mem_total );
   printf( "free memory:  %d kB\n", mem_free );

   return;
}


void help_common( void )
/********1*****|***2*********3*********4*********5*********6*********7**!

description:   Prints help information for command line parameters then
               exits program.

 invoked by:   routine            description
               --------------     --------------------------------------
               cmdline          - void (utils.src)
                                  Parses the command line and validates
                                  options.

*********1*****|***2*********3*********4*********5*********6*********7********/
{
   printf( "Command line syntax to execute MPBS system is:\n"                );
   printf( "\n"                                                              );
   printf( "(mpirun -np|aprun -n) <num_procs> ./mpbs-<shmem|mpi<1|2>>\n"     );
   printf( "                                       [-i] [-v] [-h] [-dr] \n"  );
   printf( "                                       [-nf num_files]\n"        );
   printf( "                                       [-nb num_buckets]\n"      );
   printf( "                                       [-rb rec_per_bucket]\n"   );
   printf( "                                       [-rs rec_size]\n"         );
   printf( "                                       [-rm read_method]\n"      );
   printf( "                                       [-ss serial_sort]\n"      );
   printf( "                                       [-id input_dir]\n"        );
   printf( "                                       [-od output_dir]\n"       );
   printf( "                                       [-df delete_files]\n"     );
   printf( "                                       [-sfs sorted_file_size]\n");
   printf( "                                       [-pm memory_per_pe]\n"    );
   printf( "                                       [-tilt niter]\n"          );
   printf( "                                       [-n  out_file_name]\n"
	   "                                       [-bmm niter]\n"
	   "                                       [-gmp niter]\n"
	   "                                       [-k niter]\n"
	   "                                       [-kbuf n]\n"
	   "                                       [-popcnt niter]\n"
	   "                                       [-mmbs2 niter]\n"         );
   printf( "\n"                                                              );
   printf( "   where <num_procs> is the number of processes.\n"              );
   printf( "\n"                                                              );
   printf( "   Note: Parameters for nf, nb, rb and rs can be entered as a\n" );
   printf( "         number raised to a power, e.g. 2^10, or as a\n"         );
   printf( "         conventional integer.\n"                                );
   printf( "\n"                                                              );
   printf( "PARAMETERS:\n"                                                   );
   printf( " -nf     Number of files to produce.\n"                          );
   printf( "         Defaults to number of processes.\n"                     ); 
   printf( "\n"                                                              );
   printf( " -nb     Number of buckets per file.\n"                          );
   printf( "         The number of buckets have to be one of the"            );
   printf( " following values:\n"                                            );
   printf( "         1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048\n"    );
   printf( "         Default is 4.\n"                                        );
   printf( "\n"                                                              );
   printf( " -rb     Number of records per bucket.\n"                        );
   printf( "         Default is 64.\n"                                       );
   printf( "\n"                                                              );
   printf( " -rs     Record size in bytes( 8 | 16 ).  Currently,\n"          );
   printf( "         16 byte integers(records) not implemented.\n"           );
   printf( "         Default is 8.\n"                                        );
   printf( "\n"                                                              );
   printf( " -rm     Method by which the file data, by bucket, will\n"       );
   printf( "         be read into memory for parallel sorting.\n"            );
   printf( "         There are two methods:\n"                               );
   printf( "         stripe - each process reads in a stripe\n"              );
   printf( "                           of records across all files.\n"       );
   printf( "         files(default)  - each process reads in data from a "   );
   printf( "                  predefined set of\n"                           );
   printf( "                  file in the same way files are produced.\n"    );
   printf( "\n"                                                              );
   printf( " -wm     Method by which the files will be created.\n"           );
   printf( "         ttb (default)    - top-to-bottom(ttb) method fills buckets \n" 
	   "                  sequentially.  Each PE has \n"
	   "                  exclusive access to write a group of files."   );
   printf( "         rand   - random(rand) method fills buckets\n"           );
   printf( "                  randomly.\n"                                   );
   printf( "         stripe - each process writes a stripe in\n"             );
   printf( "                  a given bucket.\n\n"                           );
   printf( "\n"                                                              );
   printf( " -ss     Serial sort method:\n"                                  );
   printf( "         select - selection sort\n"                              );
   printf( "         insert - insertion sort\n"                              );
   printf( "         bubble - bubble sort\n"                                 );
   printf( "         radix  - high performance radix sort\n"                 );
   printf( "         qsort  - quick sort(default)\n"                         );
   printf( "\n"                                                              );
   printf( " -id     Input bucket file directory.  This is the\n"            );
   printf( "         location where the individual files of phase\n"         );
   printf( "         1 will be written.  Give relative or absolute path.\n"  );
   printf( "         Default is cwd.\n"                                      );
   printf( "\n"                                                              );
   printf( " -od     Output file directory.  This is the location where\n"   );
   printf( "         the single sorted output file will be written.\n"       );
   printf( "         Give relative or absolute path.  Default is cwd.\n"     );
   printf( "\n"                                                              );
   printf( " -df     Delete the created intput files and sorted output file\n"
	   "         on program completion.  Default is off.\n\n"            );
   printf( " -sfs    Sorted file size in integral GiB.  If this option is\n"
	   "         specified, the default values for all parameters are used\n"
	   "         and the number of records per bucket is set to create an\n"
	   "         output file of the specified size.  Use of this flag\n"
	   "         overrides the -rb flag and the -pm flag.\n\n"           );
   printf( " -dr     Dry run.  Display the header information only then exit.\n\n");
   printf( " -pm     Use specified GiB per PE. May be a real number. This option will\n"
	   "         sets the number of records per bucket required to fill the\n"
	   "         specified size.  Do not use with the -sfs flag.  Use of\n"
	   "         this flag overrides the -rb flag.\n\n"                  );
   printf( " -tilt   Execute niter iterations of the tilt kernel from CPU suite\n"
	   "         during the workflow.\n\n"                               );
   printf( " -popcnt Execute niter iterations of a popcount kernel.\n\n");
   printf( " -bmm    Execute niter iterations of the BMM kernel from CPU suite\n\n");
   printf( " -gmp    Execute niter iterations of a GMP kernel similar to CPU suite\n\n");
   printf( " -k      Execute niter iterations of a collective communications kernel. \n"
	   "         Specify the buffer size with -kbuf.  If a buffer size is specified\n"
	   "         but no number of iterations is specified, then the default is 1.\n\n");
   printf( " -kbuf   Use two buffers of size 2^n to do the collective communications\n"
	   "         kernel.  If -k is specified but -kbuf is not then the default size\n"
	   "         is 1 MiB per buffer.\n\n"); 
#ifndef _CRAYC
   printf( " -mmbs2  Execute niter iterations of the mmbs2 kernel.  This kernel uses\n"
	   "         the SSE2 intrinsics and is not supported by the Cray compiler;\n"
           "         an error will be displayed if this is attempted.\n\n");
#endif 
   printf( " -n      Name of sorted output file.  Default is \"sorted\".\n"  );
   printf( "\n"                                                              );
   printf( " -i      Output system information.\n"                           );
   printf( "\n"                                                              );
   printf( " -v      Output version information.\n"                          );
   printf( "\n"                                                              );
   printf( " -h      Help menu.\n"                                           );
   printf( "\n");                                          
}
