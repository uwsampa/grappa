#include "local_graph.hpp"
#include <glog/logging.h>


std::ostream& operator<<(std::ostream& o, const Edge& e) {
  return o << "("<<e.src<<","<<e.dst<<")";
}

bool operator==(const Edge& e1, const Edge& e2) {
  return e1.src==e2.src && e1.dst==e2.dst;
}
  



std::size_t Edge_hasher::operator()(const Edge& e) const {
  return (0xFFFFffff & e.src) | ((0xFFFFffff & e.dst)<<32);
}
    

LocalAdjListGraph::LocalAdjListGraph(std::vector<Edge>& edges) : adjs() {
  // assume that the vertex ids are not compressed

  DVLOG(5) << "local construction: ";
  for (auto e : edges) {
    DVLOG(5) << "  " << e;
    auto& val = adjs[e.src];
    val.push_back(e.dst);
  }
}
    
LocalAdjListGraph::LocalAdjListGraph(std::unordered_set<Edge, Edge_hasher>& edges) : adjs() {
  // assume that the vertex ids are not compressed

  DVLOG(5) << "local construction: ";
  for (auto e : edges) {
    DVLOG(5) << "  " << e;
    VLOG_EVERY_N(4, 100000) << "edges: " << google::COUNTER;
    auto& val = adjs[e.src];
    val.push_back(e.dst);
  }
}
    

std::vector<int64_t>& LocalAdjListGraph::neighbors(int64_t root) {
  return adjs[root];
}

VertexToAdjMap& LocalAdjListGraph::vertices() {
  return adjs;
}
    


LocalMapGraph::LocalMapGraph (std::vector<Edge>& edges) : adjs() {
  // assume that the vertex ids are not compressed

  DVLOG(5) << "local construction: ";
  for (auto e : edges) {
    DVLOG(5) << "  " << e;
    auto& val = adjs[e.src];
    val.insert(e.dst);
  }
}

LocalMapGraph::LocalMapGraph (std::unordered_set<Edge, Edge_hasher>& edges) : adjs() {
  // assume that the vertex ids are not compressed

  DVLOG(5) << "local construction: ";
  for (auto e : edges) {
    DVLOG(5) << "  " << e;
    auto& val = adjs[e.src];
    val.insert(e.dst);
  }
}

bool LocalMapGraph::inNeighborhood(int64_t root, int64_t queried) {
  auto& s = adjs[root];
  return s.find(queried) != s.end();
}

int64_t LocalMapGraph::nadj(int64_t root) {
  return adjs[root].size();
}
