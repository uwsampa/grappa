#include "mpbs.h"
#include "utils.h"
#include "hpc_output.h"
#include "timer.h"
#include "ioops.h"

#include <errno.h>
#include <math.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>

#include <mpi.h>

#define barrier_all()  MPI_Barrier(MPI_COMM_WORLD);

char *pt_readm          = NULL; //< Read method "file" or "stripe"
char *pt_wrtem          = NULL; //< Write method "ttb" or "random"
char *pt_nsrt           = NULL; //< Name of the sorted file.
char *pt_rundir         = NULL; //< Execution directory.
char *pt_logpath        = NULL; //< Absolute path to log file. Set to pt_rundir.
char *pt_idir           = NULL; //< Directory for the bucket files.
char *pt_odir           = NULL; //< Directory for the output file.
char *pt_parsort        = NULL; //< Parallel sorting method.

char mem_unit_size[]    = "kB";

uint64 mem_total        = 0;    //< Total system memory.
uint64 mem_free         = 0;    //< Free system memory.

uint32 num_c_cpus       = 0;    //< Number of 
uint32 num_a_cpus       = 0;

uint32 iflag            = 0;    //< If true, print system info and quit
uint32 vflag            = 0;    //< If true, print version info and quit
uint32 hflag            = 0;    //< If true, print help and quit
uint32 dfflag           = 0;    //< If true, delete generated files after execution
uint32 drflag           = 0;    //< If true, print header and quit.

uint64 t_fsize          = 0;    //< Total size of all bucket files.

int main( int argc, char *argv[] )
{
   char *pt_fname;

   int pow_of_two   = 0;

   int i;
   int len          = 0;
   int pos          = 0;

   // Size of each bucket file.
   uint64 fsize     = 0;

   /*
    * file pointers for sorted file
    * and data files
    */
   FILE *pt_sorted_file;

   /*
    *  execution timing variables
    */
   timer t;
   timer t_whole;
 
   MPI_Init(&argc, &argv);
   MPI_Comm_rank(MPI_COMM_WORLD, &id);
   MPI_Comm_size(MPI_COMM_WORLD, &npes);

   if( !npes ) {
     fprintf( stderr, "Error determining the number of PEs. \n");
     exit( 1 );
   }

   /*
    * get system cpu info
    */
   num_c_cpus = get_nprocs_conf();
   num_a_cpus = get_nprocs();

   /*
    * get system memory info
    */
   get_mem_info(&mem_total, &mem_free);
   
   /*
    * process command line parameters
    */
   int success =  cmdline( argc, argv );
   if( !success )
     goto cleanup;

   /*
    * call help function
    */
   if( hflag ){ 
     if( id == 0 ) {
         help();
     } 
     goto cleanup;
   }

   /*
    * check if sys info and version to be output
    */
   if( iflag && vflag )
   {
      if( id == 0 )
      {
         printf( "v" );
         printf( PACKAGE_VERSION"\n" );
         print_sys_stats();
      }
      return( 0 );
   }

   /*
    * check if system version should be output
    */
   if( vflag )
   {
      if( id == 0 )
         printf( PACKAGE_VERSION"\n" );
      return( 0 );
   }

   /*
    * check if system information should be output
    */
   if( iflag )
   {
      if( id == 0 )
         print_sys_stats();
      return( 0 );
   }

   /*
    * check if the number of processes is greater than
    * the number of files.
    * If so then warn and print help.
    */
   if( npes > nf )
   {
      if( id == 0 ) {
	printf_single(id, "MPBS ERROR: Number of process elements exceeds number of files!\n");
      }
      goto cleanup;
   }

   /*
    * check if the number of processes is greater than
    * the number of records per bucket.
    * If so then warn and print help.
    */
   if( npes > rb )
   {
      if( id == 0 ) {
	printf_single(id, "MPBS ERROR: Number of process elements exceeds the number "
		      "of records per bucket!\n" );
         
      }
      goto cleanup;
   }

   pow_of_two = npes && !(npes & (npes - 1));
   if( !strcmp(pt_parsort, "hybrid") && !pow_of_two ) {
     printf_single(id, "MPBS ERROR: Hybrid sorting method requires a power of two number "
	    "of PEs. (Number of threads need not be a power of two).\n");
     goto cleanup;
   }

   /*
    * adjust input bucket file directory path string to trim
    * off trailing /'s
    */
   len = strlen( pt_idir );
   pos = len - 1;
   while( pt_idir[pos] == '/' )
      pos--;

   /*
    * fix to set end of string
    */
   pt_idir[pos+1] = '\0';

   /*
    * get the current working directory at beginning of program
    */
   pt_rundir  = getcwd( NULL, 0 );

   /*-----------------------set log file path--------------------------*/
   /*
    * get log file
    */
   pt_logpath = newstr( pt_rundir, mk_logfn() ); 

   /*
    * process 0 will establish and maintain the log file
    */
   if( id == 0 )
   {
      if( !( pt_log = fopen( pt_logpath, "w" ) ) )
      {
         perror( "fopen" );
         exit( -1 );
      }
   }
   /*------------------------------------------------------------------*/


   /*
    * calculate file size information
    */
   bsize   = rb * rs;
   fsize   = nb * bsize;
   t_fsize = fsize * nf;

   /*
    * start of benchmark timing system
    */
   timer_clear(&t_whole);
   timer_start(&t_whole);

   if( id == 0 ) {
     header( npes );
   }

   /*
    * dry run flag prints the header then quits.
    */
   if( drflag )
     goto cleanup;

   /*
    * changing to the target directory
    */
   change_dir( pt_idir );

   /*
    * process 0 will establish the sorted file
    */
   if( id == 0 ) {
     if( !( pt_sorted_file = fopen( pt_ofpath, "w" ) ) ) {
       perror( "fopen" );
       goto cleanup;
     }
     fclose( pt_sorted_file );
   }

   /*
    * figure my share of the work
    */
   work();

   /*-----------------------make the data files------------------------*/
   printf_single(id, "Making data files...\n");

   timer_clear(&t);
   timer_start(&t);

   if( !strcmp( pt_wrtem, "rand" ) )
   {
      /* switch on record size */
      switch( rs ) {
      case 8:
	make_files_random_exclusive64();
	break;
      case 16:
	make_files_random_exclusive128();
	break;
      default:
	lprintf_single( id, pt_log, "mpbs(): wrong record size for mkfiles?\n" );
      }
   }
   else if( !strcmp( pt_wrtem, "stripe" ) )
   {
     /*
      * process 0 will establish all the data files
      */
     FILE *pt_bucket_file;

     if( id == 0 ) {
       for( i = 0; i < nf; i++ ) {
	 pt_fname = mkfname( i );
	 if( !( pt_bucket_file = fopen( pt_fname, "w" ) ) ) {
	     perror( "fopen" );
	     exit( -1 );
	   }
	 fclose( pt_bucket_file );
       }
     }

     barrier_all();
     
     /* switch on record size */
     switch( rs ) {
     case 8:
       make_files_sequential_striped64();
       break;
     case 16:
       make_files_sequential_striped128();
       break;
     default:
       lprintf_single( id, pt_log, "mpbs(): wrong record size for mkfiles?\n" );
     }
   }
   else
   {
      /* switch on record size */
      switch( rs ) {
      case 8:
	make_files_sequential_exclusive64();
	break;
      case 16:
	make_files_sequential_exclusive128();
	break;
      default:
	lprintf_single( id, pt_log, "mpbs(): wrong record size for mkfiles?\n" );
      }
   }
   
   printf_single(id, "Done making data files.\n");

   /* print section's summary timing information */
   timer_stop(&t);
   print_times_and_rate( id, &t, t_fsize, "\bB" );
   log_times(pt_log, id, "FILE CREATION", &t, t_fsize);

   barrier_all();
   /*------------------------------------------------------------------*/


   /*--------------------process the data in the files-----------------*/
   printf_single(id, "Processing data files...\n" );

   timer_clear(&t);
   timer_start(&t);

   if( !strcmp( pt_readm, "stripe" ) ) {
     /* switch on record size */
     switch( rs ) {
     case 8:
       read_files_striped64();
       break;
     case 16:
       read_files_striped128();
       break;
     default:
       lprintf_single( id, pt_log, "ERROR mpbs(): wrong record size for reading files?\n" );
      }
   }
   else {
     /* switch on record size */
     switch( rs ) {
     case 8:
       read_files_exclusive64();
       break;
     case 16:
       read_files_exclusive128();
       break;
     default:
       lprintf_single( id, pt_log, "mpbs(): wrong record size for reading files?\n" );
     }
   }
   printf_single( id, "Done processing files!\n" );

   /* print section's summary timing information */
   timer_stop( &t );
   print_times_and_rate( id, &t, t_fsize, "\bB" );
   log_times( pt_log, id, "FILE PROCESSING", &t, t_fsize );

   barrier_all();
   /*------------------------------------------------------------------*/

   /*
    * end of benchmark timing
    */
   timer_stop( &t_whole );

   /*
    * print output summary
    */
   if( id == 0 )
      trailer( &t_whole );

   /*
    * close log file
    */
   if( id == 0 )
      fclose( pt_log );

   if( id == 0 && dfflag ) {
     char *files = newstr( pt_idir, "/*.dat");
     char *rm_cmd = newstr("rm -rf ", files);
     char *outfile = newstr( pt_odir, pt_nsrt);
     char *rm_outfile = newstr("rm -rf ", outfile);
     printf("Removing input data files: '%s'\n", files);
     system(rm_cmd);
     printf("Removing output sorted file: '%s'\n", outfile);
     system(rm_outfile);
     free(rm_cmd);
     free(files);
     free(outfile);
   }

 cleanup:
   MPI_Finalize();

   return( 0 );
}


void work( )
/********1*****|***2*********3*********4*********5*********6*********7**!

               *******************************************
               *   UNCLASSIFIED//FOR OFFICIAL USE ONLY   *
               *******************************************

source file:   ioops.src

description:   This function calculates the number of files and
               bucket stripes for which each process is responsible.

 invoked by:   routine            description
               --------------     --------------------------------------
               main             - int
                                  This is the main driving program for
                                  the MPBS system. 

  variables:   varible            description
               --------------     --------------------------------------
               nf_base          - int
                                  Minimum number of files that each
                                  process will process.

               rem_files        - int
                                  Number of files beyond num_file_base
                                  that have to be distributed across
                                  first remain_files processes.

               nr_base          - int
                                  Minimum number of records that each
                                  process will process.

               rem_rec          - int
                                  Number of records beyond num_rec_base
                                  that have to be distributed across
                                  first remain_rec processes.

       lmod:   07/12/11

*********1*****|***2*********3*********4*********5*********6*********7**/
{
   int nf_base;
   int rem_files;

   int nr_base;
   int rem_rec;

   /****************/
   /*  file load   */
   /*  balance     */
   /****************/
   nf_base   = nf / npes;
   rem_files = nf % npes;
   
   if( id < rem_files ) {
     nf_base += 1;
     file_start  = nf_base * id;
   }
   else {
     if( id == 0 ) {
       file_start  = 0;
     }
     else {
       file_start = rem_files * (nf_base+1) + (id-rem_files) * nf_base;
     }
   }
   file_stop = file_start + nf_base - 1;
   
   /******************/
   /*  record load   */
   /*  balance       */
   /******************/
   nr_base = rb / npes;
   rem_rec = rb % npes;

   if( id < rem_rec ) {
     nr_base += 1;
     bstripe_start = nr_base * id;
   }
   else {
     if( id == 0 ) {
       bstripe_start = 0;
     }
     else {
       bstripe_start = rem_rec * (nr_base+1) + (id-rem_rec) * nr_base;
     }
   }
   bstripe_stop = bstripe_start + nr_base - 1;
   
   return;
}

int cmdline( int na, char *args[] ) {
  return cmdline_common(na, args) && cmdline_specific(na, args);
}

int cmdline_specific(int na, char *args[] ) {
#define NUM_SPECIFIC_ARGS 1

  char buf1[256];
  char buf2[256];
  
  int arg_count;
  int found_odir = 0;
  int found_nsrt = 0;
  int len1       = 0;
  int len2       = 0;
  
  typedef struct {
    char flag[10];
    char def[10];
  } parinfo;
  
  /*
   * define pointer and make 10 records
   */
  parinfo *parms;
  
  /*
   * allocate memory for default parameter information
   */
  parms = (parinfo *)malloc( NUM_SPECIFIC_ARGS * sizeof( parinfo ) );
  
  /*
   * set default parameters
   */
  strcpy( parms[0].flag, "-parsort" );
  strcpy( parms[0].def, "basic" );
  
  arg_count = na - 1;
  while (arg_count > 0) {
    if( !strcmp( args[ na - arg_count ], "-parsort" ) ) {
      /*
       * must look at next array element
       */
      arg_count--;
      
      /*
       * allocate array for sort method string 
       */
      if( arg_count == 0 || parms[0].def[0] == '-' ) {
	printf_single( id, "ERROR: Parallel sort method not set!\n" );
	return 0;
      }
      
      /*
       * put bucket directory in array
       */
      strcpy( parms[0].def, args[ na - arg_count ] );
      
      /*
       * check sort method string against valid options
       */
      if( !strcmp( parms[0].def, "basic" ) ||
	  !strcmp( parms[0].def, "hybrid" )) {
	  ;
      } 
      else {
	printf_single( id, "ERROR: Serial sort method, %s, is not valid!\n", 
		       parms[0].def);
	return 0;
      }
      
      /*
       * must decrement to leave while loop
       */
      arg_count--;
    }
    else {
      // If I don't recognize it, then skip it. 
      arg_count--;
    }
  }
   
  /* check bucket sort method */
  if( !pt_parsort ) {
    pt_parsort=(char *)malloc( strlen( parms[0].def ) * sizeof( char ) );
    if( !pt_parsort )
      perror( "malloc" );
    strcpy( pt_parsort, parms[0].def );
  }
  
  return 1;
}
 
void header( uint32 np ) {
  header_common(np);
  header_specific(np);
}

void header_specific( uint32 np ) {
  printf("* MPI SPECIFIC OPTIONS:\n"
	 "* Parallel sort method: %s\n"
	 "***************************************************************************\n",
	 pt_parsort);
  return;
}

/**
 * Prints the output summary at conclusion of the program. 
 */
void trailer( timer *t ) {
  trailer_common( t );
  trailer_specific( t );
}

void trailer_specific( timer *t ) {
  /*
   * No MPI specific trailer information yet.
   */
  return;
}

void help() {
  help_common();
  help_specific();
}

void help_specific() {
  printf( " \nMPI SPECIFIC OPTIONS:\n" );
  printf( " -parsort   Parallel sort method:\n"
	  "            basic - bucket sort\n"
	  "            hybrid - MPI/OpenMP bucket sort\n\n");
  return;
}
