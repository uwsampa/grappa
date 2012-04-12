#include "uts.h"

#include "SoftXMT.hpp"
#include "Tasking.hpp"
#include "Cache.hpp"

int64_t global_id = 0;
int64_t global_child_index = 0;
GlobalAddress<int64_t> global_id_ga;
GlobalAddress<int64_t> global_child_index_ga;

// shared tree structures
GlobalAddress<int64_t> NumChildren;
GlobalAddress<int64_t> ChildIndex;
GlobalAddress<int64_t> Child;
GlobalAddress<int64_t> Payload;

// Initialization on each node
struct init_args {
    GlobalAddress<Semaphore> sem;
};

void init_node( init_args * ) {
    global_id_ga = make_global( &global_id, 0 );
    global_child_index_ga = make_global( &global_child_index, 0 );

    Semaphore::release(&args->sem, 1);
}
//////////////////////////////////


struct Result {
  int64_t maxdepth, size, leaves;
};

struct vertex_args {
    GlobalAddress<TreeNode> addr;
    int64_t inSum;
};
vertex_args * local_args;
int64_t args_index;

void visitVertex( vertex_args * args ) {
    GlobalAddress<TreeNode> node_address = args->addr;
    int64_t inSum = args->inSum;

    // get the vertex
    TreeNode workLocal_storage;
    Incoherent<TreeNode>::RO workLocal( node_address, 1, &workLocal_storage );  
   
    uint64_t numChildren = (*workLocal).num_children;

    // get the edge list
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

    // release the cached objects
    childrenPtrs.start_release();
    workLocal.start_release();
    
    childrenPtrs.block_until_released();
    workLocal.block_until_released();
}

void user_main ( user_main_args * args ) {
    
    NumChildren = SoftXMT_typed_malloc<int64_t>( num_tree_nodes);
    ChildIndex =  SoftXMT_typed_malloc<int64_t>( num_tree_nodes);
    Child =  SoftXMT_typed_malloc<int64_t>( num_tree_nodes);
    Payload =  SoftXMT_typed_malloc<int64_t>( num_tree_nodes);

    // initialization on each node
    Semaphore init_sem( SoftXMT_nodes(), 0 );
    for (Node no=0; no<SoftXMT_nodes(); no++) {
        SoftXMT
    

    uint64_t gen_startTime, gen_endTime, search_startTime, search_endTime;
    
    gen_startTime = uts_wctime();

    // spawn the task to visit root of tree
    int64_t rootInd = args_index++;
    args_index[rootInd] = { nodes, 0 };
    SoftXMT_publicTask( &visitVertex, &args_index[rootInd] );



    gen_endTime = uts_wctime();





    SoftXMT_free( nodes );
    SoftXMT_free( children_arrays );
}

void create_children( int i, siblings_args * sv ) {
    TreeNode child;
    child.type = s->childType;
    child.height = s->parentHeight + 1;
    child.numChildren = -1; // not yet determined
    child.id = s->childid0 + i;
    for (int j=0; j<computeGranularity; j++) {
        rng_spawn(s->parent->state.state, child.state.state, i);
    }
    Result c = parTreeCreate(s->depth+1, &child);
}

Result parTreeCreate( int depth, TreeNode * parent ) {
    int numChildren, childType;
    
    Result r = { depth, 1, 0 };
    numChildren = uts_numChildren( parent );
    childType = uts_childType( parent );

    /*** Added for the sake of remembering the tree created: ***/
    int id = uts_nodeId(parent);
    /* Record the number of children: */
    SoftXMT_delegate_write_word( NumChildren + id, numChildren );
    /* Assign fresh unique ids for the children: */
    int childid0 = SoftXMT_delegate_fetch_and_add( &global_id, numChildren );
    /* Record ids and indices of the children: */ 
    int index = SoftXMT_delegate_fetch_and_add(&global_child_index, numChildren);
    SoftXMT_delegate_write_word( ChildIndex + id, index );

    // Write out child indexes as one block
    // Incoherent acquire is fine because this section of Child is reserved
    int childIds_storage[numChildren];
    Incoherent<int>::RW childIds( Child + index, numChildren, childIds_storage );

    for (int i = 0; i < numChildren; i++) {
        childIds[i] = childid0 + i;
    }
    childIds.block_until_released();

    // record number of children in parent
    SoftXMT_delegate_write_word( parent->numChildren, numChildren );
    subtreesize = numChildren;

    // Recurse on the children
    if (numChildren > 0) {
        sibling_args args = {childType, parentHeight, parent, depth, &r, childid0 };
        //TODO parallel_loop(0, numChildren, create_children, &args);
        for (int i=0; i < numChildren; i++) {
            create_children(i, &args);
        }
    } else {
        r.leaves = 1;
    }

    return r;
}


   
size_t local_size_bytes = 1<<32; 
int main (int argc, char** argv) {
    SoftXMT_init( argc, argv, local_size_bytes*2 ); //TODO find num nodes (not 2)...
    SoftXMT_activate();

    // set up args structure; TODO: move to fork joined tasks
    local_args = new vertex_args[num_tree_nodes];
    args_index = 0;

    SoftXMT_run_user_main( &user_main, NULL );
    CHECK( SoftXMT_done() == true ) << "SoftXMT not done before scheduler exit";
    SoftXMT_finish( 0 );
}
