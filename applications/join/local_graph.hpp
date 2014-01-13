#pragma once

#include <vector>
#include <unordered_set>
#include <cstdint>
#include <iostream>

#if 1
#include <unordered_set>
#include <unordered_map>
typedef std::unordered_set<int64_t> AdjSet;
typedef std::unordered_map<int64_t, std::vector<int64_t>> VertexToAdjMap;
typedef std::unordered_map<int64_t, AdjSet> VertexToAdjSet;
#else
#include <set>
#include <map>
typedef std::set<int64_t> AdjSet;
typedef std::map<int64_t, std::vector<int64_t>> VertexToAdjMap;
typedef std::map<int64_t, AdjSet> VertexToAdjSet;
#endif

struct Edge {
  int64_t src, dst;
  Edge(int64_t src, int64_t dst) : src(src), dst(dst) {}
  // for construction by sets
  Edge() : src(-1), dst(-1) {}
};

std::ostream& operator<<(std::ostream& o, const Edge& e);
bool operator==(const Edge& e1, const Edge& e2);

struct Edge_hasher {
  std::size_t operator()(const Edge& e) const;
};

class LocalAdjListGraph {
  private:
    VertexToAdjMap adjs;
  public:
    LocalAdjListGraph(std::vector<Edge>& edges);
    LocalAdjListGraph(std::unordered_set<Edge, Edge_hasher>& edges);

    std::vector<int64_t>& neighbors(int64_t root);
    VertexToAdjMap& vertices();
};

class LocalMapGraph {
  private:
    VertexToAdjSet adjs;
  public:

    LocalMapGraph (std::vector<Edge>& edges);
    
    LocalMapGraph (std::unordered_set<Edge, Edge_hasher>& edges);

    bool inNeighborhood(int64_t root, int64_t queried);

    int64_t nadj(int64_t root);
};
    
