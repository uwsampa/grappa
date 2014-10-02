////////////////////////////////////////////////////////////////////////
// This file is part of Grappa, a system for scaling irregular
// applications on commodity clusters. 

// Copyright (C) 2010-2014 University of Washington and Battelle
// Memorial Institute. University of Washington authorizes use of this
// Grappa software.

// Grappa is free software: you can redistribute it and/or modify it
// under the terms of the Affero General Public License as published
// by Affero, Inc., either version 1 of the License, or (at your
// option) any later version.

// Grappa is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// Affero General Public License for more details.

// You should have received a copy of the Affero General Public
// License along with this program. If not, you may obtain one from
// http://www.affero.org/oagl.html.
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
/// Emulates Twitter's Cassovary graph platform
////////////////////////////////////////////////////////////////////////

#include <Grappa.hpp>
#include <random>


DEFINE_bool(metrics, false, "Dump metrics");
DEFINE_string(path, "", "Path to graph source file.");
DEFINE_string(format, "bintsv4", "Format of graph source file.");
DEFINE_int32(scale, 10, "Log2 number of vertices.");

DEFINE_double(reset_probability, 0.3, "Reset probability");
DEFINE_int32(num_steps, 10000, "Steps to take per vertex");
DEFINE_int32(max_depth, 4, "Maximum depth to traverse.");

GRAPPA_DEFINE_METRIC(SimpleMetric<double>, tuple_time, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, construction_time, 0);


float random_float() {
  static std::default_random_engine e(12345L*mycore());
  static std::uniform_real_distribution<float> dist(0, 1);
  return dist(e);
}

long random_index(long max) {
  static std::default_random_engine e(12345L*mycore());
  static std::uniform_int_distribution<long> dist(0, 1L << 48);
  return dist(e) % max;
}

struct CassovaryVertex {};
struct CassovaryEdge {};

using G = Graph<CassovaryVertex,CassovaryEdge>;

struct RandomWalk {
  std::unordered_map<VertexID,int> visits;
  CompletionEvent c;
  
  struct VisitedMap {
    // probably just implement this as a list
  };
  
  RandomWalk(VertexID start, GlobalAddress<G> g, GlobalHashMap<VertexID,> walks) {
    c.enroll(FLAGS_num_steps);
    
    auto walker = make_global(this);
    
    CHECK_EQ((g->vs+start).core(), mycore());
    auto& v = *(g->vs+start).pointer()
    
    for (int i=0; i<FLAGS_num_steps/FLAGS_max_depth; i++) {
      visit(random_index(v.nadj), walker, 1);
    }
    
    c.wait();
  }
  
  // TODO: thread visited verts through this. I'm imagining sending around a list of nodes that have been visited (having it grow as we go, so the first one only sends one VertexID, etc). To do this, we need some easier way to send variable-sized things in delegates.
  static void visit(VertexID target, VisitedMap& visits, 
                    GlobalAddress<RandomWalk> walker, int depth) {
    delegate::call<async,nullptr>(g->vs+target, [=](Vertex& v){
      if (random_float() < FLAGS_reset_probability ||
          depth == FLAGS_max_depth) {
        // go home
        delegate::call<async,nullptr>(walker, [=](RandomWalk& w){
          // merge accumulated visits with what's there
          for (VertexID& v : visits) w.visits[v]++;
          w.c.complete(1);
        });
      } else {
        // visit random
        visit(random_index(v.nadj), walker, depth+1);
      }
    });
  }
  
};


int main(int argc, char* argv[]) {
  init(&argc, &argv);
  run([]{
    
    double t;
    
    DVLOG(0) << "debug mode";
    
    TupleGraph tg;
    
    GRAPPA_TIME_REGION(tuple_time) {
      if (FLAGS_path.empty()) {
        int64_t NE = (1L << FLAGS_scale) * 16;
        tg = TupleGraph::Kronecker(FLAGS_scale, NE, 111, 222);
      } else {
        LOG(INFO) << "loading " << FLAGS_path;
        tg = TupleGraph::Load(FLAGS_path, FLAGS_format);
      }
    }
    LOG(INFO) << tuple_time;
    LOG(INFO) << "constructing graph";
    
    GRAPPA_TIME_REGION()
    
    auto g = G::create(tg, true);
    
    tg.destroy();
    
    
    
    // write out metrics, display some to stdout
    if (FLAGS_metrics) Metrics::merge_and_print();
    else std::cerr << total_time << "\n" << iteration_time << "\n";
    Metrics::merge_and_dump_to_file();
        
    g->destroy();
  });
  finalize();
}
