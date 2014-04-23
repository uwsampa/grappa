#include "MapReduce.hpp"
#include <cmath>


#define SIZE 10

using namespace Grappa;

template <typename Size>
class Vector {
  float[Size] data;
  
  public:
    Vector& operator+=(const Vector& rhs) {
      for (int i=0; i<Size; i++) {
        this->data[i] += rhs.data[i];
      }
      return *this;
    }
    
    Vector operator+(const Vector& rhs) const {
      Vector v;
      for (int i=0; i<Size; i++) {
        v[i] = this->data[i] + rhs.data[i];
      }
      return v;
    }

    Vector& operator/=(const float& rhs) {
      for (int i=0; i<Size; i++) {
        this->data[i] /= rhs;
      }
      return *this;
    }

    float sq_dist(const Vector& rhs) const {
      float sumdist = 0;
      for (int i=0; i<Size; i++) {
        sumdist += pow((this->data[i] - rhs.data[i]), 2);
      }
      return sumdist;
    }
};

typedef int32_t clusterid_t;

template <typename Size>
struct Cluster {
  Vector<Size> center;
  clusterid_t id;
};

struct means_aligned {
  std::vector<Vector<SIZE>> means;
} GRAPPA_BLOCK_ALIGNED;
GlobalAddress<means_aligned> means;


template <typename Size>
clusterid_t find_cluster(Vector<Size>& p) {
  clusterid_t = 0;
  for (auto m : means.localize()) {
    int64_t min_dist;
    clusterid_t min_idx;
    auto cur_dist = p.sq_dist(means[clusterid_t]);
    if ( cur_dist < min_dist ) {
      min_dist = cur_dist;
      min_idx = clusterid_t;
    }
  }

  return min_idx;
} 


template <typename Size=SIZE>
void KMeansMap( const MapperContext<clusterid_t,Vector<SIZE>,Cluster>& ctx, Vector& p ) {
  auto closest = find_cluster( p );
  ctx.emitIntermediate( closest, p );
}

template <typename Size=SIZE>
void KMeansReduce( Reducer<clusterid_t,Vector<Size>,Cluster<Size>>& ctx, clusterid_t id, std::vector<Vector<Size>> points ) {
  Vector<Size> center = 0; 
  for ( auto local_it = points.begin(); local_it!= points.end(); ++local_it ) {
    center += *local_it; 
  }

  center /= points.size();

  Cluster<Size> res = { center, id };

  emit( ctx, res );
}

GlobalCompletionEvent broadcast_gce;

void kmeans() {
  int K = 10;
  int numred = cores();
  
  // read in points
  auto points = global_alloc<Point<SIZE>>(100);
  //TODO

  // allocate space for current means
  means = symmetric_global_alloc<Vector<SIZE>*>(means);
  on_all_cores([=] {
      means->means = std::vector<Vector<SIZE>>>();
  });


  float tempDist = 1f;
  float convergeDist = 0.1f;
  while ( tempDist > convergeDist ) {
    GlobalAddress<Reducer<clusterid_t,Vector<SIZE>,Cluster<SIZE>> iter_result = 
      MapReduceJobExecute(points, numred, &KMeansMap, &KMeansReduce); 

    std::vector<Vector<SIZE>> oldMeans(means->means);

    // send means to all nodes using
    // poor man's all-to-all
    forall<&broadcast_gce>(iter_result, numred, [=](Cluster<Size> e) {
        call_on_all_cores([=] {
          means->means[e.id] = e.center;
        });
    })

    // how much did all centers move?
    float newDist = 0.0f;
    for (int i=0; i<means->means.size(); i++) {
      newDist += means->means[i].sq_dist(oldMeans[i]);
    }
    tempDist = newDist;
  }
}
