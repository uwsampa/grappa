
#include <Grappa.hpp>
#include <GlobalVector.hpp>
#include <Array.hpp>
using namespace Grappa;

size_t num_vertices;

DEFINE_uint64(max_children, -1, "max number of children");
DEFINE_uint64(max_color, 10, "'colors' will be in the range [0, max_color)");

#include <random>
long next_random(long max) {
  static std::default_random_engine e(12345L*mycore());
  static std::uniform_int_distribution<int64_t> dist(0, 1L << 48);
  return dist(e) % max;
}
inline long random_num_children() {
  if (FLAGS_max_children == -1) FLAGS_max_children = num_vertices / 4;
  return next_random(FLAGS_max_children);
}
inline long random_color() { return next_random(FLAGS_max_color); }

////////////////////////////
// Types
using index_t = int64_t;
class Vertex;

////////////////////////////
// Globals
GlobalAddress< Vertex > vertex_array;
GlobalAddress< index_t > counter;

struct Vertex {
  index_t id;
  index_t num_children;
  GlobalAddress<Vertex> first_child;
  
  long color;
  
  void init(index_t index) {
    id = index;
    color = random_color();
    num_children = random_num_children();
    
    auto first_child_index = delegate::fetch_and_add(counter, num_children);
    first_child = vertex_array + first_child_index;
    
    if (first_child_index >= num_vertices) {
      num_children = 0;
    } else if (first_child_index + num_children > num_vertices) {
      num_children = num_vertices - first_child_index;
    }
        
    forall<async>(first_child, num_children, [](int64_t i, Vertex& child){
      child.init(i);
    });
  }
};

GlobalAddress<Vertex> create_tree(size_t _num_vertices) {
  auto _vertex_array = global_alloc<Vertex>(_num_vertices);
  auto vertex_count = make_global(new index_t[1]);
  delegate::write(vertex_count, 1);
  on_all_cores([vertex_count, _vertex_array, _num_vertices]{
    counter = vertex_count;
    vertex_array = _vertex_array;
    num_vertices = _num_vertices;
  });
  
  auto root = vertex_array+0;
  Vertex v;
  finish([&v]{
    v.init(0);
  });
  delegate::write(root, v);
  
  return _vertex_array;
}
