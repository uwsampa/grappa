#include <cassert>
#include "Collective.hpp"
#include "Delegate.hpp"

// Delegates implementation of reductions.
// Only one reduction can happen at a time.

#define _MAX_PROC_NODES 128

// relies on static address being the same all binaries
int64_t _col_values[_MAX_PROC_NODES];
int64_t _col_done_count;
bool _col_allowed = true;

Thread * reducing_thread;
int64_t reduction_result;
Node reduction_reported_in = 0;

/// Reduction involving all SoftXMT Nodes.
/// For now involves a barrier, so only one
/// collective op is allowed at a time.
int64_t SoftXMT_collective_reduce( int64_t (*commutative_func)(int64_t, int64_t), Node home_node, int64_t myValue, int64_t initial ) {
    assert(_col_allowed);
    _col_allowed = false;

    Node myNode = SoftXMT_mynode();
    if (myNode == home_node) {
        const uint64_t num_nodes = SoftXMT_nodes();
        assert( num_nodes <= _MAX_PROC_NODES );

        _col_done_count = 0;  

        // TODO method without barrier (or reducer thread-only barrier)
        VLOG(5) << "home enters collective barrier";
        SoftXMT_barrier_commsafe();
        VLOG(5) << SoftXMT_mynode() << " exits collective barrier";

        _col_values[home_node] = myValue;
        VLOG(5) << "home enters collective waiting";
        while (true) {
            SoftXMT_yield();
            if (_col_done_count==num_nodes-1) {
                break;
            }
        }

        // perform the reduction
        int64_t sofar = initial;
        for (int i=0; i<num_nodes; i++) {
           sofar = (*commutative_func)(sofar, _col_values[i]);
        }
        
        _col_allowed = true;
        return sofar;
    } else {
        GlobalAddress<int64_t> done_adr = GlobalAddress<int64_t>::TwoDimensional(&_col_done_count, home_node);
        GlobalAddress<int64_t> value_adr = GlobalAddress<int64_t>::TwoDimensional(&_col_values[myNode], home_node);

        // TODO no barrier
        VLOG(5) << SoftXMT_mynode() << " enters collective barrier";
        SoftXMT_barrier_commsafe();
        VLOG(5) << SoftXMT_mynode() << " exits collective barrier";

        SoftXMT_delegate_write_word( value_adr, myValue );
        /* implicit fence since write is blocking */
        SoftXMT_delegate_fetch_and_add_word( done_adr, 1);
        
        _col_allowed = true;
        return 0;
    }
}

int64_t collective_max(int64_t a, int64_t b) { return (a>b) ? a : b; }
int64_t collective_min(int64_t a, int64_t b) { return (a<b) ? a : b; }
int64_t collective_add(int64_t a, int64_t b) { return a+b; }
int64_t collective_mult(int64_t a, int64_t b) { return a*b; }
