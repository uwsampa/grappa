
#ifndef __SOFTXMT_HPP__
#define __SOFTXMT_HPP__

#include "Communicator.hpp"
#include "Aggregator.hpp"


void SoftXMT_init( int * argc_p, char ** argv_p[] );
void SoftXMT_activate();
void SoftXMT_finish( int retval );

Node SoftXMT_nodes();
Node SoftXMT_mynode();

void SoftXMT_poll();
void SoftXMT_flush( Node n );

void SoftXMT_barrier();



#endif
