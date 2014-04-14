#pragma once

#include <Grappa.hpp>
#include <GlobalCompletionEvent.hpp>
#include <Addressing.hpp>
#include <Collective.hpp>

#include <functional>
#include <cstdint>
#include <vector>
#include <unordered_map>


extern Grappa::GlobalCompletionEvent default_mr_gce;


template <typename K, typename V, typename OutType>
struct Reducer {
  std::unordered_map<K, std::vector<V>> * groups;
  std::vector<OutType> * result;

  Reducer() : groups(new std::unordered_map<K, std::vector<V>>()), result(new std::vector<OutType>()) {}

} GRAPPA_BLOCK_ALIGNED; // using pointers as members because of #157

template <typename K, typename V, typename OutType, Grappa::GlobalCompletionEvent * GCE = &default_mr_gce>
void reducer_append( GlobalAddress<Reducer<K,V,OutType>> r, K key, V val ) {
  Grappa::delegate::call<async, GCE>(r.core(), [=] {
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


template < typename K, typename V, typename OutType >
void emit(Reducer<K,V,OutType>& ctx, OutType result) {
  ctx.result->push_back(result);
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
    for ( auto local_it = reducer.groups->begin(); local_it!= reducer.groups->end(); ++local_it ) {
      rf(reducer, local_it->first, local_it->second );
    }
    // deallocate the group
    reducer.groups->clear();
  });
}


// push-based map reduce
template < typename T, typename K, typename V, typename OutType, typename MapF, typename ReduceF>
GlobalAddress<Reducer<K,V,OutType>> MapReduceJobExecute(GlobalAddress<T> keyvals, size_t num, size_t num_reducers/*Grappa::cores()*/, MapF mf, ReduceF rf) {
  auto reducers = Grappa::global_alloc<Reducer<K, V, OutType>>( num_reducers );    
  Grappa::forall<&default_mr_gce>(reducers, num_reducers, [](Reducer<K,V,OutType>& r) {
      r = Reducer<K,V,OutType>();
      }); 
  //TODO symmetric alloc of a MapReduceContext GCE to allow concurrent jobs

  mapExecute<T,K,V,OutType,MapF>(MapperContext<K,V,OutType>(reducers, num_reducers), keyvals, num, mf);

  reduceExecute<K,V,OutType,ReduceF>(reducers, num_reducers, rf);

  return reducers;
}




        /// TODO: join: quite annoying might as well do parallel(map/map) reduce because otherwise need to marshal data from two tables into an forall() iterator anyway 
