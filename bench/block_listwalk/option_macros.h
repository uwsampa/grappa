
#ifndef __OPTION_MACROS_H__
#define __OPTION_MACROS_H__

#define OPTION_MAX_STR_LEN 128

struct options;
typedef void *options_setter_t( struct options * );



#include "listwalk_options.h"


//
// declare options struct
// this is how options are exposed to the user
//
#define OPTION_INTBOOL_DECL( n, c, d, f ) int n;
#define OPTION_STR_DECL( n, c, d, f ) char n[ OPTION_MAX_STR_LEN ];
/* #define OPTIONS_STRUCT()						\ */
/*   struct options {							\ */
/*     OPTIONS( OPTION_INTBOOL_DECL, OPTION_INTBOOL_DECL, OPTION_STR_DECL ) \ */
/*   }; */
struct options {
  OPTIONS( OPTION_INTBOOL_DECL, OPTION_INTBOOL_DECL, OPTION_STR_DECL )
};


struct options parse_options( int * argc, char ** argv[] );

void print_options( struct options * opt );

#endif
