#include <vector>
#include <unordered_set>

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

bool operator==(const Edge& e1, const Edge& e2) {
  return e1.src==e2.src && e1.dst==e2.dst;
}
struct Edge_hasher {
  std::size_t operator()(const Edge& e) const {
    return (0xFFFFffff & e.src) | ((0xFFFFffff & e.dst)<<32);
  }
};

class LocalAdjListGraph {
  private:
    VertexToAdjMap adjs;
  public:

    LocalAdjListGraph(std::vector<Edge>& edges) : adjs() {
      // assume that the vertex ids are not compressed
      
      for (auto e : edges) {
        auto& val = adjs[e.src];
        val.push_back(e.dst);
      }
    }
    
    LocalAdjListGraph(std::unordered_set<Edge, Edge_hasher>& edges) : adjs() {
      // assume that the vertex ids are not compressed
      
      for (auto e : edges) {
        auto& val = adjs[e.src];
        val.push_back(e.dst);
      }
    }

    std::vector<int64_t>& neighbors(int64_t root) {
      return adjs[root];
    }

    VertexToAdjMap& vertices() {
      return adjs;
    }
};

class LocalMapGraph {
  private:
    VertexToAdjSet adjs;
  public:

    LocalMapGraph (std::vector<Edge>& edges) : adjs() {
      // assume that the vertex ids are not compressed

      for (auto e : edges) {
        auto& val = adjs[e.src];
        val.insert(e.dst);
      }
    }
    
    LocalMapGraph (std::unordered_set<Edge, Edge_hasher>& edges) : adjs() {
      // assume that the vertex ids are not compressed

      for (auto e : edges) {
        auto& val = adjs[e.src];
        val.insert(e.dst);
      }
    }

    bool inNeighborhood(int64_t root, int64_t queried) {
      auto& s = adjs[root];
      return s.find(queried) != s.end();
    }

    int64_t nadj(int64_t root) {
      return adjs[root].size();
    }
};
    
