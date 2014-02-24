#include "tree.hpp"

// pointer to the results vector
GlobalAddress<GlobalVector<index_t>> results;

long search_color;

GlobalCompletionEvent gce;

void search(GlobalAddress<Vertex> vertex_addr) {
  // fetch the vertex info
  Vertex v = delegate::read(vertex_addr);
  
  // check the color
  if (v.color == search_color) {
    // save the id to the results vector
    results->push( v.id );
  }
  
  // search children
  auto children = v.first_child;
  forall_here<unbound,async,&gce>(0, v.num_children, [children](int64_t i){
    search(children+i);
  });
}

int main( int argc, char * argv[] ) {
  init( &argc, &argv );
  run([]{
    
    size_t num_vertices = 1000;
    
    GlobalAddress<Vertex> root = create_tree(num_vertices);
    
    GlobalAddress<GlobalVector<index_t>> rv = GlobalVector<index_t>::create(num_vertices);
    
    // initialize all cores
    on_all_cores([=]{
      search_color = 7; // arbitrary search
      results = rv;
    });
    
    finish<&gce>([=]{
      search( root );
    });
    
    // print id's of the first 10 results
    LOG(INFO) << "found " << results->size() << " matches";
    LOG(INFO) << util::array_str("first 10 results", results->begin(), 
                                  std::min(results->size(), (size_t)10));
    
  });
  finalize();
  return 0;
}
