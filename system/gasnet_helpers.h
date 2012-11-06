
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#ifndef __GASNET_HELPERS_H__
#define __GASNET_HELPERS_H__

/// Macro to make error-checking GASNet function calls easier
#define GASNET_CHECK( code )				\
  do {							\
    int retval;						\
    if( (retval = code) != GASNET_OK ) {		\
      fprintf( stderr, "ERROR: %s returned %d\n"	\
	       "%s:%i: %s (%s)\n",			\
	       #code, retval, __FILE__, __LINE__,		\
	       gasnet_ErrorName(retval),		\
	       gasnet_ErrorDesc(retval) );		\
      fflush( stderr );					\
      gasnet_exit( retval );				\
    }							\
  } while(0)


#endif
