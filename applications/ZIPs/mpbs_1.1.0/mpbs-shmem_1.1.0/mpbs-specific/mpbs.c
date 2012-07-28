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

#include <mpp/shmem.h>

#define barrier_all()  shmem_barrier_all()

char *pt_readm          = NULL; //< Read method "file" or "stripe"
char *pt_wrtem          = NULL; //< Write method "ttb" or "random"
char *pt_nsrt           = NULL; //< Name of the sorted file.
char *pt_rundir         = NULL; //< Execution directory.
char *pt_logpath        = NULL; //< Absolute path to log file. Set to pt_rundir.
char *pt_idir           = NULL; //< Directory for the bucket files.
char *pt_odir           = NULL; //< Directory for the output file.

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
/******************************************************************************

  variables:   variable           description
               --------------     ----------------------------------------------

               i                - int
                                  Loop over number of processes 
                                  using shmem to grab other processes'
                                  allocated memory totals.

               len              - int
                                  Length of bucket directory string.

               pos              - int
                                  Position counter in bucket directory
                                  character array.

               pt_sf            - FILE *
                                  This is the file pointer for the
                                  sorted file.  This file contains
                                  all of the data sorted lowest to
                                  highest.

               start            - times
                                  Wall clock & clock tick times.
                                  This is the overall program start
                                  time.

               stop             - times
                                  Wall clock & clock tick times.
                                  This is the overall program stop
                                  time.

               start_mkfs       - times
                                  Wall clock & clock tick times.
                                  This is the start time to make all
                                  of the files.

               stop_mkfs        - times
                                  Wall clock & clock tick times.
                                  This is the end time to make all
                                  of the files.

               start_rdfl       - times
                                  Wall clock & clock tick times.
                                  This is the start time to read and
                                  sort all of the data in the files
                                  and write the final output file.

               stop_rdfl        - times
                                  Wall clock & clock tick times.
                                  This is the stop time to read and
                                  sort all of the data in the files
                                  and write the final output file.

               ets              - e_times
                                  Elapsed cpu time in seconds for the
                                  benchmark, ( cpu_stop - cpu_start ), 
                                  and elapsed wall time in seconds for
                                  the benchmark,(wall_stop - wall_start).
                                  This is the elapsed time for the
                                  whole program.

               ets_mkfs         - e_times
                                  Elapsed cpu time in seconds for the
                                  benchmark, ( cpu_stop - cpu_start ), 
                                  and elapsed wall time in seconds for
                                  the benchmark,(wall_stop - wall_start).
                                  These are the elapsed times to make all
                                  the files.

               ets_rdfl         - e_times
                                  Elapsed cpu time in seconds for the
                                  benchmark, ( cpu_stop - cpu_start ), 
                                  and elapsed wall time in seconds for
                                  the benchmark,(wall_stop - wall_start).
                                  These are the elapsed times to read
                                  read and sort all of the data.
*********1*****|***2*********3*********4*********5*********6*********7**/
{
   /*
    * variable declarations
    */
   char *pt_fname;

   int i;
   int len          = 0;
   int pos          = 0;

   // Value to return to the shell.
   int retval       = 0;

   // Size of each bucket file.
   uint64 fsize     = 0;

   /*
    * file pointers for sorted file
    * and data files
    */
   FILE *pt_sf;
   FILE *pt_bf;

   /*
    *  execution timing variables
    */
   timer t;
   timer t_whole;
 
   /*******************************
    * shmem initialization
    ********************************/
#ifdef SHMEM
   start_pes( 0 );
#ifdef CRAY
   npes = shmem_n_pes();
   id   = shmem_my_pe();
#endif
#ifdef SGI
   npes = _num_pes();
   id   = _my_pe();
#endif
#endif

   /*
    * The Makefile also protects against this, but these are here just in case.
    */
#if defined(_USE_O_DIRECT) && defined(SHMEM)
   printf("Use of O_DIRECT is only supported in the MPI-1 and MPI-2 versions.\n");
   MPI_Finalize();
   return 1;
#endif

   if( !npes ) {
     fprintf( stderr, "Error determining the number of PEs. Compile with "
	      "either -DCRAY or -DSGI\n");
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
   cmdline( argc, argv );

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
     if(id == 0)
       print_sys_stats();

     printf_single( id, "v%s\n", PACKAGE_VERSION );
     goto cleanup;
   }

   /*
    * check if system version should be output
    */
   if( vflag )
   { 
     printf_single( id, "v%s\n", PACKAGE_VERSION );
     goto cleanup;
   }

   /*
    * check if system information should be output
    */
   if( iflag )
   {
      if( id == 0 )
         print_sys_stats();

      goto cleanup;
   }

   /*
    * check if the number of processes is greater than
    * the number of files.
    * If so then warn and print help.
    */
   if( npes > nf )
   {
     printf_single(id, "ERROR: Number of PEs exceeds the number of files!\n");
     retval = 1;
     goto cleanup;
   }

   /*
    * check if the number of processes is greater than
    * the number of records per bucket.
    * If so then warn and print help.
    */
   if( npes > rb )
   {
     printf_single( id, "Number of PEs exceeds the number of records per  bucket!\n" );
     retval = 1;
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
         retval = 1;
	 goto cleanup;
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
   if( drflag ) {
     goto cleanup;
   }

   /*
    * changing to the target directory
    */
   change_dir( pt_idir );

   /*
    * process 0 will establish the sorted file
    */
   if( id == 0 ) {
     if( !( pt_sf = fopen( pt_ofpath, "w" ) ) ) {
       perror( "fopen" );
       exit( -1 );
     }
     fclose( pt_sf );
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
	retval = 1;
	goto cleanup;
      }
   }
   else if( !strcmp( pt_wrtem, "stripe" ) )
   {
   /*
    * process 0 will establish all the data files
    */
     if( id == 0 ) {
       for( i = 0; i < nf; i++ ) {
	 pt_fname = mkfname( i );
	 if( !( pt_bf = fopen( pt_fname, "w" ) ) )
	   {
	     perror( "fopen" );
	     exit( -1 );
	   }
	 fclose( pt_bf );
       }
     }
     barrier_all();

      /* switch on record size *\/ */
     switch( rs ) {
     case 8:
       make_files_sequential_striped64();
       break;
     case 16:
       make_files_sequential_striped128();
       break;
     default:
       lprintf_single( id, pt_log, "mpbs(): wrong record size for mkfiles?\n" );
       retval = 1;
       goto cleanup;
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
       retval = 1;
       goto cleanup;
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

   if( !strcmp( pt_readm, "stripe" ) )
   {
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
	retval = 1;
	goto cleanup;
      }
   }
   else
   {
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
	retval = 1;
	goto cleanup;
      }
   }
   printf_single(id, "Done processing files!\n" );

   /* print section's summary timing information */
   timer_stop(&t);
   print_times_and_rate( id, &t, t_fsize, "\bB" );
   log_times(pt_log, id, "FILE PROCESSING", &t, t_fsize);

   barrier_all();
   /*------------------------------------------------------------------*/

   /*
    * end of benchmark timing
    */
   timer_stop(&t_whole);

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

   if( id == 0 && dfflag) {
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
#ifdef SHMEM
#ifdef CRAY
   shmem_finalize();
#endif
#endif

   return( retval );
}


void work( void )
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

   if( id < rem_files )
   {
      nf_base += 1;
      file_start  = nf_base * id;
   }
   else
   {
      if( id == 0 )
      {
         file_start  = 0;
      }
      else
      {
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

   if( id < rem_rec )
   {
      nr_base += 1;
      bstripe_start = nr_base * id;
   }
   else
   {
      if( id == 0 )
      {
         bstripe_start = 0;
      }
      else
      {
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
  /*
   * No command line arguments specific to SHMEM yet.
   */
  return 1;
}
 
void header( uint32 np ) {
  header_common(np);
  header_specific(np);
}

void header_specific( uint32 np ) {
  /*
   * No SHMEM specific header information yet.
   */
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
   * No SHMEM specific trailer information yet.
   */
  return;
}

void help() {
  help_common();
  help_specific();
}

void help_specific() {
  return;
}
