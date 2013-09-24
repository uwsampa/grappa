
#include <Grappa.hpp>
#include <ParallelLoop.hpp>
#include <GlobalVector.hpp>
#include <Array.hpp>
using namespace Grappa;

DEFINE_uint64(log_vertices, 8, "log2(total number of vertices)");
#define NUM_VERTICES (1L << FLAGS_log_vertices)

DEFINE_uint64(max_children, -1, "max number of children");
DEFINE_uint64(max_color, 10, "'colors' will be in the range [0, max_color)");
DEFINE_uint64(search_color, 0, "which 'color' to look for");

#include <random>
long next_random(long max) {
  static std::default_random_engine e(12345L*mycore());
  static std::uniform_int_distribution<int64_t> dist(0, 1L << 48);
  return dist(e) % max;
}
inline long random_num_children() {
  if (FLAGS_max_children == -1) FLAGS_max_children = NUM_VERTICES / 4;
  return next_random(FLAGS_max_children);
}
inline long random_color() { return next_random(FLAGS_max_color); }

////////////////////////////
// Types
using index_t = uint64_t;
class Vertex;

////////////////////////////
// Globals
GlobalAddress<Vertex> vertex_array;
GlobalAddress<index_t> counter;
GlobalAddress<GlobalVector<index_t>> results;

struct Vertex {
  index_t id;
  index_t num_children;
  index_t first_child;
  
  long color;
  
  void init(index_t index) {
    id = index;
    color = random_color();
    num_children = random_num_children();
    
    first_child = delegate::fetch_and_add(counter, num_children);
    if (first_child >= NUM_VERTICES) {
      num_children = 0;
    } else if (first_child + num_children > NUM_VERTICES) {
      num_children = NUM_VERTICES - first_child;
    }
        
    forall_public_async(first_child, num_children, [](index_t i){
      Vertex child;
      child.init(i);
      delegate::write(vertex_array+i, child);
    });
  }
  
  void print(int depth = 0) {
    std::stringstream s; for (int i=0; i<depth; i++) s << "  ";
    if (num_children == 0) {
      LOG(INFO) << s.str() << "node(id: " << id << ")";
    } else {
      LOG(INFO) << s.str() << "node(id: " << id << ", first_child: " << first_child << ", num_children: " << num_children << "";
      for (int i=0; i<num_children; i++) {
        Vertex v = delegate::read(vertex_array+first_child+i);
        v.print(depth+1);
      }
      LOG(INFO) << s.str() << ")";
    }
  }
};

void create_tree() {
  auto _vertex_array = global_alloc<Vertex>(NUM_VERTICES);
  auto child_count = make_global(new index_t[1]);
  delegate::write(child_count, 1);
  on_all_cores([child_count, _vertex_array]{
    counter = child_count;
    vertex_array = _vertex_array;
  });
  
  auto root = vertex_array+0;
  Vertex v; v.init(0);
  delegate::write(root, v);
  default_gce().wait();
}

void search(index_t vertex_index, long color, bool should_wait = true) {
  Vertex v = delegate::read(vertex_array + vertex_index);
  if (v.color == color) {
    results->push(v.id);
  }
  // search children
  forall_public_async(v.first_child, v.num_children, [color](index_t i){
    search(i, color, false);
  });
  if (should_wait) default_gce().wait();
}

int main( int argc, char * argv[] ) {
  init( &argc, &argv );
  run( [] {
    
    create_tree();
    
    index_t root = 0;
    delegate::read(vertex_array+root).print();
    
    auto results_vector = GlobalVector<index_t>::create(NUM_VERTICES);
    on_all_cores([results_vector]{ results = results_vector; });
    
    search(root, FLAGS_search_color);
    
    // print results
    size_t n = std::max(results->size(), (size_t)10);
    LOG(INFO) << util::array_str("results", results->begin(), n);
    
  } );
  finalize();
  return 0;
}
