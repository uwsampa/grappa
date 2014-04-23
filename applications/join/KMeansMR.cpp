#include "MapReduce.hpp"
#include <cmath>
#include <limits>


#define SIZE 8

using namespace Grappa;

template <int Size>
class Vector {
  private:
    float data[Size];
  
  public:
    Vector() { }
    Vector(std::vector<float>& el) {
      for (int i=0; i<Size; i++) {
        data[i] = el[i];
      }
    }
    Vector(float all) {
      for (int i=0; i<Size; i++) {
        data[i] = all;
      }
    }

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
} GRAPPA_BLOCK_ALIGNED;


template <int Size>
std::ostream& operator<<(std::ostream& o, Vector<Size> const& v) {
  return o<<"V(TODO)";
}

typedef int32_t clusterid_t;

template <int Size>
struct Cluster {
  Vector<Size> center;
  clusterid_t id;
};
template <int Size>
std::ostream& operator<<(std::ostream& o, Cluster<Size> const& c) {
  return o << "(" << c.id << ", " << c.center << ")";
}

struct means_aligned {
  std::vector<Vector<SIZE>> means;
} GRAPPA_BLOCK_ALIGNED;
GlobalAddress<means_aligned> means;


template <int Size>
clusterid_t find_cluster(Vector<Size>& p) {
  clusterid_t c = 0;
  int64_t min_dist = LLONG_MAX;
  clusterid_t argmin_idx = -1;
  DVLOG(5) << "find cluster in means of size " << means->means.size();
  for (auto m : means->means) {
    auto cur_dist = p.sq_dist(m);
    if ( cur_dist < min_dist ) {
      min_dist = cur_dist;
      argmin_idx = c;
    }
    ++c;
  }

  return argmin_idx;
} 


template <int Size=SIZE>
void KMeansMap( const MapperContext<clusterid_t,Vector<Size>,Cluster<Size>>& ctx, Vector<Size>& p ) {
  auto closest = find_cluster( p );
  ctx.emitIntermediate( closest, p );
}

template <int Size=SIZE>
void KMeansReduce( Reducer<clusterid_t,Vector<Size>,Cluster<Size>>& ctx, clusterid_t id, std::vector<Vector<Size>> points ) {
  Vector<Size> center(0); 
  for ( auto local_it = points.begin(); local_it!= points.end(); ++local_it ) {
    center += *local_it; 
  }

  center /= points.size();

  Cluster<Size> res = { center, id };

  emit( ctx, res );
}


void kmeans() {
  int K = 2;
  size_t numred = cores();
  
  // read in points
  int numpoints = 100;
  auto points = global_alloc<Vector<SIZE>>(numpoints);
  //TODO
  forall(points, numpoints, [=](int64_t i, Vector<SIZE>& p) {
      float ff = i;
      std::vector<float> v;
      for (int j = 0; j<SIZE; j++) v.push_back(ff+j);
      p = Vector<SIZE>(v);
      });

  // allocate space for current means
  means = symmetric_global_alloc<means_aligned>();
  call_on_all_cores([=] {
      means->means = std::vector<Vector<SIZE>>();
  });

  // pick K of the points to be centers
  std::vector<Vector<SIZE>> initial_means;
  for (int c=0; c<K; c++) {
    // TODO actually randomly pick a center without replacement
    int idx = c;
    initial_means.push_back(delegate::read(points + idx));
  }
  // broadcast initial centers (poor mans broadcast)
  for (auto m : initial_means) {
    call_on_all_cores([=] {
      means->means.push_back(m);
    });
  }
  call_on_all_cores([=] {
    CHECK( means->means.size() == K ) << means->means.size() << " != " << K;
  });

  float tempDist = 1;
  float convergeDist = 0.1;
  uint32_t iter = 0;
  while ( tempDist > convergeDist ) {
    GlobalAddress<Reducer<clusterid_t,Vector<SIZE>,Cluster<SIZE>>> iter_result = 
      MapReduceJobExecute<Vector<SIZE>,clusterid_t,Vector<SIZE>,Cluster<SIZE>>(points, numpoints, numred, &KMeansMap<SIZE>, &KMeansReduce<SIZE>); 

    std::vector<Vector<SIZE>> oldMeans(means->means);

    // send means to all nodes using
    // poor man's all-to-all
    forall(iter_result, numred, [=](Reducer<clusterid_t,Vector<SIZE>,Cluster<SIZE>>& r) {
        for (Cluster<SIZE> clust : *(r.result)) {
          call_on_all_cores([=] {
            DVLOG(4) << "saving to means[" << clust.id << "]";
            means->means[clust.id] = clust.center;
          });
        }        
        r.result->clear();
    });
    global_free(iter_result); 

    // how much did all centers move?
    float newDist = 0.0f;
    for (int i=0; i<means->means.size(); i++) {
      newDist += means->means[i].sq_dist(oldMeans[i]);
    }
    tempDist = newDist;
    LOG(INFO) << "iteration " << iter << ": dist=" << tempDist;

    ++iter;
  }
}

int main(int argc, char** argv) {
  Grappa::init(&argc, &argv);
  Grappa::run([=] {
      kmeans();
  });
  Grappa::finalize();
}
