#ifndef __CBARRIER_HPP__
#define __CBARRIER_HPP__

/// Type for Node ID. 
#include <boost/cstdint.hpp>
typedef int16_t Node;

void cbarrier_cancel( );
bool cbarrier_wait( );
void cbarrier_init( Node num_nodes );
void cbarrier_cancel_local( );


#endif
