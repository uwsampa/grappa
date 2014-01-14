#include "tree.hpp"

int64_t count;

long search_color;

GlobalCompletionEvent gce;

void search(GlobalAddress<Vertex> vertex_addr) {
  // fetch the vertex info
  Vertex v = delegate::read(vertex_addr);
  
  // check the color
  if (v.color == search_color) count++;
  
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
    
    // initialize all cores
    on_all_cores([]{
      search_color = 7; // arbitrary search
      count = 0;
    });
    
    finish<&gce>([=]{
      search( root );
    });
    
    // compute total count
    int64_t total = reduce<int64_t,collective_add<int64_t>>(&count);
    
    LOG(INFO) << "total count: " << total;
    
  });
  finalize();
  return 0;
}
