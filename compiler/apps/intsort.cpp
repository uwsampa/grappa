#include <Grappa.hpp>
#include <map>

#ifdef __GRAPPA_CLANG__
#include <Primitive.hpp>
#endif

using namespace Grappa;

using Key = int;
using Index = int64_t;

struct NPBClass { int log_keys, log_maxkey, log_buckets; };
std::map<char,NPBClass> npb = {
  {'S',{ 16, 11, 10 }},
  {'W',{ 20, 16, 10 }},
  {'A',{ 23, 19, 10 }},
  {'B',{ 25, 21, 10 }},
  {'C',{ 27, 23, 10 }},
  {'D',{ 29, 27, 10 }},
  {'E',{ 31, 27, 10 }}
};

DEFINE_bool( metrics, false, "Dump metrics");
DEFINE_int32(log_keys, 10, "Log2 number of keys to sort");
DEFINE_int32(log_buckets, 10, "log2(buckets_per_core)");
DEFINE_int32(log_maxkey, 64, "log2(maxkey)");
DEFINE_string(cls, "", "Use nkey/maxkey from specific NPB class");
DEFINE_int32(iterations, 10, "Average over this many times");
DEFINE_bool(verify, false, "Do full verify");

size_t nkeys, nbuckets, maxkey;
double my_seed;

#define BSHIFT (FLAGS_log_maxkey - FLAGS_log_buckets)

GRAPPA_DEFINE_METRIC(SummarizingMetric<double>, histogram_time, 0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<double>, allreduce_time, 0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<double>, scatter_time, 0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<double>, local_rank_time, 0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<double>, full_verify_time, 0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<double>, rank_time, 0);


template< typename T, typename Index = int64_t >
class LocalVector {
  T * v;
  Index sz;
  Index nalloc;
public:
  // for 'rank' step (easiest to include it here so it's aligned with the bucket)
  Index * ranks;
  
  LocalVector(): v(nullptr), sz(0), nalloc(0) {}
  ~LocalVector() {
    if (v) { locale_free(v); }
  }
  void reserve(size_t m) {
    if (nalloc < m) {
      if (v) { locale_free(v); }
      nalloc = m;
      v = locale_alloc<T>(nalloc);
    }
  }
  void resize(size_t m) {
    reserve(m);
    sz = m;
  }
  Index size() { return sz; }
        T& operator[](const Index i)       { return v[i]; }
  const T& operator[](const Index i) const { return v[i]; }
  
  void append(T val) {
    v[sz] = val;
    sz++;
    assert(sz <= nalloc);
  }
  
  void clear() { sz = 0; }
  
} GRAPPA_BLOCK_ALIGNED;

using Bucket = LocalVector<Key>;

double randlc(double *X, const double *A);
double find_my_seed(int,int,long,double,double);

void init_seed() {
  my_seed = find_my_seed(Grappa::mycore(), Grappa::cores(), 4*(long)nkeys,
                         314159265.00,    // Random number gen seed
                         1220703125.00 ); // Random number gen mult
}

inline int next_seq_element() {
  const double a = 1220703125.00; // Random number gen mult
  int k = maxkey/4;
  
  double x = randlc(&my_seed, &a);
  x += randlc(&my_seed, &a);
  x += randlc(&my_seed, &a);
  x += randlc(&my_seed, &a);
  
  return k*x;
}

GlobalAddress<Key> key_array;
GlobalAddress<Bucket> bucketlist;

#ifdef __GRAPPA_CLANG__
Key global* keys;
Bucket global* buckets;
#endif

/// Local copy of counts, each core has a copy of the global state
LocalVector<Index> counts, bucket_ranks;

void full_verify() {
  auto sorted_keys = global_alloc<Key>(nkeys);
  memset(sorted_keys, -1, nkeys);
  
  forall<1>(bucketlist, nbuckets, [sorted_keys](int64_t b_id, Bucket& b){
    auto ranks = b.ranks - (b_id<<BSHIFT);
    for (int64_t i=0; i<b.size(); i++) {
      Key k = b[i];
      CHECK_LT(ranks[k], nkeys);
      delegate::write<async>(sorted_keys+ranks[k], k);
      ranks[k]++;
    }
  });
  
  forall(sorted_keys, nkeys-1, [](int64_t i, Key& k){
    Key o = delegate::read(make_linear(&k)+1);
    CHECK_LE(k, o) << "!! bad sort: [" << i << ":" << i+1 << "] = " << k << ", " << o;
  });
  VLOG(1) << "done";
}

void rank(int iteration) {
  VLOG(1) << "iteration " << iteration;
  
  call_on_all_cores([]{
    counts.resize(nbuckets);
    bucket_ranks.resize(nbuckets);
    for (size_t i=0; i<nbuckets; i++) counts[i] = 0;
  });
  
  VLOG(2) << "histogram...";
  GRAPPA_TIME_REGION(histogram_time) {
    // histogram to find out how many fall into each bucket
    forall(key_array, nkeys, [](Key& k){
      counts[ k >> BSHIFT ]++;
    });
  }
  
  VLOG(2) << "allreduce...";
  GRAPPA_TIME_REGION(allreduce_time) {
    on_all_cores([]{
      allreduce_inplace<Index,collective_add>(&counts[0], counts.size());
      // everyone computes prefix sum over buckets locally
      bucket_ranks[0] = 0;
      for (size_t i=1; i<nbuckets; i++) {
        bucket_ranks[i] = bucket_ranks[i-1] + counts[i-1];
      }
    });
  }
  
  // allocate space in buckets
  forall(bucketlist, nbuckets, [](int64_t id, Bucket& b){
    b.clear();
    b.reserve(counts[id]);
  });
  
  VLOG(2) << "scattering...";
  GRAPPA_TIME_REGION(scatter_time) {
    
#ifdef __GRAPPA_CLANG__
    
    forall(key_array, nkeys, [](Key& k){
      auto b = k >> BSHIFT;
      CHECK( b < nbuckets ) << "bucket id = " << b << ", nbuckets = " << nbuckets;
      delegate::call<async>(bucketlist+b, [k](Bucket& b){
        b.append(k);
      });
    });
#else
    // scatter into buckets
    forall(key_array, nkeys, [](Key& k){
      auto b = k >> BSHIFT;
      CHECK( b < nbuckets ) << "bucket id = " << b << ", nbuckets = " << nbuckets;
      delegate::call<async>(bucketlist+b, [k](Bucket& b){
        b.append(k);
      });
    });
#endif
  }
  
  VLOG(2) << "locally ranking...";
  GRAPPA_TIME_REGION(local_rank_time) {
    on_all_cores([iteration]{
      auto bucket_base = bucketlist.localize(),
           bucket_end = (bucketlist+nbuckets).localize();
      
      size_t bucket_range = 1L << BSHIFT;
      
      // for each bucket...
      for (int64_t i=0; i<bucket_end-bucket_base; i++) {
        auto& b = bucket_base[i];
        
        if (b.size() == 0) continue;
        
        b.ranks = new Index[bucket_range];
        for (int64_t j=0; j<bucket_range; j++) b.ranks[j] = 0;
        
        auto b_id = make_linear(&b)-bucketlist;
        CHECK_LT(b_id, nbuckets) << "bucketlist: " << bucketlist;
        size_t min_key = bucket_range * b_id,
        max_key = bucket_range * (b_id+1);
        
        // count number of instances of each key
        auto key_counts = b.ranks - min_key;
        
        auto b_buf = &b[0];
        for (int64_t j=0; j<b.size(); j++) {
          // VLOG(1) << "b_id = " << b_id << ", b_buf[" << j << "] = " << b_buf[j];
          DCHECK_EQ(b_buf[j] >> BSHIFT, b_id);
          DCHECK_GE(b_buf[j], min_key);
          DCHECK_LT(b_buf[j], max_key);
          key_counts[ b_buf[j] ]++;
        }
        
        // prefix_sum the counts to get ranks
        // apparently it's okay for us to not add in the global offsets
        int64_t accum = bucket_ranks[b_id];
        for (int64_t j=0; j<bucket_range; j++) {
          int64_t tmp = b.ranks[j];
          b.ranks[j] = accum;
          accum += tmp;
        }
        DCHECK_EQ(accum-bucket_ranks[b_id], b.size());
      }
    });
  }
  
  if (iteration == 0 && FLAGS_verify) {
    GRAPPA_TIME_REGION(full_verify_time) {
      full_verify();
    }
  }
}

int main(int argc, char* argv[]) {
  Grappa::init(&argc, &argv);
  
  if (FLAGS_cls.size()) {
    auto cls = npb[FLAGS_cls[0]];
    FLAGS_log_keys = cls.log_keys;
    FLAGS_log_maxkey = cls.log_maxkey;
    FLAGS_log_buckets = cls.log_buckets;
  }
  nkeys = 1L << FLAGS_log_keys;
  nbuckets = 1L << FLAGS_log_buckets;
  maxkey = (FLAGS_log_maxkey == 64) ? (std::numeric_limits<Key>::max())
                                    : (1ul << FLAGS_log_maxkey) - 1;
  
  Grappa::run([]{
    double t;

    auto _k = global_alloc<Key>(nkeys);
    auto _b = global_alloc<LocalVector<Key>>(nbuckets);
    call_on_all_cores([=]{
      key_array = _k;
      bucketlist = _b;
      init_seed();
#ifdef __GRAPPA_CLANG__
      keys = as_ptr(key_array);
      buckets = as_ptr(bucketlist);
#endif
    });
    
    // initialize buckets
    forall<1>(bucketlist, nbuckets, [](Bucket& bucket){ new (&bucket) Bucket(); });
    
    //////////////////////////////////////////////////////////////
    // generate random sequence w/ roughly Gaussian distribution
    forall(key_array, nkeys, [](Key& k){ k = next_seq_element(); });
    
    ///////////////////////////////////////////////
    // one free 'rank' iteration to warm things up
    rank(0);
    
    Metrics::reset_all_cores();
    
    t = walltime();
    
    for (int i=0; i<FLAGS_iterations; i++) {
      GRAPPA_TIME_REGION(rank_time) {
        rank(i);
      }
    }
    
    LOG(INFO) << "total_time: " << (walltime()-t);
    
    if (FLAGS_metrics) Metrics::merge_and_print();
    else {
      histogram_time  .pretty(std::cout) << "\n";
      allreduce_time  .pretty(std::cout) << "\n";
      scatter_time    .pretty(std::cout) << "\n";
      local_rank_time .pretty(std::cout) << "\n";
      full_verify_time.pretty(std::cout) << "\n";
      rank_time       .pretty(std::cout) << "\n";
    }
    Metrics::merge_and_dump_to_file();
  });
  Grappa::finalize();
}


/*****************************************************************/
/*************           R  A  N  D  L  C             ************/
/*************                                        ************/
/*************    portable random number generator    ************/
/*****************************************************************/

double  randlc( double *X, const double *A )
{
  static int        KS=0;
  static double R23, R46, T23, T46;
  double        T1, T2, T3, T4;
  double        A1;
  double        A2;
  double        X1;
  double        X2;
  double        Z;
  int           i, j;
  
  if (KS == 0)
  {
    R23 = 1.0;
    R46 = 1.0;
    T23 = 1.0;
    T46 = 1.0;
    
    for (i=1; i<=23; i++)
    {
      R23 = 0.50 * R23;
      T23 = 2.0 * T23;
    }
    for (i=1; i<=46; i++)
    {
      R46 = 0.50 * R46;
      T46 = 2.0 * T46;
    }
    KS = 1;
  }
  
  /*  Break A into two parts such that A = 2^23 * A1 + A2 and set X = N.  */
  
  T1 = R23 * *A;
  j  = T1;
  A1 = j;
  A2 = *A - T23 * A1;
  
  /*  Break X into two parts such that X = 2^23 * X1 + X2, compute
   Z = A1 * X2 + A2 * X1  (mod 2^23), and then
   X = 2^23 * Z + A2 * X2  (mod 2^46).                            */
  
  T1 = R23 * *X;
  j  = T1;
  X1 = j;
  X2 = *X - T23 * X1;
  T1 = A1 * X2 + A2 * X1;
  
  j  = R23 * T1;
  T2 = j;
  Z = T1 - T23 * T2;
  T3 = T23 * Z + A2 * X2;
  j  = R46 * T3;
  T4 = j;
  *X = T3 - T46 * T4;
  return(R46 * *X);
}



/*****************************************************************/
/************   F  I  N  D  _  M  Y  _  S  E  E  D    ************/
/************                                         ************/
/************ returns parallel random number seq seed ************/
/*****************************************************************/

/*
 * Create a random number sequence of total length nn residing
 * on np number of processors.  Each processor will therefore have a
 * subsequence of length nn/np.  This routine returns that random
 * number which is the first random number for the subsequence belonging
 * to processor rank kn, and which is used as seed for proc kn ran # gen.
 */

double   find_my_seed( int  kn,       /* my processor rank, 0<=kn<=num procs */
                      int  np,       /* np = num procs                      */
                      long nn,       /* total num of ran numbers, all procs */
                      double s,      /* Ran num seed, for ex.: 314159265.00 */
                      double a )     /* Ran num gen mult, try 1220703125.00 */
{
  
  long   i;
  
  double t1,t2,t3,an;
  long   mq,nq,kk,ik;
  
  
  
  nq = nn / np;
  
  for( mq=0; nq>1; mq++,nq/=2 )
    ;
  
  t1 = a;
  
  for( i=1; i<=mq; i++ )
    t2 = randlc( &t1, &t1 );
  
  an = t1;
  
  kk = kn;
  t1 = s;
  t2 = an;
  
  for( i=1; i<=100; i++ )
  {
    ik = kk / 2;
    if( 2 * ik !=  kk )
      t3 = randlc( &t1, &t2 );
    if( ik == 0 )
      break;
    t3 = randlc( &t2, &t2 );
    kk = ik;
  }
  
  return( t1 );
  
}

