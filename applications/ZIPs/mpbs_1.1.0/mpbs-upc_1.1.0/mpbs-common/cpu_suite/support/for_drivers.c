/* CVS info                         */
/* $Date: 2010/04/01 21:53:40 $     */
/* $Revision: 1.6 $                 */
/* $RCSfile: for_drivers.c,v $      */

#include "bench.h"
#include "uqid.h"

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX (256)
#endif

/* prints information, initializes PRNG, returns number of iterations */

#define INIT_ST  "INIT>"
#define END_ST   "END>"
#define INFO_ST  "INFO>"

#define BADUSER  "root"

#define NUL      ('\0')

#ifdef __BMK_SHORT_FORMAT
// This structure holds data to be printed out in a single line
struct {
  char*  progname;
  uint64 seed;
  int64  niters;
  double wall;
  double cpus;
  long   launch_time;
  long   cnode_start;
  int    correct;
  char*  hostname;
  char*  argv[8];
  long   started_at;
  long   ended_at;
  int    status;
  int    uqidv;
} p_line;

void set_status(int status){
  p_line.status = status;
}
void get_status(int* status){
  *status = p_line.status;
}

#endif

extern char u_tag[];

unsigned long e_strtoul(const char *nptr){
  /**********************************************\
  *  Calls strtoul with base 10 and check for    *
  *  errors.                                     *
  \**********************************************/
  unsigned long lt;
  char* endptr;

  lt = strtoul( nptr, &endptr, 10);
  if (nptr == endptr && lt == 0UL && errno == ERANGE ){
    // H&S pg 423
    err_msg_ret("ERROR: string \"%s\" can't be converted to unsigned long\n", nptr);
    return ULONG_MAX;
  }
  if (*nptr == NUL || *endptr != NUL ){
    err_msg_ret("ERROR: string \"%s\" contains an invalid character\n", nptr);
    return ULONG_MAX;
  }
  return lt;
}

int64 bench_init (int* argc, char *argv[], brand_t *br,
			 timer *t, /*@null@*/ const char * const more_args)

{
  uint64 seed;
  int64 niters;
  int i;
  time_t c;
  static char host[HOST_NAME_MAX];

  time_t launch_time, sleep_time, cnode_start, time_diff;
  int extra_args = 0;
  struct utsname info;

  char* login;
  int uqidv = UID_NOT_USED;

  // If each process needs a unique id initial the process
  UQID_INT();
  UQID(&uqidv);
#ifdef __BMK_SHORT_FORMAT
  if (uqidv == UID_NOT_USED){
    // This is for Farms (i.e RES)
      p_line.uqidv = 0;
  }else{
    // This is for MPPs
    p_line.uqidv = uqidv;
  }
#endif
  /*****************************************************\
  *  Only print out POSIX information if its different  *
  *  than than requested in file 'common_inc.`          *
  \*****************************************************/
#ifndef __BMK_SHORT_FORMAT
  printf ("\n===================================================\n\n");
  printf( "%s Benchmark Version %s\n",  INFO_ST, iobmversion);

#  ifdef _POSIX_VERSION
#    if ( _POSIX_VERSION != __ASK_FOR )
        printf( "%s _POSIX_VERSION = %ld\n", INFO_ST, _POSIX_VERSION) ;
#    endif
#  else
      printf( "%s _POSIX_VERSION undefined ", INFO_ST ) ;
#  endif

#  ifdef _XOPEN_UNIX
#    ifdef _XOPEN_VERSION
#      if ( _XOPEN_VERSION != _XOPEN_SOURCE )
          printf( "%s _XOPEN_VERSION = %d\n", INFO_ST, _XOPEN_VERSION ) ;
#      endif
#    else
        printf( "%s _XOPEN_VERSION undefined\n", INFO_ST ) ;
#    endif
#  else
     printf( "%s _XOPEN_UNIX undefined\n", INFO_ST ) ;
#  endif

   /**************************************************************\
    *  Print 'uname' information about where the program ran      *
   \**************************************************************/
   errno = 0;
   if ( uname( &info ) == -1 ) {
     printf( "%s no uname data\n",  INFO_ST );
   } else {
     printf( "%s system name: %s\n", INFO_ST, info.sysname);
     printf( "%s node name:   %s\n", INFO_ST, info.nodename);
     printf( "%s release:     %s\n", INFO_ST, info.release);
     printf( "%s version:     %s\n", INFO_ST, info.version);
     printf( "%s machine:     %s\n", INFO_ST, info.machine);
   }
   /**************************************************************\
   *  Print compiler(s) and  flags                                *
   \**************************************************************/

   print_cmplr_flags( INFO_ST );
#endif
   /**************************************************************\
   *  Not allowed to run as 'root' in any fashion                 *
   \**************************************************************/

   errno = 0;
   if ( gethostname (host, HOST_NAME_MAX ) == -1 ){
     strcpy(host, "unknown_host");
   }

   snprintf(u_tag, (size_t) 299, "<On %s><from %s>", host, argv[0] );

   if ( (getuid() == 0) || (geteuid() == 0 ) ){
     err_msg_exit("Not valid to run as root");
   }

   errno = 0;
   if ( (login = getlogin()) == NULL){
     login = "in_background";
     /*     if( errno != ENXIO ){
       // Has a controlling tty
       sys_err_exit(errno,"getlogin" );
       }*/
   }
   if (strncmp(login, BADUSER, (size_t) 5) == 0 ){
     err_msg_exit("ERROR can't run logged in as %s", login );
   }


   if ((i = (int)sizeof(void *)) != 8) {
     err_msg_exit("ERROR: sizeof(void *) = %d", i);
   }
  if ((i = (int)sizeof(long)) != 8) {
    err_msg_exit("ERROR: sizeof(long *) = %d", i);
  }
  if ((i = (int)sizeof(int)) != 4) {
    err_msg_exit("ERROR: sizeof(int) = %d", i);
  }

  if (*argc < 3) {

	if (!strncmp(&argv[0][strlen(argv[0]) - 3], "bmm",3)){
		printf("Usage:\t%s seed 55179\n",argv[0]);
		exit(EXIT_SUCCESS);
	}

	else if (!strncmp(&argv[0][strlen(argv[0]) - 12], "mmap_file_in",12)){
		printf("Usage:\t%s seed 16 directory\n",argv[0]);
		exit(EXIT_SUCCESS);
	}
	else if (!strncmp(&argv[0][strlen(argv[0]) - 21], "read_from_cnode_cache",21)){
		printf("Usage:\t%s seed 64 directory\n",argv[0]);
		exit(EXIT_SUCCESS);
	}
	else{
    /* prog seed iters [... other args] */
    		printf ("Usage:\t%s seed iters %s\n",
	    		argv[0], (more_args != NULL) ? more_args : "");
    		exit(EXIT_SUCCESS);
	}
  }

  cnode_start = time(NULL);
  launch_time = time(NULL);
  sleep_time = 0;

  if ( *argc >= 5 && argv[3][0] == '-'){
    if( argv[3][1] == 's'){ // seconds to sleep
      sleep_time = (time_t)e_strtoul(argv[4] );
      // option -s first
      if( sleep_time == (time_t) ULONG_MAX  ){
	err_msg_exit("ERROR in sleep time option -s\n");
      }
      extra_args += 2;
    }
    if( argv[3][1] == 'l'){ // launched time in seconds since epoch
      // option -l first
      launch_time = (time_t)e_strtoul( argv[4] );
      if ( launch_time == (time_t) ULONG_MAX ){
	err_msg_exit("ERROR in launch time option -l\n");
      }
      extra_args += 2;
    }
  }

  if ( *argc >= 7 && argv[5][0] == '-'){
    if( argv[5][1] == 's'){ // seconds to sleep
      // option -s second
      sleep_time = (time_t) e_strtoul(argv[6]);
      if ( sleep_time == (time_t) ULONG_MAX ){
	err_msg_exit("ERROR in launch time option -s\n");
      }
      extra_args += 2;
    }

    if( argv[5][1] == 'l'){ // launched time in seconds since epoch
     // option -l second
      launch_time = (time_t) e_strtoul(argv[6]);
      if ( launch_time == (time_t) ULONG_MAX ){
	err_msg_exit("ERROR in launch time option -s\n");
      }
      extra_args += 2;
    }
  }

  i = 3;
  while( argv[i+extra_args] ){
    argv[i] = argv[i+extra_args];
    i++;
  }
  argv[i] = NULL;
  *argc = *argc - extra_args;

  if ( (long)launch_time < 1221237961L ) { // Launch a long time ago. Must be an error
    char ldate[80];
    struct tm * tm_ptr;
    tm_ptr = localtime(&launch_time);
    (void) strftime(ldate, (size_t)80,"%c",tm_ptr);
    err_msg_exit("ERROR a launch at epoch %ld  %s  is impossible.",
		 launch_time, ldate );
  }

  time_diff = cnode_start - launch_time;

#ifndef __BMK_SHORT_FORMAT
  printf ("%s started on cnode at %ld  -  launched at epoch %ld = diff %ld: Sleep  %ld - diff  %ld = %ld\n",
	  INFO_ST, (long)cnode_start, (long)launch_time, (long)time_diff,
	  (long)sleep_time, (long)time_diff, (long)(sleep_time - time_diff) );
   printf("\n");
#endif

   // Remove time used to launch the job
   sleep_time = sleep_time - time_diff;
   if( sleep_time < 0 ){
     sleep_time = 0;
     err_msg_exit("ERROR: Benchmark arrived at node too late\n");
   }
   // Sleep remaining time
   sleep( sleep_time );
  /* print start time of day */
   (void)time (&c);
#ifdef __BMK_SHORT_FORMAT
   p_line.started_at = c;
#else
   printf ("%s %s started at:  %s", INIT_ST, argv[0], ctime(&c));
#endif
   t->init_time = c;
#ifndef __BMK_SHORT_FORMAT
   printf ("%s host machine is %s\n", INIT_ST, host);

   printf ("%s program built on %s @ %s\n",
	   INIT_ST,  __DATE__, __TIME__);
#endif

   if (!strcmp(argv[1], "-1")){
    int my_uid;
    uqid(&my_uid);
    seed = (uint64) my_uid;
   }
   else
	seed   = (uint64) e_strtoul(argv[1]);

   //   seed   = seed + uqidv;
   niters = (long) e_strtoul(argv[2]);



#ifdef __BMK_SHORT_FORMAT
  p_line.progname = argv[0];
  p_line.seed = seed;
  p_line.niters = niters;
  p_line.launch_time = launch_time;
  p_line.cnode_start = cnode_start;
  p_line.hostname = host;
  p_line.status = 0;
#else
  printf ("%s seed is %lu   niters is %ld\n", INIT_ST, (unsigned long)seed, niters);
#endif
  if (*argc > 3 ) {
    argv += 3;
#ifndef __BMK_SHORT_FORMAT
    printf ("%s other args: ", INIT_ST);
    while (*argv)
      printf (" %s", *argv++);
    printf ("\n");
#else
    // save the arguments
    i = 0;
    while (*argv){
      p_line.argv[i] = *argv++;
      i++ ;
    }
#endif
  } // end if


  if (br != NULL)
    brand_init (br, seed);

  if (t != NULL)
    timer_clear (t);

#ifndef __BMK_SHORT_FORMAT
  printf ("\n");
#endif

  return niters;
}

void bench_end (timer *t, const int64 iters, /*@null@*/ const char* const work)

{
  time_t c;
  double wall, cpus;
#ifndef __BMK_SHORT_FORMAT
  double rate;
#endif
  int i;

#ifndef __BMK_SHORT_FORMAT
  /* Write out the data in the old format, human readable */
  printf ("\n");

  /* print end time of day */
  (void) time (&c);
  printf ("%s ended at:  %s", END_ST, ctime(&c));
  c = c - t->init_time;

  printf ("%s elapsed time is %ld seconds\n", END_ST, (long)c);

  if (t != NULL) {
    timer_report(t, &wall, &cpus, 0);

    printf ("%s %7.3f secs of wall time      ",
	    END_ST, wall);
    if (c <= 0) c = 1;
    printf ("%7.3f%% of value reported by time()\n", wall/c*100.);

    if (wall <= 0)  wall = 0.0001;
    printf ("%s %7.3f secs of cpu+sys time   utilization = %5.3f%%\n",
	    END_ST, cpus, cpus/wall*100.);

    if (cpus > (wall+.01))
      printf ("this result is suspicious since cpu+system > wall\n");
    if ((iters > 0) && (work != NULL)) {
      const char *units[5] = {"", "Ki", "Mi", "Gi", "Ti"};
      int i = 0;

      rate = (double)iters/wall;
      while (i < 4) {
	if (rate > 999.999) {
	  rate /= 1024.;
	  i++;
	}
	else
	  break;
      }

      printf ("%s %8.4f %s %s per second\n",
	      END_ST, rate, units[i], work);
    }
  }
#else
  /* Write out data as tab separated columns. Suitable for awk, Perl, exel, ... */
  (void) time (&c);
  p_line.ended_at = c;
  timer_report(t, &wall, &cpus, 0);

  p_line.wall = wall;
  p_line.cpus = cpus;

  // Comma separated output
  //name,wall,status,cpu,ltime,start,startedat,emd,host,seed,iter,uqidv
  // printf("%s,%lf,%d,%lf,%ld,%ld,%ld,%ld,%s,%lu,%ld,%d",

  //Tab separated output
  //    name wall status cpu  ltime start startedat emd     host    seed  iter  uqidv
  printf("%s\t%lf\t%d\t%lf\t%ld\t%ld\t%ld\t%ld\t%s\t%lu\t%ld\t%d",
	 p_line.progname, p_line.wall, p_line.status, p_line.cpus, p_line.launch_time,
	 p_line.cnode_start, p_line.started_at, p_line.ended_at,
	 p_line.hostname,  p_line.seed, p_line.niters, p_line.uqidv);

  for(i=0;i<8;i++){
    if(  p_line.argv[i] != NULL ){
      printf ("\t%s", p_line.argv[i]);
      //printf (",%s", p_line.argv[i]);
    }else {
      break;
    }
  }
  printf ("\n");
#endif
  // Clean up UQID
  UQID_FINALIZE();

  return;
}

/*
Emacs reads the stuff in this comment to set local variables
Local Variables:
compile-command: "make for_drivers.o "
End:
*/

