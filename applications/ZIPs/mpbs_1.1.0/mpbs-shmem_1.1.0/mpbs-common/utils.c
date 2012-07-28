#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/netdevice.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <math.h>

#include "utils.h"
#include "types.h"
#include "hpc_output.h"
#include "kernels.h"
#include "timer.h"

#include <time.h>


/*
 * Prototypes for functions not to be exported.
 */
char * get_timestamp( void );


uint32 get_num_sockets( const char *file )
/********1*****|***2*********3*********4*********5*********6*********7**!

description:   Counts the number of sockets per node. 

 invoked by:   routine            description
               --------------     --------------------------------------
               get_sys_stats    - void (utils.src)
                                  This program gets system statistics
                                  by opening the /proc/cpuinfo if on
                                  a Linux based machine.

  variables:   varible            description
               --------------     --------------------------------------
               file             - const char *
                                  File to be parsed for cpu information.
                                  This will be the cpuinfo file.

               i, j             - uint32
                                  General loop variables.

               num_socks        - uint32
                                  Number of sockets found on node.

               line[]           - char
                                  Will temporarily store each line
                                  of file.

               phys_id[]        - char
                                  Will temporarily store first 11 chars
                                  of each read line. 

               id_str[]         - char
                                  Stores physical id which is the number
                                  of sockets on node.

               phys_id_str[]    - const char
                                  This is the string that is sought. 

               pt_f             - FILE *
                                  File to be read.

*********1*****|***2*********3*********4*********5*********6*********7**/
{
   /*
    * variable declaration section
    */
   uint32 i, j;
   uint32 num_socks = 0;

   char line[BUFSIZ];
   char phys_id[12];
   char id_str[BUFSIZ];
   const char phys_id_str[] = "physical id";

   FILE *pt_f;

   phys_id[11] = '\0';
   if( ( pt_f = fopen( file, "r" ) ) != NULL )
   {
      /*
       * loop over each line in file
       */
      while( fgets( line, sizeof(line), pt_f ) != NULL )
      {
         /*
          * load first 11 characters of line which corresponds to
          * the physical id
          */
         for( i = 0; i < 11; i++ )
            phys_id[ i ] = line[ i ];

         /*
          * compare strings to see if physical id line found
          */
         if( strcmp( phys_id_str, phys_id ) == 0 )
         {
            /*
             * found a socket, must now get the number
             */
            j = 0;
            i = 14;
            while( isdigit( line[ i ] ) != 0 )
            {
               id_str[ j++ ] = line[ i++ ]; 
            }         
            id_str[ j ] = '\0';

            /* turn into an int */
            num_socks = atoi( id_str );
         }
      }
   }
   else
   {
      perror( "fopen" );
   }
   fclose( pt_f );

   return( num_socks + 1 );
}


uint32 get_num_cores_per_socket( const char *file )
/********1*****|***2*********3*********4*********5*********6*********7**!

description:   Retrieves the number of cores per socket from the
               /proc/cpuinfo file. 

 invoked by:   routine            description
               --------------     --------------------------------------
               get_sys_stats    - void (utils.src)
                                  This program gets system statistics
                                  from the /proc/cpuinfo file if on
                                  a Linux based machine.

  variables:   varible            description
               --------------     --------------------------------------
               file             - const char *
                                  File to be parsed for cpu information.
                                  This will be the cpuinfo file.
             
               i, j             - uint32
                                  General loop variables.

               num_cps          - uint32
                                  Number of cores per socket.

               line[]           - char
                                  Will temporarily store each line
                                  of file.

               cpu_core[]       - char
                                  Will temporarily store first 9 chars
                                  of each read line.

               core_str[]       - char
                                  Stores core per socket number.

               cpu_cores[]      - const char
                                  This is the string that is sought. 

               pt_f             - FILE *
                                  File pointer.  This will be for the
                                  cpuinfo file.

*********1*****|***2*********3*********4*********5*********6*********7**/
{
   /*
    * variable declaration section
    */
   uint32 i, j;
   uint32 num_cps = 0;

   char line[BUFSIZ];
   char cpu_core[10];
   char core_str[BUFSIZ];
   const char cpu_cores[] = "cpu cores";

   FILE *pt_f;

   cpu_core[9] = '\0';
   if( ( pt_f = fopen( file, "r" ) ) != NULL )
   {
      while( fgets( line, sizeof(line), pt_f ) != NULL )
      {
         /*
          * load first 9 characters of line which corresponds to
          * the cpu cores 
          */
         for( i = 0; i < 9; i++ )
            cpu_core[ i ] = line[ i ];
 
         /*
          * compare strings to see if cpu cores line found
          */
         if( strcmp( cpu_cores, cpu_core ) == 0 ) 
         {
            /*
             * found a cpu core, must now get the number
             */
            j = 0;
            i = 12;
            while( isdigit( line[ i ] ) != 0 )
            {
               core_str[ j++ ] = line[ i++ ]; 
            }         
            core_str[ j ] = '\0';

            /* turn into an int */
            num_cps = atoi( core_str );
            break;
         }
      }
   }
   else
   {
      perror( "fopen" );
   }
   fclose( pt_f );

   return( num_cps );
}


uint32 get_num_logical_cpus( const char *file )
/********1*****|***2*********3*********4*********5*********6*********7**!

description:   Counts the number of processors from the
               /proc/cpuinfo file. 

 invoked by:   routine            description
               --------------     --------------------------------------
               get_sys_stats    - void (utils.src)
                                  This program gets system statistics
                                  from the /proc/cpuinfo file if on
                                  a Linux based machine.

  variables:   varible            description
               --------------     --------------------------------------
               file             - const char *
                                  File to be parsed for cpu information.
                                  This will be the cpuinfo file.

               i                - uint32
                                  General loop variables.

               num_logical_
               cpus             - uint32
                                  Number of threads per core/logical
                                  cpus per core.

               line[]           - char
                                  Will temporarily store each line
                                  of file.

               proc_id[]        - char
                                  Will temporarily store first 12 chars
                                  of each read line.

               proc_id_str[]    - const char
                                  This is the string that is sought.

               pt_f             - FILE *
                                  File to be read.

*********1*****|***2*********3*********4*********5*********6*********7**/
{
   /*
    * variable declaration section
    */
   uint32 i;
   uint32 num_logical_cpus = 0;

   char line[BUFSIZ];
   char proc_id[12];
   const char proc_id_str[] = "processor";

   FILE *pt_f;

   proc_id[9] = '\0';
   if( ( pt_f = fopen( file, "r" ) ) != NULL )
   {
      while( fgets( line, sizeof(line), pt_f ) != NULL )
      {
         /*
          * load first 9 characters of line which corresponds to
          * the processor
          */
         for( i = 0; i < 9; i++ )
            proc_id[ i ] = line[ i ];

         /*
          * compare strings to see if processor line found
          */
         if( strcmp( proc_id_str, proc_id ) == 0 )
            num_logical_cpus++;
      }
   }
   else
   {
      perror( "fopen" );
   }
   fclose( pt_f );

   return( num_logical_cpus );
}


char * get_mem_value( const char *line )
/********1*****|***2*********3*********4*********5*********6*********7**!

description:   This function finds a number in a string delimited
               by spaces.

 invoked by:   routine            description
               --------------     --------------------------------------
               get_mem_info     - void
                                  This function gets the total and free
                                  memory.

  variables:   varible            description
               --------------     --------------------------------------
               line             - const char *
                                  String read in from file and passed
                                  in by get_mem_info.

               number[]         - char
                                  Holds the number string found in line.

               mem_value        - char *
                                  Holds the number string from number
                                  allocated to specific length.

               i, j             - int
                                  Position counter in strings. 

*********1*****|***2*********3*********4*********5*********6*********7**/
{
   /*
    * variable declaration section
    */
   char number[20] = { ' ' };
   char *mem_value = NULL;
   int i = 0;
   int j = 0;

   do
   {
      if( isalpha( line[i] ) )
         ;
      if( isspace( line[i] ) )
         ;
      if( isdigit( line[i] ) )
      {
         number[j] = line[i];
         j++;
      }
      i++;
   } while( line[i] != '\n' );

   number[j] = '\0';
   mem_value = malloc( (j+1)*sizeof(char) );
   strcpy( mem_value, number );

   return( mem_value );
}

void get_mem_info( uint64 *memtotal, uint64 *memfree )
/********1*****|***2*********3*********4*********5*********6*********7**!

description:   This function gets the total and free memory.

 invoked by:   routine            description
               --------------     --------------------------------------
               main             - int (mpbs.src)
                                  This is the main driving program for
                                  the Massive Parallel Bucket Sort(MPBS)
                                  system.

    invokes:   routine            description
               ---------------    --------------------------------------
               get_mem_value    - char *
                                  Finds a number in a string delimited
                                  by spaces.

  variables:   varible            description
               --------------     --------------------------------------
               file             - FILE *
                                  File pointer that points to memory
                                  info file.

               line[]           - char
                                  Holds a line of text read in from
                                  /proc/meminfo file.

*********1*****|***2*********3*********4*********5*********6*********7**/
{
   /*
    * variable declaration section
    */
   FILE *file;
   char line[BUFSIZ];

   //char *get_mem_value( const char * );

   if( (file = fopen( "/proc/meminfo", "r" )) != NULL )
   {
      /* get first line */
      fgets( line, sizeof( line ), file );
      *memtotal = atoi( get_mem_value( line ) );

      /* get second line */
      fgets( line, sizeof( line ), file );
      *memfree = atoi( get_mem_value( line ) );
   } else {
      perror( "fopen" );
   }
   fclose( file );

   return;
}

net_info get_netinfo( uint32 idx )
/********1*****|***2*********3*********4*********5*********6*********7**!

description:   This function will return host network information.

 invoked by:   routine            description
               --------------     --------------------------------------
               header           - void (utils.src)
                                  Prints the output header.

  variables:   varible            description
               --------------     --------------------------------------
               idx              - uint32
                                  Index for potential network card name.

               s                - int
                                  Handle to socket. 

               ifconf           - ifconf
                                  Structure used in SIOCGIFCONF request.
                                  Used to retrieve interface configuration
                                  for machine (useful for programs which
                                  must know all networks accessible).

               ifr[]            - ifreq
                                  Interface request structure used for
                                  socket ioctl's.

               ip[]             - char
                                  Character array that stores the ip
                                  address.               

               net              - net_info
                                  Stores network device name and ip
                                  address.

*********1*****|***2*********3*********4*********5*********6*********7**/
{
   /*
    * variable declaration section
    */
   int s;
   char ip[ INET_ADDRSTRLEN ];

   struct ifconf ifconf;
   struct ifreq ifr[ 50 ];

   net_info net;

   /*
    * create a network socket
    */
   s = socket( AF_INET, SOCK_STREAM, 0 );
   if( s < 0 ) {
      perror( "socket" );
      exit( -1 );
   }

   ifconf.ifc_buf = (char *)ifr;
   ifconf.ifc_len = sizeof ifr;

   if( ioctl( s, SIOCGIFCONF, &ifconf ) == -1 ) {
      perror( "ioctl" );
      exit( -1 );
   }

   struct sockaddr_in *s_in = (struct sockaddr_in *) &ifr[ idx ].ifr_addr;

   if( !inet_ntop( AF_INET, &s_in->sin_addr, ip, sizeof( ip ) ) ) {
      perror( "inet_ntop" );
      exit( -1 );
   }

   net.if_name = malloc( strlen( ifr[ idx ].ifr_name ) );
   net.ip      = malloc( strlen( ip ) );

   strcpy( net.if_name, ifr[ idx ].ifr_name );
   strcpy( net.ip, ip );

   close( s );

   return( net );
}


void change_dir( char *pt_dir ) {
   int err;

   err = chdir( pt_dir );
   if( err ) {
      perror( "chdir" );
      exit(1);
   }
   
   return;
}


int len_f( double ft )
/********1*****|***2*********3*********4*********5*********6*********7**!

description:   This function returns the length of a float.

 invoked by:   routine            description
               --------------     --------------------------------------
               trailer          - void (utils.src)
                                  Prints the output summary at conclusion
                                  of program. 

  variables:   varible            description
               --------------     --------------------------------------
               ft               - float
                                  Floating point number for which length
                                  is required.

               len              - uint32
                                  Length of floating point number.

               rem              - uint32
                                  Remainder as the result of a modulus
                                  calculation.

               fti              - uint32
                                  Number that was originally a float.

*********1*****|***2*********3*********4*********5*********6*********7**/
{
   /*
    * variable declaration section
    */
   uint64 len = 0;
   uint64 rem = 0;
   uint64 fti = (uint64)ft;

   /*
    * determine the length of the thread number
    *
    */
   if( fti < 10 )
   {
      len = 1;
   }
   else
   {
      rem = fti % 10;
      fti = fti / 10;
      len++;
      while( fti > 0 )
      {
         rem = fti % 10;
         fti = fti / 10;
         len++;
      }
   }
   return( len + 2 );
}


int len_i( uint64 it )
/********1*****|***2*********3*********4*********5*********6*********7**!

description:   This function returns the length of an integer.

 invoked by:   routine            description
               --------------     --------------------------------------
               header           - void (utils.src)
                                  Prints the output header.

               trailer          - void (utils.src)
                                  Prints the output summary at conclusion
                                  of program. 

  variables:   varible            description
               --------------     --------------------------------------
               it               - uint64 
                                  Number for which length is required.

               len              - uint64
                                  Length of floating point number.

               rem              - uint64
                                  Remainder as the result of a modulus
                                  calculation.

*********1*****|***2*********3*********4*********5*********6*********7**/
{
   /*
    * variable declaration section
    */
   uint64 len = 0;
   uint64 rem = 0;

   /*
    * determine the length of the thread number
    *
    */
   if( it < 10 )
   {
      len = 1;
   }
   else
   {
      rem = it % 10;
      it  = it / 10;
      len++;
      while( it > 0 )
      {
         rem = it % 10;
         it  = it / 10;
         len++;
      }
   }
   return( len );
}


char * itos( uint64 n )
/********1*****|***2*********3*********4*********5*********6*********7**!

escription:   Integer is converted to a string. 

 invoked by:   routine            description
               --------------     --------------------------------------
               mkfname          - char * (utils.src)
                                  This subroutine reads in process's
                                  record stripe which are all of the
                                  records striped across all files for a
                                  given bucket.

  variables:   varible            description
               --------------     --------------------------------------
               n                - uint32 
                                  Number for which length is required.

               i                - int
                                  Generic loop variable.

               m                - int
                                  Stores integer to be converted.  Used
                                  in determining the length of integer.
 
               len              - uint32
                                  Length of floating point number.

               rem              - uint32
                                  Remainder as the result of a modulus
                                  calculation.

               pt_str           - char *
                                  String representing of integer.

               c                - char
                                  Generic character holder.

*********1*****|***2*********3*********4*********5*********6*********7**/
{
   /*
    * variable declaration section
    */
   int i           = 0;
   int m           = 0;
   uint32 len      = 0;
   uint32 rem      = 0;

   char *pt_str = NULL;
   char c;

   /*
    * determine the length of the thread number
    */
   m = n;
   if( m < 10 )
   {
      len = 1;
   }
   else
   {
      rem = m % 10;
      m   = m / 10;
      len++;
      while( m > 0 )
      {
         rem = m % 10;
         m   = m / 10;
         len++;
      }
   }

   /*
    * allocate memory for string 
    */
   pt_str = (char *)malloc( (len + 1) * sizeof( char ) );
   pt_str[len] = '\0';   /* add end of string char */

   /* load string with chars from high to low indices */
   for( i = len - 1; i >= 0; i-- )
   {
      rem = n % 10;
      n   = n / 10;
      switch( rem ) {

         case 0:
                  c = '0';
                  break;
         case 1:
                  c = '1';
                  break;
         case 2:
                  c = '2';
                  break;
         case 3:
                  c = '3';
                  break;
         case 4:
                  c = '4';
                  break;
         case 5:
                  c = '5';
                  break;
         case 6:
                  c = '6';
                  break;
         case 7:
                  c = '7';
                  break;
         case 8:
                  c = '8';
                  break;
         case 9:
                  c = '9';
                  break;
         default:
                  printf( "%d error in itos\n", rem );
      }
      pt_str[i] = c;
   }
   return( pt_str );
}

char * mkfname( int file_num )
/********1*****|***2*********3*********4*********5*********6*********7**!

description:   This subroutine reads in process's record stripe which
               are all of the records striped across all files for a
               given bucket.

 invoked by:   routine            description
               --------------     --------------------------------------
               mkfilesXY        - void (ioops.src)
                                  These functions create the data files
                                  for each process.

               rstrpsXY         - void (ioops.src)
                                  These functions read in process's
                                  record stripe which are all of the
                                  records striped across all files for
                                  a given bucket.

    invokes:   routine            description
               ---------------    --------------------------------------
               len_i            - int (utils.src)
                                  This function returns the length of
                                  an integer.

  variables:   varible            description
               --------------     --------------------------------------
               i                - int
                                  Generic loop variable.

               len_fnum         - int
                                  Length of file index number.

               len_fsx          - int
                                  Length of file sufix ".dat" which
                                  is 4.

               len_tot          - int
                                  Total length of file name =
                                  len_fnum + len_fsx.

               pos              - int
                                  String position index counter.

               suffix[]         - const char
                                  Suffix string ".dat".               

               pt_name          - char *
                                  File name string.

               pt_fnum          - char *
                                  File index string.

*********1*****|***2*********3*********4*********5*********6*********7**/
{
   /*
    * variable declaration section
    */
   int i;
   int len_fnum;
   int len_fsx = 4;
   int len_tot;
   int pos;

   const char suffix[] = ".dat";
   
   char *pt_name;
   char *pt_fnum;

   /* get length of file index number */
   len_fnum = len_i( file_num );

   /* len_tot  = len_id + len_fnum + len_fsx + 2;*/  /* '-' + '\0' = 2 */
   len_tot  = len_fnum + len_fsx + 1;  /* '\0' = 1 */

   /* allocate memory for file name */
   pt_name  = (char *)malloc( len_tot * sizeof( char ) );
   pt_name[len_tot-1] = '\0';

   /* create file number string */
   pt_fnum = itos( file_num );

   /* add in id string */
   pos = 0;
   /*
   for( i = 0; i < len_id; i++ )
      pt_name[pos++] = pt_id[i];
   */

   /* add dash separator */
   /*
   pt_name[pos++] = '-';
   */

   /* add in number string */
   for( i = 0; i < len_fnum; i++ )
      pt_name[pos++] = pt_fnum[i];

   /* add in suffix string */
   for( i = 0; i < 4; i++ )
      pt_name[pos++] = suffix[i];

   free(pt_fnum);
   return( pt_name );
}


char * newstr( char *s1, char *s2 )
/********1*****|***2*********3*********4*********5*********6*********7**!

description:   Concatenates s2 to the end of s1 and returns a pointer
               two the new string.

 invoked by:   routine            description
               --------------     --------------------------------------
               main             - int (mpbs.src)
                                  This is the main driving program for
                                  the MPBS system.

  variables:   varible            description
               --------------     --------------------------------------
               s1               - char *
                                  String passed in from calling function.

               s2               - char *
                                  String passed in from calling function.

               s                - char *
                                  Concatenated string.

               len1             - int
                                  Length of s1.

               len2             - int
                                  Length of s2.

               i                - int
                                  General loop variable.

*********1*****|***2*********3*********4*********5*********6*********7**/
{
   /*
    * variable declaration section
    */
   char *s;
   int len1;
   int len2;
   int i;

   len1 = strlen( s1 );
   len2 = strlen( s2 );

   /*
    * allocate new string
    */
   s = (char *)malloc( (len1+len2+1)*sizeof(char) );

   /* first place directory in string */
   for( i = 0; i < len1; i++ )
      s[i] = s1[i];

   /* second place name in string */
   for( i = 0; i < len2; i++ )
      s[i+len1] = s2[i];
   s[len1+len2] = '\0';

   return( s );
}

char * bytes_to_human( double rt ) {
  char *to_return;
  char *rate_string;
  
  char *units[6] = {"", " KiB", " MiB", " GiB", " TiB", " PiB"};
  int i = 0;
  
  while (i < 6) {
    if (rt >= 1024.0) {
      rt /= 1024.0;
      i++;
    }
    else
      break;
  }
  
  rate_string = ftos( rt );
  to_return = newstr(rate_string, units[i]);
  free(rate_string);
  return( to_return );
}

char * dhms( int t )
/********1*****|***2*********3*********4*********5*********6*********7**!

description:   Converts seconds to days, hours, minutes and seconds and
               returns a string of the following format to the calling
               function:

                       seconds=Days-Hours:Minutes:Seconds
                       XXXXXXs=DDd-HH:MM:SS

 invoked by:   routine            description
               --------------     --------------------------------------
               tsum             - void (utils.src)
                                  Prints timing summary information for
                                  various sections of execution to
                                  standard output and mpbs log file.

               trailer          - void (utils.src)
                                  This function prints the output summary
                                  at conclusion of program.

    invokes:   routine            description
               ---------------    --------------------------------------
               len_i            - int (utils.src)
                                  This function returns the length of
                                  an integer.

               itos             - char * (utils.src)
                                  Integer is converted to a string.

  variables:   varible            description
               --------------     --------------------------------------
               t                - int
                                  Seconds passed in from calling
                                  function.

               s                - char *
                                  Final string in the format
                                  XXXXXXs=DDd-HH:MM:SS returned to
                                  calling function.

               s_t              - char *
                                  Representation of passed seconds in t.

               s_secs           - char *
                                  Representation of calculated seconds
                                  in t.

               s_mins           - char *
                                  Representation of calculated minutes
                                  in t.

               s_hours          - char *
                                  Representation of calculated hours
                                  in t.

               s_days           - char *
                                  Representation of calculated days
                                  in t.

               i                - int
                                  General loop variable.

               pos              - int
                                  Position counter in strings.

               secs             - int
                                  Calculated seconds in t.

               mins             - int
                                  Calculated minutes in t.

               hours            - int
                                  Calculated hours in t.

               days             - int
                                  Calculated days in t.

               l_t              - int
                                  Length of t seconds integer.

               l_secs           - int
                                  Length of secs integer.

               l_mins           - int
                                  Length of mins integer.

               l_hours          - int
                                  Length of hours integer.

               l_days           - int
                                  Length of days integer.

               l_tot            - int
                                  Sum of all lengths above. 7 is
                                  added to accomodate the other
                                  parts of the string, i.e. 's=d-::\0'.
                                  This will be the length of the
                                  string s.

*********1*****|***2*********3*********4*********5*********6*********7**/
{
   /*
    * variable declaration section
    */
   char *s;
   char *s_t;
   char *s_secs;
   char *s_mins;
   char *s_hours;
   char *s_days;

   int i;
   int pos;
   int secs;
   int mins;
   int hours;
   int days;

   int l_t;
   int l_secs;
   int l_mins;
   int l_hours;
   int l_days;
   int l_tot;

   /*
    * breakout days, hours, min, secs.
    */
   secs  = t % 60;
   mins  = (t / 60) % 60;
   hours = (t / 3600) % 24;
   days  = (t / 86400);

   /*
    * calculate lengths of time ints
    */
   l_t     = len_i(   t   );
   l_secs  = len_i( secs  );
   l_mins  = len_i( mins  );
   l_hours = len_i( hours );
   l_days  = len_i(  days );

   /*
    * convert ints to strings
    */
   s_t     = itos(  t    );
   s_secs  = itos( secs  );
   s_mins  = itos( mins  );
   s_hours = itos( hours );
   s_days  = itos( days  );

   /*
    * calculate total length of time string
    */
   l_tot = l_t + l_secs + l_mins + l_hours + l_days + 7;

   /*
    * allocate time string
    */
   s = (char *)malloc( l_tot * sizeof(char) );

   /*
    * add in total seconds
    */
   for( i = 0; i < l_t; i++ )
      s[i] = s_t[i];
   s[l_t] = 's';
   pos = l_t;
   s[++pos] = '=';
   pos++;

   /*
    * add in days
    */
   for( i = 0; i < l_days; i++ )
      s[pos+i] = s_days[i];
   pos      += l_days;
   s[pos++]  = 'd';
   s[pos++]  = '-';

   /*
    * add in hours
    */
   for( i = 0; i < l_hours; i++ )
      s[pos+i] = s_hours[i];
   pos     += l_hours;
   s[pos++] = ':';

   /*
    * add in minutes
    */
   for( i = 0; i < l_mins; i++ )
      s[pos+i] = s_mins[i];
   pos     += l_mins;
   s[pos++] = ':';

   /*
    * add in minutes
    */
   for( i = 0; i < l_secs; i++ )
      s[pos+i] = s_secs[i];
   pos     += l_secs;
   s[pos++] = '\0';
   
   free(s_t);
   free(s_secs);
   free(s_mins);
   free(s_hours);
   free(s_days);
   
   return( s );
}


char * ftos( double f )
/********1*****|***2*********3*********4*********5*********6*********7**!

description:   Given a floating point number, returns a string
               representing the number to one decimal place.

 invoked by:   routine            description
               --------------     --------------------------------------
               newstr           - char * (utils.src)
                                  Concatenates s2 to the end of s1 and
                                  returns a pointer two the new string.

    invokes:   routine            description
               ---------------    --------------------------------------
               len_i            - int (utils.src)
                                  This function returns the length of
                                  an integer.

               itos             - char *
                                  Integer is converted to a string.

  variables:   varible            description
               --------------     --------------------------------------
               f                - double
                                  Floating point number passed in.

               dec[]            - char
                                  Decimal point followed by single
                                  digit because float is converted to
                                  string to one decimal place.

               c                - char
                                  Character representation of a single
                                  digit.

               s                - char *
                                  Final string returned to calling
                                  function which is the string repre-
                                  sentation of f.

               snum             - char *
                                  String representation of whole number
                                  part of f.

               i                - int
                                  Generic loop variable.

               len              - int
                                  Length of whole number part of f.

               num              - uint64
                                  Integer representation of whole number
                                  part of f.

               fi               - double
                                  Floating point representation of
                                  integer representation of whole number
                                  part of f.

               fd               - double
                                  Fractional part only of f.

*********1*****|***2*********3*********4*********5*********6*********7**/
{
   /*
    * variable declaration section
    */
   char dec[3];
   char c;
   char *s;
   char *snum;

   int i;
   int len = 0;
   uint64 num = 0;

   double fi = 0.0;
   double fd = 0.0;

   /*
    * cast float as integer thus
    * removing fractional part
    */
   num = (uint64)f;

   /*
    * calculate length of
    * integer
    */
   len = len_i( num );

   /*
    * convert integer
    * to string
    */
   snum = itos( num );

   /*
    * convert integer back to a
    * float without fractional part
    */
   fi = num;

   /*
    * isolate fractional part of
    * original float
    */
   fd = f - fi;

   /*
    * promote fraction to whole
    * number
    */
   fd *= 10;

   /*
    * set decimal point
    */
   dec[0] = '.';

   /*
    * determine which character represents
    * fractional part
    */
   switch( (uint64)fd ) {
      case 0:
               c = '0';
               break;
      case 1:
               c = '1';
               break;
      case 2:
               c = '2';
               break;
      case 3:
               c = '3';
               break;
      case 4:
               c = '4';
               break;
      case 5:
               c = '5';
               break;
      case 6:
               c = '6';
               break;
      case 7:
               c = '7';
               break;
      case 8:
               c = '8';
               break;
      case 9:
               c = '9';
               break;
      default:
	       c = '*';
               printf( "switch:case error in ftos\n" );
   }

   /*
    * finish making string
    */
   dec[1] = c;
   dec[2] = '\0';

   /*
    * allocate final string
    */
   s = (char *)malloc( (len + strlen(dec) + 1)*sizeof(char) );

   /*
    * fill with whole number part
    */
   for( i = 0; i < len; i++ )
      s[i] = snum[i];

   /*
    * fill with fractional part
    */
   for( i = 0; i < 2; i++ )
      s[len+i] = dec[i];

   /*
    * terminate string
    */
   s[len+strlen(dec)] = '\0';

   free(snum);
   return( s );
}

char * mk_logfn( void )
/********1*****|***2*********3*********4*********5*********6*********7**!

description:   Makes the MPBS log file name and returns it to the
               calling function. 

 invoked by:   routine            description
               --------------     --------------------------------------
               main             - int (mpbs.src)
                                  This is the main driving program for
                                  the MPBS system.

    invokes:   routine            description
               ---------------    --------------------------------------
               get_timestamp    - char * (utils.src)
                                  Creates and returns timestamp stirng.

               newstr           - char * (utils.src)
                                  Concatenates s2 to the end of s1 and
                                  returns a pointer two the new string.

               return           - void
                                  C library function that returns
                                  control to calling function.

  variables:   varible            description
               --------------     --------------------------------------
               prefix           - char *
                                  MPBS timestamped log file prefix which
                                  includes path separator at beginning.

               log              - char *
                                  MPBS timestamped log file suffix.
                                  Includes string terminator.

               fulllog          - char *
                                  Full path name of timestamped log file.

*********1*****|***2*********3*********4*********5*********6*********7**/
{
   /*
    * set the log file prefix and suffix
    */
   char *prefix = "/mpbs_";
   char *log    = ".log\0";
   char *fulllog;

   /*
    * make the full log file pathname
    */
   fulllog = newstr( newstr( prefix, get_timestamp() ), log );

   /*
    * return log name
    */
   return( fulllog );
}


char * get_timestamp( void )
/********1*****|***2*********3*********4*********5*********6*********7**!

               *******************************************
               *   UNCLASSIFIED//FOR OFFICIAL USE ONLY   *
               *******************************************

source file:   utils.src

description:   Creates and returns a timestamp string of the form

               year-mon-dayThour-min-sec.

 invoked by:   routine            description
               --------------     --------------------------------------
               mk_logfn         - char *
                                  Makes the MPBS log file name and
                                  returns it to the calling function.

    invokes:   routine            description
               ---------------    --------------------------------------
               time             - time_t
                                  Returns  the  time  since  the  Epoch
                                  (00:00:00 UTC, January 1, 1970),
                                  measured in seconds.

               localtime        - struct tm *
                                  Converts the calendar time t into a
                                  null-terminated string of the form.

                                  "Wed Jun 30 21:49:08 1993\n"

  variables:   varible            description
               --------------     --------------------------------------
               tmst             - char *
                                  Timestamp string. 

               ltime            - time_t
                                  Holds the number of seconds since
                                  the epoch.

               tm               - struct tm
                                  Includes the following members:
                                  int tm_sec;      Seconds.     [0-60]
                                  int tm_min;      Minutes.     [0-59]
                                  int tm_hour;     Hours.       [0-23]
                                  int tm_mday;     Day.         [1-31]
                                  int tm_mon;      Month.       [0-11]
                                  int tm_year;     Year - 1900.
                                  int tm_wday;     Day of week. [0-6]
                                  int tm_yday;     Days in year.[0-365]
                                  int tm_isdst;    DST.         [-1/0/1]

*********1*****|***2*********3*********4*********5*********6*********7**/
{
   char *tmst;

   time_t ltime;
   struct tm *tm;

   /*
    * get time information
    */
   ltime = time( NULL );
   tm = localtime( &ltime );

   /*
    * allocate memory for string
    */
   tmst = (char *)malloc( 20 * sizeof( char ) );

   /*
    * format timestamp string
    */
   sprintf( tmst, "%04d-%02d-%02dT%02d-%02d-%02d", tm->tm_year+1900,
            tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec );
   /* terminate string */
   tmst[19] = '\0';

   /*
    * return timestamp string
    */
   return( tmst );
}


int is_integer(char *arg) {
  unsigned long val;
  char *endptr;

  /*
   * Use strtol to determine if arg is a valid base 10 integer.  Because 
   * 0 is a valid return value for strtol, we need to zero out and check the
   * value of errno.
   */
  errno = 0;
  val = strtoul(arg, &endptr, 10);

  if( endptr == arg ) {
    return 0;
  }

  return (!errno);
}

uint64 get_num( char *str, int *valid )
/********1*****|***2*********3*********4*********5*********6*********7**!

description:   Returns long integer to calling function.  Takes numbers
               in two forms; either as a straight base 10 number or a
               number raised to a power.

 invoked by:   routine            description
               --------------     --------------------------------------
               cmdline          - void (utils.c)
                                  This is the main driving program for
                                  the MPBS system.

    invokes:   routine            description
               ---------------    --------------------------------------
               help             - void (utils.c)
                                  Prints help information for command
                                  line parameters.  Exits program.

  variables:   varible            description
               --------------     --------------------------------------
               str              - char *
                                  Incoming string representing integer
                                  given on command line.

               bstr             - char *
                                  Integer base number.  Found before '^'
                                  in string(str).

               bstr             - char *
                                  Integer exponent number.  Found after
                                  '^' in string(str).

               i                - int
                                  General loop variable.

               len              - int
                                  Length of incoming string(str).

               found            - int
                                  Logical variable used to determine
                                  if a '^' has been found in a string.
                                  1 if found and 0 if not found.

               pos              - int
                                  Position in string where '^' is
                                  found.

               before           - int
                                  Position in string before '^'.

               after            - int
                                  Position in string after '^'.

               n                - uint64
                                  Final number to be passed back to
                                  calling function.

               n1               - uint64
                                  Base integer found in '^' string.

               n2               - uint64
                                  Exponent integer found in '^' string.

*********1*****|***2*********3*********4*********5*********6*********7**/
{
   /*
    * variable declaration section
    */
   char *bstr = NULL;
   char *astr = NULL;

   int i;
   int len    = 0;
   int found  = 0;
   int pos    = 0;
   int before = 0;
   int after  = 0;

   uint64 n   = 0L;
   uint64 n1  = 0L;
   uint64 n2  = 0L;

   /*
    * must calculate length of incoming string
    */
   len = strlen( str );

   /*
    * look for possible ^ in string
    */
   for( i = 0; i < len; i++ )
      if( str[i] == '^' )
      {
         found  = 1;
         pos    = i;
         before = pos-1;
         after  = pos+1;
      }

   /*
    * if a '^' exists then must check that it
    * lies in a functional pos.  If at pos = 0 or
    * pos = len-1 then not correct
    */
   if( found == 1 )
   {
      if( pos == 0 || pos == len-1 )
      {
	 *valid = 0;
	 return 0;
      }
      else
      {
         /*
          * get number before '^'
          */
         bstr = (char *)malloc( (before+2) * sizeof( char ) );
         for( i = 0; i <= before; i++ )
            bstr[i] = str[i];
         bstr[before+1] = '\0';

         /*
          * get number after '^'
          */
         astr = (char *)malloc( (len-after+1) * sizeof( char ) );
         for( i = 0; i < len-after; i++ )
            astr[i] = str[after+i];
         astr[len-after] = '\0';
      }

      /*
       * calculate integer values from strings
       */
      n1 = atol( bstr );
      n2 = atol( astr );

      /*
       * calculate final integer
       */
      n = powl( n1, n2 );
      if(valid)
	*valid = 1;
   }
   else {
     /*
      * strtoul can return 0 as a valid number, so we need to check
      * the value of errno to determine if something went wrong.
      */
     errno = 0;
     char *first_invalid;
     n = strtoul( str, &first_invalid, 10 );
     if (str == first_invalid || errno > 0) {
       if(valid) {
	 *valid = 0;
       }
     }
   }
   return( n );
}
