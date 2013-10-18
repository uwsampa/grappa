#include <set>
#include <vector>
#include <map>

struct Edge {
  int64_t src, dst;
};

class LocalAdjListGraph {
  private:
    std::map<int64_t, std::vector<int64_t>> adjs;
  public:

    LocalAdjListGraph(std::vector<Edge>& edges) : adjs() {
      // assume that the vertex ids are not compressed
      
      for (auto e : edges) {
        auto& val = adjs[e.src];
        val.push_back(e.dst);
      }
    }

    std::vector<int64_t>& neighbors(int64_t root) {
      return adjs[root];
    }

    std::map<int64_t, std::vector<int64_t>>& vertices() {
      return adjs;
    }
};

class LocalMapGraph {
  private:
    std::map<int64_t, std::set<int64_t>> adjs;
  public:

    LocalMapGraph (std::vector<Edge>& edges) : adjs() {
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
    
