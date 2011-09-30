

#define PRINT_HUMAN_METRIC( n, f, d ) printf("%50s: " f "\n", n, d)
#define PRINT_HUMAN_METRICS( M )		\
  do {						\
    printf("{\n");				\
    M( PRINT_HUMAN_METRIC );			\
    printf("}\n");				\
  } while(0)




#define PRINT_CSV_METRIC_HEADER( n, f, d ) printf(", %s", n)
#define PRINT_CSV_METRIC_DATA( n, f, d ) printf(", " f, d)
#define PRINT_CSV_METRICS( M )			\
  do {						\
    printf("header");				\
    M( PRINT_CSV_METRIC_HEADER );		\
    printf("\n");				\
    printf("data");				\
    M( PRINT_CSV_METRIC_DATA );			\
    printf("\n");				\
  } while (0)
