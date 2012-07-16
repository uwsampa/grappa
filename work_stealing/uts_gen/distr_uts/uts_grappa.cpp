#include "uts.h"

#include "SoftXMT.hpp"
#include "Tasking.hpp"
#include "Cache.hpp"

int main (int argc, char** argv) {
    SoftXMT_init( argc, argv );
    SoftXMT_activate();

    // set up args structure; TODO: move to fork joined tasks
    local_args = new vertex_args[num_tree_nodes];
    args_index = 0;

    SoftXMT_run_user_main( &user_main, NULL );
    CHECK( SoftXMT_done() == true ) << "SoftXMT not done before scheduler exit";
    SoftXMT_finish( 0 );
}

struct vertex_args {
    GlobalAddress<TreeNode> addr;
    int64_t inSum;
};
vertex_args * local_args;
int64_t args_index;

void visitVertex( vertex_args * args ) {
    GlobalAddress<TreeNode> node_address = args->addr;
    int64_t inSum = args->inSum;

    TreeNode workLocal_storage;
    Incoherent<TreeNode>::RO workLocal( node_address, 1, &workLocal_storage );  
   
    uint64_t numChildren = (*workLocal).num_children;

    TreeNode childrenPtrs_storage[numChildren];
    Incoherent<TreeNode>::RO childrenPtrs( (*workLocal).children, numChildren, childrenPtrs_storage );

    //TODO grab and sum an array
    int64_t sum = inSum+1;

    // TODO parallel_loop
    for (int i=0; i<numChildren; i++) {
        int64_t ci = args_index++;
        local_args[ci] = { childrenPtrs[i], sum };
        SoftXMT_spawnPublic( &visitVertex, sum );
    }

}

void user_main ( Thread * me, void * args ) {
    
    GlobalAddress<TreeNode> nodes = SoftXMT_typed_malloc<TreeNode>( num_tree_nodes * 1.1 );
    GlobalAddress<GlobalAddress<TreeNode>> children_arrays = SoftXMT_typed_malloc<GlobalAddress<TreeNode>( num_tree_nodes * 1.1 );


    uint64_t startTime, endTime;
    
    startTime = uts_wctime();

    // spawn the task to visit root of tree
    int64_t rootInd = args_index++;
    args_index[rootInd] = { nodes, 0 };
    SoftXMT_publicTask( &visitVertex, &args_index[rootInd] );


    // wait for traversal to finish
    SoftXMT_waitForTasks( );

    endTime = uts_wctime();



    SoftXMT_signal_done( );
}


    
int64_t generateTree;
   // get unique id
   // permute
   // store nc     
   
 
