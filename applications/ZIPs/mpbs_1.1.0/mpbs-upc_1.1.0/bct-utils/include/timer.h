#ifndef __TIMER_H
#define __TIMER_H

/**
 * @file timer.h
 *
 * @section DESCRIPTION   
 * Timing module for use in timing sections of code.  Operation
 * is like a stopwatch.
 */

#include <stdint.h>
#include <time.h>
#include <stdio.h>

typedef struct {
  double accum_wall; 
  double accum_cpus;
  double start_wall;
  double start_cpus;
  time_t init_time;
  char running;
} timer;

/**
 * Capture the current wall time.
 *
 * @return Seconds since the Epoch.  Granularity to microsecond level.
 */
double wall();

/**
 * Capture the CPU and system usage time.
 *
 * @return Seconds of system and CPU usage.  Granularity to microsecond level.
 */
double cpus();

/**
 * Zero out and clear the timer.
 * 
 * @param *t Address of timer to be cleared.
 */
void timer_clear (timer *t);

/**
 * Start the timer.
 *
 * @param *t Address of timer to start.
 */
void timer_start (timer *t);

/**
 * Pause the timer and update the accumulated running times.  Does not clear it.
 *
 * @param *t Address of timer to pause.
 */
void timer_stop  (timer *t);

/**
 * Capture the accumulated CPU and wall time of a running or stopped timer. 
 * This function is required to get valid accumulated times on a running timer 
 * but is optional for a stopped one.  If the timer is stopped the result is
 * equivalent to accessing the accumulated time fields directly.
 *
 * @param *t    Address of the timer to access
 * @param *wall Address of where to write the elapsed wall time.
 * @param *cpus Address of where to write the elapsed cpu and system time.
 */ 
void timer_peek(timer *t, double *wall, double *cpus);

/**
 * Print out times and a rate to standard out.  The rate is calculated and 
 * converted to a human readable format for display based on the op and nops
 * parameters.
 *
 * Example:
 *        CPU: 0.0920      Wall: 0.0928      Rate:    1.395 Gi Bytes/s
 *
 * @param id   Rank of calling process in a parallel environment.  If in a
 *             single PE environment simply pass in 0.
 * @param *t   Address of timer to print.
 * @param nops Number of operations performed or bytes sent during the span
 *             measured by t.
 * @param *op  String describing the operation that was timed.
 */
void print_times_and_rate(int id, timer *t, uint64_t nops, char *op);

/**
 * Print out elapsed CPU and wall time. 
 *
 * Example:
 *        CPU: 0.0920      Wall: 0.0928  
 *
 * @param id   Rank of calling process in a parallel environment.  If in a
 *             single PE environment simply pass in 0.
 * @param *t   Address of timer to print.
 */
void print_times(int id, timer *t);

/**
 * Print out times and a rate to a log file in a tab separated format.
 *
 * Example: 
 * FILE PROCESSING         0.104006        0.125085        65491.530196
 * 
 * @param *fout File stream to print to.
 * @param id    Rank of calling process in a parallel environment.  If in a
 *              single PE environment simply pass in 0.
 * @param *desc String describing the operation that was timed.
 * @param *t    Address of timer to print.
 * @param nops  Number of operations performed or bytes sent during the span
 *              measured by t. 
 */
void log_times(FILE *fout, int id, char *desc, timer *t, uint64_t nops);


#endif



