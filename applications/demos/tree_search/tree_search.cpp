////////////////////////////////////////////////////////////////////////
// Copyright (c) 2010-2015, University of Washington and Battelle
// Memorial Institute.  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//     * Redistributions of source code must retain the above
//       copyright notice, this list of conditions and the following
//       disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials
//       provided with the distribution.
//     * Neither the name of the University of Washington, Battelle
//       Memorial Institute, or the names of their contributors may be
//       used to endorse or promote products derived from this
//       software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// UNIVERSITY OF WASHINGTON OR BATTELLE MEMORIAL INSTITUTE BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
////////////////////////////////////////////////////////////////////////

#include <Grappa.hpp>
#include <GlobalVector.hpp>
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
GlobalAddress< Vertex > vertex_array;
GlobalAddress< index_t > counter;
GlobalAddress< GlobalVector<index_t> > results;

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
        
    forall<unbound,async>(first_child, num_children, [](int64_t i){
      Vertex child;
      child.init(i);
      delegate::write(vertex_array+i, child);
    });
  }
};

void create_tree() {
  auto _vertex_array = global_alloc<Vertex>(NUM_VERTICES);
  auto vertex_count = make_global(new index_t[1]);
  delegate::write(vertex_count, 1);
  on_all_cores([vertex_count, _vertex_array]{
    counter = vertex_count;
    vertex_array = _vertex_array;
  });
  
  auto root = vertex_array+0;
  Vertex v;
  finish([&v]{
    v.init(0);
  });
  delegate::write(root, v);
}

void search(index_t vertex_index, long color) {
  Vertex v = delegate::read(vertex_array + vertex_index);
  if (v.color == color) {
    results->push(v.id);
  }
  // search children
  forall<unbound,async>(v.first_child, v.num_children, [color](int64_t i){
    search(i, color);
  });
}

int main( int argc, char * argv[] ) {
  init( &argc, &argv );
  run([]{
    //////////////////////////////////////////////////////////////////////////
    // vertex_array -> [
    //   (first_child:1,num_children:4,color:3),
    //   (first_child:-1,num_children:0,color:1),
    //   (first_child:5,num_children:1,color:1),
    //   (first_child:6,num_children:1,color:1),
    //   ...
    // ]
    //////////////////////////////////////////////////////////////////////////
    create_tree();
    
    auto results_vector = GlobalVector<index_t>::create(NUM_VERTICES);
    on_all_cores([results_vector]{ results = results_vector; });
    
    {
      index_t root = 0;
      search(root, FLAGS_search_color);
      default_gce().wait();
    }
    
    // print first 10 results
    LOG(INFO) << util::array_str("results (first 10)", results->begin(), 
                                  std::min(results->size(), (size_t)10));
    
  });
  finalize();
  return 0;
}
