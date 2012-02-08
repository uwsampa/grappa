
#ifndef __GASNET_HELPERS_H__
#define __GASNET_HELPERS_H__

// wrap gasnet calls with this
#define GASNET_CHECK( code )				\
  do {							\
    int retval;						\
    if( (retval = code) != GASNET_OK ) {		\
      fprintf( stderr, "ERROR: %s returned %d\n"	\
	       "%s:%i: %s (%s)\n",			\
	       #code, __FILE__, __LINE__,		\
	       gasnet_ErrorName(retval),		\
	       gasnet_ErrorDesc(retval) );		\
      fflush( stderr );					\
      gasnet_exit( retval );				\
    }							\
  } while(0)


#endif
