
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "option_macros.h"

#include "listwalk_options.h"



// declare options
#define LONG_NOARG_OPTION( n, c, d, f ) { #n, no_argument, NULL, c },
#define LONG_REQARG_OPTION( n, c, d, f ) { #n, required_argument, NULL, c },
static struct option long_options[] = {
  OPTIONS( LONG_REQARG_OPTION, LONG_NOARG_OPTION, LONG_REQARG_OPTION )
  {"help", no_argument, NULL, 'h'},
  {NULL, no_argument, NULL, 0}
};

//
// parse argc/argv and fill in options struct
//
struct options parse_options( int * argc, char ** argv[] ) {
  struct options opt;

  // initialize struct with default values
#define OPTION_INTBOOL_DEFAULT( n, c, d, f ) opt.n = d;
#define OPTION_STR_DEFAULT( n, c, d, f ) strncpy( opt.n, d, sizeof( opt.n ) );
  OPTIONS( OPTION_INTBOOL_DEFAULT, OPTION_INTBOOL_DEFAULT, OPTION_STR_DEFAULT );

  // initialize short options string
  // first, declare storage for short options string
#define OPTION_REQARG_SIZE( n, c, d, f ) 2 +
#define OPTION_NOARG_SIZE( n, c, d, f ) 1 +
  int short_options_len = OPTIONS( OPTION_REQARG_SIZE, OPTION_NOARG_SIZE, OPTION_REQARG_SIZE ) 2;
  char short_options[ OPTIONS( OPTION_REQARG_SIZE, OPTION_NOARG_SIZE, OPTION_REQARG_SIZE ) 2 ];
  // then fill it in
  int short_option_index = 0;
#define OPTION_REQARG_CHAR( n, c, d, f )	\
  short_options[ short_option_index++ ] = c;	\
  short_options[ short_option_index++ ] = ':';
#define OPTION_NOARG_CHAR( n, c, d, f ) short_options[ short_option_index++ ] = c;
  OPTIONS( OPTION_REQARG_CHAR, OPTION_NOARG_CHAR, OPTION_REQARG_CHAR );
  short_options[ short_option_index++ ] = 'h';
  short_options[ short_option_index++ ] = '?';
  short_options[ short_option_index ] = '\0';

  // now parse options
  int ch, option_index = 1;
  while ((ch = getopt_long( *argc, *argv, short_options,
			    long_options, &option_index )) >= 0) {
    switch (ch) {
    case 0: // flag set
      break;

      // generate 
#define OPTION_REQARG_INT_SET( n, c, d, f )	\
      case c:					\
	opt.n = atoi(optarg);			\
	if ( (f) != NULL ) { \
	  options_setter_t * setterp = (options_setter_t *) f; \
	  setterp(&opt); \
	}	   \
	break;
#define OPTION_NOARG_BOOL_SET( n, c, d, f ) \
      case c:				    \
	opt.n = 1;			    \
	if ( (f) != NULL ) { \
	  options_setter_t * setterp = (options_setter_t *) f; \
	  setterp(&opt); \
	}	   \
	break;
#define OPTION_REQARG_STR_SET( n, c, d, f )	   \
      case c:					   \
	strncpy( opt.n, optarg, sizeof( opt.n ) ); \
	if ( (f) != NULL ) {				       \
	  options_setter_t * setterp = (options_setter_t *) f; \
	  setterp(&opt);				       \
	}						       \
	break;
      OPTIONS( OPTION_REQARG_INT_SET, OPTION_NOARG_BOOL_SET, OPTION_REQARG_STR_SET );

      // add help text
    case 'h':
    case '?':
    default: 
      {
	printf("Available options:\n");
	struct option* optp;
	for (optp = &long_options[0]; optp->name != NULL; ++optp) {
	  if (optp->has_arg == no_argument) {
	    printf("  -%c, --%s\n", optp->val, optp->name);
	  } else {
	    printf("  -%c, --%s <ARG>\n", optp->val, optp->name);
	  }
	}
	exit(1);
      }
    }
  }
  return opt;
}

void print_options( struct options * opt ) {
#define OPTION_PRINT_INTBOOL( n, c, d, f ) printf("%50s: %d\n", #n, opt->n );
#define OPTION_PRINT_STR( n, c, d, f ) printf("%50s: %s\n", #n, opt->n );
  printf("options = {\n");
  OPTIONS( OPTION_PRINT_INTBOOL, OPTION_PRINT_INTBOOL, OPTION_PRINT_STR );
  printf("}\n");
}

  

/* int foo_main( int argc, char * argv[] ) { */

/*   struct options opt = parse_options( &argc, &argv ); */
/*   print_options( opt ); */
/*   printf( "Important options are cores=%d, messages=%d, type=%s.\n", opt.cores, opt.messages, opt.type ); */


/* #define OPTIONS2( OPTION_INT, OPTION_BOOL, OPTION_STR )			\ */
/*   OPTION_INT( cores			, 'c', 1, NULL )		\ */

/* #define METRICS( METRIC )						\ */
/*   METRIC( "Foo",			"%s",   "foo" );		\ */
/*   METRIC( "Bar",			"%s",   "bar" );		\ */
/*   METRIC( "Baz",			"%s",   "baz" ); */


/*   //PRINT_CSV_METRICS( METRICS ); */
/*   //PRINT_HUMAN_METRICS( METRICS ); */

/* #define OPTION_ACCESSOR( n ) opt.n */
/*   PRINT_HUMAN_OPTIONS_AND_METRICS( OPTIONS, METRICS ); */
/*   PRINT_CSV_OPTIONS_AND_METRICS( OPTIONS, METRICS ); */

/*   return 0; */
/* } */
