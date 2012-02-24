
#ifndef __METRICS_HPP__
#define __METRICS_HPP__

#include <iomanip>

/*
 * macros to make metrics collection easier
 *
 * In your source file, include this file and define something like this:
 * #define METRICS( METRIC ) \
 *    METRIC( "Metric name", some_variable ) \
 *    METRIC( "Other metric name", other_variable ) \
 *    METRIC( "Third metric name", thrid_variable )
 *
 * Then make "calls" to generate human-readable metric output like this:
 *   std::cout << STREAMIFY_HUMAN_METRICS( METRICS ) << std::endl;
 * and in CSV like this:
 *   std::cout << STREAMIFY_CSV_METRICS( METRICS ) << std::endl;
 *
 * Run all your experiments and collect their stdout in a log
 * file. Then you can extract the CSV output into its own file with a
 * command like this:
 *   egrep ^header\|^data experiment.log | sort -r | uniq > experiment.csv

 * This puts a single instance of the header line at the top of the
 * file, and dumps all the other output after it. (Note if lines are
 * the same the uniq will eliminate the duplicates, but this seems
 * unlikey when running experiments.)
 */



#define STREAMIFY_HUMAN_METRIC( n, d )    \
  << std::setw(50) << std::setfill(' ')  \
  << (n)                                 \
  << std::setw(0) << std::setfill('0')   \
  << ": "                                \
  << (d)                                 \
  << "\n"
#define STREAMIFY_HUMAN_METRICS( M )             \
  "Metrics in human-readable form:\n"            \
  "{\n" M( STREAMIFY_HUMAN_METRIC ) << "}"


#define STREAMIFY_CSV_METRIC_HEADER( n, d )     \
  << ", " << (n) 
#define STREAMIFY_CSV_METRIC_DATA( n, d )       \
  << ", " << (d) 
#define STREAMIFY_CSV_METRICS( M )                       \
  "Metrics in CSV:\n"                                    \
  << "header" M( STREAMIFY_CSV_METRIC_HEADER ) << "\n"   \
  << "data" M( STREAMIFY_CSV_METRIC_DATA )

#endif
