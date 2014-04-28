#include "MapReduce.hpp"
#include "relation_io.hpp"
#include <Reducer.hpp>
#include <cmath>
#include <limits>
#include <random>

// Questions: what if the rows are wider and fewer? This comes up in high dimensional feature spaces.
//    then computing closestPoint and the distance need to be parallelized. What is the effort to do this
//    in Spark or Grappa?

#define SIZE 4
#define CORES_NUM_REDUCERS 0
#define NO_MAX_ITERS 0

DEFINE_bool(normalize, false, "Whether to treat all features in the vector as real values in [0,1]");
DEFINE_double(converge_dist, 0.0001, "How far all the means must move in one iteration to consider converged");
DEFINE_string(input, "", "Input file: binary integers of size SIZE");
DEFINE_uint64(num_generate, 100, "Number of points to generate (if --input not specified)");
DEFINE_uint64(k, 2, "Number of clusters");
DEFINE_uint64(numred, CORES_NUM_REDUCERS, "Number of reducers; default = 0 (indicates to use number of cores)");
DEFINE_uint64(maxiters, NO_MAX_ITERS, "Number of max iterations; default = 0 (indicates no maximum)");

GRAPPA_DEFINE_METRIC(SummarizingMetric<double>, iterations_runtime, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, kmeans_runtime, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, pick_centers_runtime, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, ingress_runtime, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, normalize_runtime, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, number_of_points, 0);

// RNG
typedef std::mt19937 RNG;
RNG rng;


using namespace Grappa;


template <int Size>
class Vector {
  private:
  
  public:
    double data[Size];
    Vector() { }
    Vector(std::vector<double>& el) {
      for (int i=0; i<Size; i++) {
        data[i] = el[i];
      }
    }
    Vector(double all) {
      for (int i=0; i<Size; i++) {
        data[i] = all;
      }
    }

    Vector& operator+=(const Vector& rhs) {
        DVLOG(5) << "adding i " << Size;
      for (int i=0; i<Size; i++) {
        this->data[i] += rhs.data[i]; // only segfaults with reduce in -O3: does get exacerbated with a move optimization?
      }

      return *this;
    }
    
    Vector operator+(const Vector& rhs) const {
      Vector v(*this);
      v += rhs;
      return v;
    }

    Vector& operator/=(const double& rhs) {
      for (int i=0; i<Size; i++) {
        this->data[i] /= rhs;
      }
      return *this;
    }

    Vector operator/(const double& rhs) const {
      Vector v(*this);
      v /= rhs;
      return v;
    }

    Vector& operator/=(const Vector& rhs) {
      for (int i=0; i<Size; i++) {
        this->data[i] /= rhs.data[i];
      }
      return *this;
    }

    double sq_dist(const Vector& rhs) const {
      double sumdist = 0;
      for (int i=0; i<Size; i++) {
        sumdist += pow((this->data[i] - rhs.data[i]), 2);
      }
      return sumdist;
    }
    
    template <int S>
    friend std::ostream& operator<<(std::ostream& o, Vector<S> const& v);
} GRAPPA_BLOCK_ALIGNED;
// NOTE: if there is a segfault in here, it is probably the reduction code
// which is only used by normalize


template <int Size>
std::ostream& operator<<(std::ostream& o, Vector<Size> const& v) {
  o << "{";
  for (int i=0; i<Size; i++) {
    o << v.data[i] << ",";
  }
  return o<<"}";
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
  auto min_dist = std::numeric_limits<double>::max();
  clusterid_t argmin_idx = -1;
  DVLOG(5) << "find cluster in means of size " << means->means.size();
  for (auto m : means->means) {
    auto cur_dist = p.sq_dist(m);
    VLOG(4) << "mean " << c << " has dist " << cur_dist << " comparing to min so far " << min_dist;
    if ( cur_dist < min_dist ) {
      min_dist = cur_dist;
      argmin_idx = c;
    }
    ++c;
  }
  
  VLOG(5) << "pick cluster " << argmin_idx << " of " << means->means.size() << " for " << p;

  return argmin_idx;
} 


template <int Size=SIZE>
void KMeansMap( const MapReduce::MapperContext<clusterid_t,Vector<Size>,Cluster<Size>>& ctx, Vector<Size>& p ) {
  auto closest = find_cluster( p );
  ctx.emitIntermediate( closest, p );
}

template <int Size=SIZE>
void KMeansReduce( MapReduce::Reducer<clusterid_t,Vector<Size>,Cluster<Size>>& ctx, clusterid_t id, std::vector<Vector<Size>> points ) {
  Vector<Size> center(0); 
  for ( auto local_it = points.begin(); local_it!= points.end(); ++local_it ) {
    center += *local_it; 
  }

  center /= points.size();

  Cluster<Size> res = { center, id };
  
  VLOG(1) << "cluster " << res << " contains " << points.size() << " points";

  emit( ctx, res );
}

SimpleSymmetric<Vector<SIZE>> normal_reducer;

void kmeans() {
  const uint64_t K = FLAGS_k;
  uint64_t numred = cores();
  if (FLAGS_numred != CORES_NUM_REDUCERS) {
    numred = FLAGS_numred;
  }
  
  double start_ingress = walltime();
  // read in points
  size_t numpoints;
  GlobalAddress<Vector<SIZE>> points;
  if (FLAGS_input.compare("") != 0) {
    numpoints = readTuplesUnordered<Vector<SIZE>>( FLAGS_input, &points, SIZE );
  } else {
    numpoints = FLAGS_num_generate;
    points = global_alloc<Vector<SIZE>>(numpoints);

    // make up points
    forall(points, numpoints, [=](int64_t i, Vector<SIZE>& p) {
        double ff = i;
        std::vector<double> v;
        for (int j = 0; j<SIZE; j++) v.push_back(ff+j);
        p = Vector<SIZE>(v);
      });
  }
  VLOG(1) << numpoints << " points";
  number_of_points = numpoints;
  ingress_runtime = walltime() - start_ingress;

  if (FLAGS_normalize) {
    double start_normalize = walltime();
    VLOG(1) << "normalizing data";

    forall(points, numpoints, [=](Vector<SIZE>& p) {
      normal_reducer += p;
    }); 
    //FIXME: doing the final reduce by hand because reduce() segfaults with Vector
    for (int i=1; i<cores(); i++) {
      normal_reducer += delegate::call(i, []{ return normal_reducer.local(); });
    }
    //auto sumV = sum(&normal_reducer);
    auto sumV = normal_reducer.local();


    VLOG(1) << "sum vector=" << sumV;

    forall(points, numpoints, [=](Vector<SIZE>& p) {
        p = p /= sumV;
     });
    normalize_runtime = walltime() - start_normalize;
  }

  double start_pick = walltime();
  // allocate space for current means
  auto lmeans = symmetric_global_alloc<means_aligned>();
  VLOG(2) << "initializing means struct";
  on_all_cores([lmeans] {
      means = lmeans;
      means->means = std::vector<Vector<SIZE>>();
  });
  VLOG(2) << "picking means";

  // pick K of the points to be centers
  std::vector<Vector<SIZE>> initial_means;
  std::random_device rd;
  rng.seed(mycore() + rd());
  std::uniform_int_distribution<uint32_t> unif_dist(0, numpoints-1);  // FIXME: Need to sample without replacement
  for (int c=0; c<K; c++) {
    // TODO actually randomly pick a center without replacement
    int idx = unif_dist(rng);
    initial_means.push_back(delegate::read(points + idx));
  }
  // broadcast initial centers (poor mans broadcast)
  for (auto m : initial_means) {
    VLOG(1) << "initial center: " << m;
    call_on_all_cores([=] {
      means->means.push_back(m);
    });
  }
  call_on_all_cores([=] {
    CHECK( means->means.size() == K ) << means->means.size() << " != " << K;
  });
  pick_centers_runtime = walltime() - start_pick;

  double start = walltime();

  double tempDist = std::numeric_limits<double>::max();
  uint64_t iter = 0;
  while ( (tempDist > FLAGS_converge_dist) 
      and ((FLAGS_maxiters == NO_MAX_ITERS) or (iter < FLAGS_maxiters)) ) {
    double iter_start = walltime();

    GlobalAddress<MapReduce::Reducer<clusterid_t,Vector<SIZE>,Cluster<SIZE>>> iter_result = 
      MapReduce::MapReduceJobExecute<Vector<SIZE>,clusterid_t,Vector<SIZE>,Cluster<SIZE>>(points, numpoints, numred, &KMeansMap<SIZE>, &KMeansReduce<SIZE>); 

    std::vector<Vector<SIZE>> oldMeans(means->means);

    // send means to all nodes using
    // poor man's all-to-all
    forall(iter_result, numred, [=](MapReduce::Reducer<clusterid_t,Vector<SIZE>,Cluster<SIZE>>& r) {
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
    double newDist = 0.0f;
    for (int i=0; i<means->means.size(); i++) {
      newDist += means->means[i].sq_dist(oldMeans[i]);
    }
    tempDist = newDist;
    LOG(INFO) << "iteration " << iter << ": dist=" << tempDist;

    ++iter;
    double iter_end = walltime();
    iterations_runtime += iter_end - iter_start;
  }
  kmeans_runtime = walltime() - start;
}

int main(int argc, char** argv) {
  Grappa::init(&argc, &argv);
  Grappa::run([=] {
      Grappa::Metrics::reset(); // we are including all the non-iteration stuff in stats (besides timing), in hopes that iterations are the bulk
      kmeans();
      Grappa::Metrics::merge_and_print();
  });
  Grappa::finalize();
}
