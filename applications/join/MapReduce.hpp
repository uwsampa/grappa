#pragma once

#include <Grappa.hpp>
#include <GlobalCompletionEvent.hpp>
#include <Addressing.hpp>
#include <Collective.hpp>

#include <functional>
#include <cstdint>
#include <vector>
#include <unordered_map>


GRAPPA_DECLARE_METRIC(SummarizingMetric<double>, mr_mapping_runtime);
GRAPPA_DECLARE_METRIC(SummarizingMetric<double>, mr_combining_runtime);
GRAPPA_DECLARE_METRIC(SummarizingMetric<double>, mr_reducing_runtime);
GRAPPA_DECLARE_METRIC(SummarizingMetric<double>, mr_reallocation_runtime);

namespace MapReduce {

extern Grappa::GlobalCompletionEvent default_mr_gce;

template <Grappa::GlobalCompletionEvent * GCE, typename RandomAccess, typename AF, typename CF>
void forall_symmetric(GlobalAddress<RandomAccess> vs, AF accessor, CF f ) { 

  Grappa::on_all_cores([=] {
      //VLOG(1) << "size = " << accessor(vs).size();
      auto local_vs = accessor(vs);
      Grappa::forall_here<Grappa::async,GCE>(0, local_vs.size(), [=](int64_t start, int64_t iters) {
        for (int i=start; i<start+iters; i++) {
          auto e = local_vs[i];
          f(e);
        }
      }); // local task blocks for all iterations
     // VLOG(1) << "finished mine";
     });
     // VLOG(1) << "calling wait on me only";
  GCE->wait(); // block until all tasks are done
      //VLOG(1) << "wait done";
}

template <typename K, typename V, typename OutType>
struct Reducer {
  std::unordered_map<K, std::vector<V>> * groups;
  std::vector<OutType> * result;

  Reducer() : groups(new std::unordered_map<K, std::vector<V>>()), result(new std::vector<OutType>()) {}

} GRAPPA_BLOCK_ALIGNED; // using pointers as members because of #157

template <typename K, typename V, typename OutType, Grappa::GlobalCompletionEvent * GCE = &default_mr_gce>
void reducer_append( GlobalAddress<Reducer<K,V,OutType>> r, K key, V val ) {
  Grappa::delegate::call<Grappa::async, GCE>(r.core(), [=] {
    VLOG(5) << "add (" << key << ", " << val << ") at " << r.pointer();
    std::vector<V>& slot = (*(r->groups))[key];
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

template <typename K, typename V>
struct Combiner {
  std::unordered_map<K, std::vector<V>> * groups;

  Combiner() : groups(new std::unordered_map<K, std::vector<V>>()) {}
} GRAPPA_BLOCK_ALIGNED;

template <typename K, typename V, typename OutType>
struct CombiningMapperContext {
  GlobalAddress<Reducer<K,V,OutType>> reducers;
  int64_t num_reducers;
  GlobalAddress<Combiner<K,V>> combining;

  CombiningMapperContext(GlobalAddress<Reducer<K,V,OutType>> reducers, GlobalAddress<Combiner<K,V>> combining, int64_t num_reducers)
    : reducers(reducers)
    , combining(combining)
    , num_reducers(num_reducers) {}

  // called within user map
  void emitIntermediate(K key, V val) const {
    DVLOG(5) << "push key " << key;
    std::vector<V>& slot = (*(combining->groups))[key];
    slot.push_back(val);
  }

  // called within user combine
  template < Grappa::GlobalCompletionEvent * GCE=&default_mr_gce >
  void emitCombinedIntermediate(K key, V val) const {
    auto index = std::hash<K>()(key) % num_reducers;
    auto target = reducers + index;
    DVLOG(5) << "index = " << index;
    reducer_append<GCE>( target, key, val );
  }

  void combineDoneWithKey( K key ) const {
    DVLOG(4) << "clear key " << key;
    (*(combining->groups))[key].clear();
  }
};

template < typename K, typename V, typename OutType, typename CombineF >
void combineExecute(CombiningMapperContext<K,V,OutType> ctx, CombineF combinef) {
  Grappa::on_all_cores([=] {
      auto local = ctx.combining->groups;
      for (auto it=local->begin(); it!=local->end(); ++it) {
        combinef( ctx, it->first, it->second );
        ctx.combineDoneWithKey( it->first );
      }
  });
}
     




template < typename K, typename V, typename OutType >
void emit(Reducer<K,V,OutType>& ctx, OutType result) {
  ctx.result->push_back(result);
}

template < typename T, typename K, typename V, typename OutType, typename MapF, Grappa::GlobalCompletionEvent * GCE=&default_mr_gce > 
void mapExecute(MapperContext<K, V, OutType> ctx, GlobalAddress<T> keyvals, size_t num, MapF mf) {
  Grappa::forall<GCE>(keyvals, num, [=]( T& kv ) {
     mf(ctx, kv);
  });
}  

//TODO get rid of this code duplication
template < typename T, typename K, typename V, typename OutType, typename MapF, Grappa::GlobalCompletionEvent * GCE=&default_mr_gce > 
void mapExecute(CombiningMapperContext<K, V, OutType> ctx, GlobalAddress<T> keyvals, size_t num, MapF mf) {
  Grappa::forall<GCE>(keyvals, num, [=]( T& kv ) {
     mf(ctx, kv);
  });
}  

// takes a symmetric global address
template < typename T, typename K, typename V, typename OutType, typename MapF, typename RA, Grappa::GlobalCompletionEvent * GCE=&default_mr_gce > 
void mapExecute(MapperContext<K, V, OutType> ctx, GlobalAddress<RA> keyvals_sym, MapF mf) {
  forall_symmetric<GCE>(keyvals_sym, [](GlobalAddress<RA> collection) { return collection->data; },
                                     [=]( T& kv ) { 
     mf(ctx, kv);
  });
}  

template < typename K, typename V, typename OutType, typename ReduceF, Grappa::GlobalCompletionEvent * GCE=&default_mr_gce > 
void reduceExecute(GlobalAddress<Reducer<K,V,OutType>> reducers, size_t num, ReduceF rf) {
  Grappa::forall<GCE>(reducers, num, [=]( int64_t i, Reducer<K,V,OutType>& reducer) {
    for ( auto local_it = reducer.groups->begin(); local_it!= reducer.groups->end(); ++local_it ) {
      rf(reducer, local_it->first, local_it->second );
    }
    // deallocate the group
    reducer.groups->clear();
  });
}

template < typename K, typename V, typename OutType >
GlobalAddress<Reducer<K,V,OutType>> allocateReducers( size_t num_reducers ) {
  auto reducers = Grappa::global_alloc<Reducer<K, V, OutType>>( num_reducers );    
  Grappa::forall<&default_mr_gce>(reducers, num_reducers, [](Reducer<K,V,OutType>& r) {
      r = Reducer<K,V,OutType>();
      }); 
  return reducers;
}

template < typename K, typename V >
GlobalAddress<Combiner<K,V>> allocateCombiners() {
    auto combiners = Grappa::symmetric_global_alloc<Combiner<K,V>>();
    Grappa::on_all_cores([=] {
        *(combiners.localize()) = Combiner<K,V>();
        });
    return combiners;
}

// MapRedue with local combiner
// This signature takes pointers to existing reducer/combiner structures so they can be reused
// (global array) reducers
// (symmetric alloc) combiners
template < typename T, typename K, typename V, typename OutType, typename MapF, typename CombineF, typename ReduceF>
void CombiningMapReduceJobExecute(GlobalAddress<T> keyvals, size_t num, GlobalAddress<Reducer<K,V,OutType>> reducers, GlobalAddress<Combiner<K,V>> combiners, size_t num_reducers/*Grappa::cores()*/, MapF mf, CombineF cf, ReduceF rf) {
  auto start_realloc = Grappa::walltime();
  // clear all data structures from a previous usage
  Grappa::forall<&default_mr_gce>(reducers, num_reducers, [](Reducer<K,V,OutType>& r) {
      r.result->clear();
      r.groups->clear();
      }); 
  Grappa::on_all_cores([=] {
      combiners->groups->clear();
  });
  auto stop_realloc = Grappa::walltime();
  mr_reallocation_runtime += stop_realloc - start_realloc;

  auto start_map = stop_realloc;

  CombiningMapperContext<K,V,OutType> ctx(reducers, combiners, num_reducers);
  VLOG(1) << "map";
  mapExecute<T,K,V,OutType,MapF>(ctx, keyvals, num, mf);
  auto stop_map = Grappa::walltime();
  mr_mapping_runtime += stop_map - start_map;

  auto start_combine = stop_map;
  VLOG(1) << "combine/send";
  combineExecute<K,V,OutType>(ctx, cf);
  auto stop_combine = Grappa::walltime();

  mr_combining_runtime += stop_combine - start_combine;

  auto start_reduce = stop_combine;
  VLOG(1) << "reduce";
  reduceExecute<K,V,OutType,ReduceF>(reducers, num_reducers, rf);
  auto stop_reduce = Grappa::walltime(); 

  mr_reducing_runtime += stop_reduce - start_reduce;


  VLOG(1) << "complete";
}
// (global array) reducers
// (symmetric alloc) combiners
template < typename T, typename K, typename V, typename OutType, typename MapF, typename ReduceF>
void MapReduceJobExecute(GlobalAddress<T> keyvals, size_t num, GlobalAddress<Reducer<K,V,OutType>> reducers, size_t num_reducers/*Grappa::cores()*/, MapF mf, ReduceF rf) {
  auto start_realloc = Grappa::walltime();
  // clear all data structures from a previous usage
  Grappa::forall<&default_mr_gce>(reducers, num_reducers, [](Reducer<K,V,OutType>& r) {
      r.result->clear();
      r.groups->clear();
      }); 
  auto stop_realloc = Grappa::walltime();
  mr_reallocation_runtime += stop_realloc - start_realloc;

  auto start_map = stop_realloc;

  MapperContext<K,V,OutType> ctx(reducers, num_reducers);
  VLOG(1) << "map";
  mapExecute<T,K,V,OutType,MapF>(ctx, keyvals, num, mf);
  auto stop_map = Grappa::walltime();
  mr_mapping_runtime += stop_map - start_map;

  auto start_reduce = stop_map;
  VLOG(1) << "reduce";
  reduceExecute<K,V,OutType,ReduceF>(reducers, num_reducers, rf);
  auto stop_reduce = Grappa::walltime(); 

  mr_reducing_runtime += stop_reduce - start_reduce;


  VLOG(1) << "complete";
}


// push-based map reduce
template < typename T, typename K, typename V, typename OutType, typename MapF, typename ReduceF>
GlobalAddress<Reducer<K,V,OutType>> MapReduceJobExecute(GlobalAddress<T> keyvals, size_t num, size_t num_reducers/*Grappa::cores()*/, MapF mf, ReduceF rf) {
  auto reducers = allocateReducers<K,V,OutType>( num_reducers );
  Grappa::forall<&default_mr_gce>(reducers, num_reducers, [](Reducer<K,V,OutType>& r) {
      r = Reducer<K,V,OutType>();
      }); 
  //TODO symmetric alloc of a MapReduceContext GCE to allow concurrent jobs

  mapExecute<T,K,V,OutType,MapF>(MapperContext<K,V,OutType>(reducers, num_reducers), keyvals, num, mf);

  reduceExecute<K,V,OutType,ReduceF>(reducers, num_reducers, rf);

  return reducers;
}

template < typename T, typename K, typename V, typename OutType, typename MapF, typename ReduceF, typename RA>
GlobalAddress<Reducer<K,V,OutType>> MapReduceJobExecute(GlobalAddress<RA> keyvals, size_t num_reducers/*Grappa::cores()*/, MapF mf, ReduceF rf) {
  auto reducers = allocateReducers<K,V,OutType>( num_reducers );
  Grappa::forall<&default_mr_gce>(reducers, num_reducers, [](Reducer<K,V,OutType>& r) {
      r = Reducer<K,V,OutType>();
      }); 
  //TODO symmetric alloc of a MapReduceContext GCE to allow concurrent jobs

  mapExecute<T,K,V,OutType,MapF>(MapperContext<K,V,OutType>(reducers, num_reducers), keyvals, mf);

  reduceExecute<K,V,OutType,ReduceF>(reducers, num_reducers, rf);

  return reducers;
}


} // end namespace

        /// TODO: join: quite annoying might as well do parallel(map/map) reduce because otherwise need to marshal data from two tables into an forall() iterator anyway 
