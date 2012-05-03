#ifndef __CBARRIER_HPP__
#define __CBARRIER_HPP__

/// Type for Node ID. 
#include <boost/cstdint.hpp>
typedef int16_t Node;

enum cb_cause {
    CB_Cause_Null,
    CB_Cause_Local,
    CB_Cause_Cancel,
    CB_Cause_Done
};

void cbarrier_cancel( );
cb_cause cbarrier_wait( );
void cbarrier_init( Node num_nodes );
void cbarrier_cancel_local( );


#endif
