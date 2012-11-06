#ifndef __UTILS_H
#define __UTILS_H

/**
 * @file
 * 
 * Miscellaneous utility functions.
 */

#include <stdio.h> 
#include <stdint.h>

#include "timer.h"

/* #ifdef _USE_DIRECT_IO */
/* #define _XOPEN_SOURCE 600 */
/* #endif */


/**
 * Type that stores network device name and ip address
 */
typedef struct {
  char *if_name;   //< network device name 
  char *ip;        //< ip address of device
} net_info;


/**
 * Counts the number of sockets on each node based on the contents of
 * /proc/cpuinfo
 * 
 * @param   file "/proc/cpuinfo" or similar file.
 * @returns Number of sockets per node.
 */
uint32_t get_num_sockets( const char *file );

/**
 * Counts the number of cores per socket based on the contents of
 * /proc/cpuinfo
 * 
 * @param   file "/proc/cpuinfo" or similar file.
 * @returns Number of cores per socket.
 */
uint32_t get_num_cores_per_socket( const char *file );

/**
 * Counts the number of sockets on each node based on the contents of
 * /proc/cpuinfo
 * 
 * @param   file "/proc/cpuinfo" or similar file.
 * @returns Number of CPUs.
 */
uint32_t get_num_logical_cpus( const char *file );

/**
 * Finds the total and free system memory from /proc/meminfo
 *
 * @param total [out] Address of integer to hold total memory info.
 * @param free  [out] Address of integer to hold free memory info.
 */  
void get_mem_info( uint64_t *total, uint64_t *free );

/**
 * Returns the host network name and IP address.
 * 
 * @param Index for potential network card name.
 *
 * @returns Filled network info.
 */
net_info get_netinfo( uint32_t idx );

/**
 * Wrapper for the C chdir function.  Exits on failure.
 *
 * @param dir Directory to change to.
 */
void change_dir( char *dir );

/**
 * Calculates the string length of a floating point number to one decimal place.
 * 
 * @param ft Floating point number.
 * 
 * @returns Length of the string representation of ft to one decimal place.
 */ 
int len_f( double ft );

/**
 * Calculates the string length of an integer.
 *
 * @param it Integer
 * 
 * @returns Length of the string representation of it.
 */
int len_i( uint64_t it );

/**
 * Convert an unsigned 64-bit integer to a string.
 *
 * @param n Integer to convert.
 * 
 * @returns Pointer to string representation of n. This pointer must be freed by
 *          the calling function.
 */ 
char * itos( uint64_t n );

/**
 * Creates a file name based on the passed in integer.  
 * Example: 1.dat 2.dat etc...
 *
 * @param file_num Integer for file name.
 * 
 * @returns String containing the file name.  This pointer must be freed by the
 *          calling function.
 */
char * mkfname( int file_num );

/**
 * Concatinate two strings and return their result in a newly allocated string.
 * 
 * @param s1 First string.
 * @param s2 Second string.
 *
 * @returns Null terminated concatination of s1 and s2.  This pointer must be
 *          freed by the calling function.
 */
char * newstr( char *s1, char *s2 );

/**
 * Converts a byte quantity or rate into a string.  Uses the largest SI prefix
 * that is appropriate for the input (MiB vs GiB etc...)
 *
 * @param rt The quantity to convert.
 * 
 * @returns String of the form %.1lf <units> where <units> is KiB, MiB, GiB...
 *          This pointer must be freed by the calling function.
 */
char * bytes_to_human( double rt );

/**
 * Converts seconds to days, hours, minutes and seconds and returns a string of
 * the following format:  
 *
 *                     seconds=Days-Hours:Minutes:Seconds
 *                     XXXXXXs=DDd-HH:MM:SS
 *
 * @param t Elapsed seconds.
 * 
 * @returns String in the above format. This pointer must be freed by the 
 *          calling function.
 */
char * dhms( int t );

/**
 * Converts a floating point number to a string representation out to one 
 * decimal place.
 *
 * @param f Input number
 * 
 * @returns String representation of f.  This pointer must be freed by the 
 *          calling function.
 */
char * ftos( double f );

/**
 * Generates the MPBS log filename.
 *
 * @returns Log filenam.  This pointer must be freed by the calling function.
 */
char * mk_logfn( );

/**
 * Converts a string to a long integer.  Two forms are accepted, a base 10 
 * number or a number raised to a power (ex. 2^32).
 * 
 * @param *str String to convert.
 * @param *is_valid [out] Set to non-zero if the return value is valid or 0
 *             if str does not represent a number.
 *
 * @returns The number represented by str.
 */
uint64_t get_num( char *str, int *is_valid );

/**
 * Returns true if the input is an integer, and false otherwise.
 *
 * @param *arg String to check.
 *
 * @returns True if arg is an integer, false otherwise.
 */
int is_integer(char *arg);

#endif
