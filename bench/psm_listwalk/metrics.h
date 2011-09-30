
/*
 * macros to make metrics collection easier
 *
 * In your source file, include this file and define something like this:
 * #define METRICS( METRIC ) \
 *    METRIC( "Metric name", "%d", some_int_variable ); \
 *    METRIC( "Other metric name", "%f", some_double_variable ); \
 *    METRIC( "Third metric name", "%s", a_string_variable );
 *
 * Then make "calls" to generate human-readable metric output like this:
 *   PRINT_HUMAN_METRICS( METRICS );
 * and in CSV like this:
 *   PRINT_CSV_METRICS( METRICS );
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



#define PRINT_HUMAN_METRIC( n, f, d ) printf("%50s: " f "\n", (n), (d))
#define PRINT_HUMAN_METRICS( M )		\
  do {						\
    printf("{\n");				\
    M( PRINT_HUMAN_METRIC );			\
    printf("}\n");				\
  } while(0)




#define PRINT_CSV_METRIC_HEADER( n, f, d ) printf(", %s", (n))
#define PRINT_CSV_METRIC_DATA( n, f, d ) printf(", " f, (d))
#define PRINT_CSV_METRICS( M )			\
  do {						\
    printf("header");				\
    M( PRINT_CSV_METRIC_HEADER );		\
    printf("\n");				\
    printf("data");				\
    M( PRINT_CSV_METRIC_DATA );			\
    printf("\n");				\
  } while (0)
