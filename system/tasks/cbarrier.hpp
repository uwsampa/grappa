#ifndef __CBARRIER_HPP__
#define __CBARRIER_HPP__


void cbarrier_cancel( );
int cbarrier_wait( );
void cbarrier_init( Node num_nodes );
void cbarrier_cancel_local( );


#endif
