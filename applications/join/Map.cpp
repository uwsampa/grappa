#include <Grappa.hpp>
#include <GlobalCompletionEvent.hpp>
#include <Addressing.hpp>

#include <functional>
#include <cstdint>
#include <vector>
#include <unordered_map>


//extern GlobalCompletionEvent default_mr_gce;
//TODO use cpp
Grappa::GlobalCompletionEvent default_mr_gce;


template <typename K, typename V, typename OutType>
struct Reducer {
  std::unordered_map<K, std::vector<V>> groups;
  std::vector<OutType> result;

  Reducer() : groups(), result() {}
} GRAPPA_BLOCK_ALIGNED; // FIXME: blocksize < sizeof(T) should be asserted bug for loops

template <typename K, typename V, typename OutType, Grappa::GlobalCompletionEvent * GCE = &default_mr_gce>
void reducer_append( GlobalAddress<Reducer<K,V,OutType>> r, K key, V val ) {
  Grappa::delegate::call<async, GCE>(r.core(), [=] {
    VLOG(5) << "add (" << key << ", " << val << ") at " << r.pointer();
    std::vector<V>& slot = r->groups[key];
    slot.push_back(val);
  });
}
// Overload for GCE specified
template <Grappa::GlobalCompletionEvent * GCE, typename K, typename V, typename OutType>
void reducer_append( GlobalAddress<Reducer<K,V,OutType>> r, K key, V val ) {
  reducer_append<K,V,OutType,GCE>(r, key, val);
}

template <typename K, typename V, typename OutType>
struct MapperContext {
  GlobalAddress<Reducer<K,V,OutType>> reducers;
  int64_t num_reducers;

  MapperContext(GlobalAddress<Reducer<K,V,OutType>> reducers, int64_t num_reducers)
    : reducers(reducers)
    , num_reducers(num_reducers) {}

  template < Grappa::GlobalCompletionEvent * GCE=&default_mr_gce >
  void emitIntermediate(K key, V val) const {
    auto index = std::hash<K>()(key) % num_reducers;
    auto target = reducers + index;
    VLOG(5) << "index = " << index;
    reducer_append<GCE>( target, key, val );
  }
};


template < typename K, typename V, typename OutType >
void emit(Reducer<K,V,OutType>& ctx, OutType result) {
  ctx.result.push_back(result);
}

template < typename T, typename K, typename V, typename OutType, typename MapF, Grappa::GlobalCompletionEvent * GCE=&default_mr_gce > 
void mapExecute(MapperContext<K, V, OutType> ctx, GlobalAddress<T> keyvals, size_t num, MapF mf) {
  Grappa::forall<GCE>(keyvals, num, [=]( int64_t i, T& kv ) {
     mf(ctx, kv);
  });
}  

template < typename K, typename V, typename OutType, typename ReduceF, Grappa::GlobalCompletionEvent * GCE=&default_mr_gce > 
void reduceExecute(GlobalAddress<Reducer<K,V,OutType>> reducers, size_t num, ReduceF rf) {
  Grappa::forall<GCE>(reducers, num, [=]( int64_t i, Reducer<K,V,OutType>& reducer) {
    for ( auto local_it = reducer.groups.begin(); local_it!= reducer.groups.end(); ++local_it ) {
      rf(reducer, local_it->first, local_it->second );
    }
    // deallocate the group
    reducer.groups.clear();
  });
}


// push-based map reduce
template < typename T, typename K, typename V, typename OutType, typename MapF, typename ReduceF>
GlobalAddress<Reducer<K,V,OutType>> MapReduceJobExecute(GlobalAddress<T> keyvals, size_t num, size_t num_reducers/*Grappa::cores()*/, MapF mf, ReduceF rf) {
  auto reducers = Grappa::global_alloc<Reducer<K, V, OutType>>( num_reducers );    // MALLOC NOT NEW!!!
  Grappa::forall<&default_mr_gce>(reducers, num_reducers, [](Reducer<K,V,OutType>& r) {
      r = Reducer<K,V,OutType>();
      for (int i=0; i<16; i++) {
      VLOG(5) << r.groups[i].size();
      }
      });
  //TODO symmetric alloc of a MapReduceContext to allow concurrent jobs

  mapExecute<T,K,V,OutType,MapF>(MapperContext<K,V,OutType>(reducers, num_reducers), keyvals, num, mf);

  reduceExecute<K,V,OutType,ReduceF>(reducers, num_reducers, rf);

  return reducers;
}


struct WordCount {
  int64_t word;
  int64_t count;
  WordCount(int64_t w, int64_t c) : word(w), count(c) {}
};

void NumCountMap( const MapperContext<int64_t,int64_t,WordCount>& ctx, int64_t word ) {
  ctx.emitIntermediate( word, 1 );
}

//template < class IntIterable >
//void NumCountReduce( const Reducer<int64_t,int64_t,WordCount>& ctx, int64_t word, IntIterable counts ) {
void NumCountReduce( Reducer<int64_t,int64_t,WordCount>& ctx, int64_t word, std::vector<int64_t> counts ) {
    int64_t sum = 0; 
    for ( auto local_it = counts.begin(); local_it!= counts.end(); ++local_it ) {
      sum += *local_it; 
    }
    emit( ctx, WordCount(word, sum) );
}

int main(int argc, char** argv) {
  Grappa::init(&argc, &argv);
  Grappa::run([=] {
    size_t numw = 20;
    size_t numred = 2*Grappa::cores();
    GlobalAddress<int64_t> words = Grappa::global_alloc<int64_t>(numw);
    Grappa::forall(words, numw, [=](int64_t i, int64_t& w) {
      w = i%numred;
    });

    GlobalAddress<Reducer<int64_t,int64_t,WordCount>> reds = MapReduceJobExecute<int64_t, int64_t, int64_t, WordCount, decltype(NumCountMap), decltype(NumCountReduce)>(words, numw, numred, &NumCountMap, &NumCountReduce); 

    Grappa::forall(reds, numred, [=](int64_t i, Reducer<int64_t, int64_t, WordCount>& r) {
      LOG(INFO) << "Reducer " << i << " has " << r.result.end() - r.result.begin() << " keys";
      for ( auto local_it = r.result.begin(); local_it!= r.result.end(); ++local_it ) {
        LOG(INFO) << "Reducer " << i << " (" << local_it->word << ", " << local_it->count << ")";
      }
    });
  });
  Grappa::finalize();
}


        /// TODO: join: quite annoying might as well do parallel(map/map) reduce because otherwise need to marshal data from two tables into an forall() iterator anyway 
