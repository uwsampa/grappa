#pragma once

#include <Grappa.hpp>
#include <GlobalCompletionEvent.hpp>
#include <unordered_map>
#include <vector>

#include "utils.h"
#include "stats.h"

extern Grappa::GlobalCompletionEvent default_join_left_gce;
extern Grappa::GlobalCompletionEvent default_join_right_gce;
extern Grappa::GlobalCompletionEvent default_join_reduce_gce;

template <typename K, typename VL, typename VR, typename OutType>
struct JoinReducer {
  std::unordered_map<K, std::vector<VL>> * groupsL;
  std::unordered_map<K, std::vector<VR>> * groupsR;

  std::vector<OutType> * result;

  JoinReducer() : groupsL(new std::unordered_map<K, std::vector<VL>>())
            , groupsR(new std::unordered_map<K, std::vector<VR>>())
            , result(new std::vector<OutType>()) {}

  static std::vector<OutType>& resultAccessor(GlobalAddress<JoinReducer<K, VL, VR, OutType>> o) {
    return *(o->result);
  }


} GRAPPA_BLOCK_ALIGNED; 

template < typename K, typename VL, typename VR, typename OutType >
GlobalAddress<JoinReducer<K,VL,VR,OutType>> allocateJoinReducers( size_t num_reducers ) {
  auto reducers = Grappa::global_alloc<JoinReducer<K, VL, VR, OutType>>( num_reducers );    
  Grappa::forall(reducers, num_reducers, [](JoinReducer<K,VL,VR,OutType>& r) {
      r = JoinReducer<K,VL,VR,OutType>();
      }); 
  return reducers;
}

template < typename K, typename VL, typename VR, typename OutType >
void freeJoinReducers(GlobalAddress<JoinReducer<K,VL,VR,OutType>> reducers, size_t num_reducers) {
  Grappa::forall(reducers, num_reducers, [](JoinReducer<K,VL,VR,OutType>& r) {
      delete r.result;
  });
  Grappa::global_free(reducers);
}

template <typename K, typename VL, typename VR, typename OutType, Grappa::GlobalCompletionEvent * GCE = &default_join_left_gce>
static void reducer_append_left( GlobalAddress<JoinReducer<K,VL,VR,OutType>> r, K key, VL val ) {
  Grappa::delegate::call<Grappa::async, GCE>(r.core(), [=] {
    VLOG(5) << "add (" << key << ", " << val << ") at " << r.pointer();
    std::vector<VL>& slot = (*(r->groupsL))[key];
    slot.push_back(val);
  });
}

template <typename K, typename VL, typename VR, typename OutType, Grappa::GlobalCompletionEvent * GCE = &default_join_right_gce>
static void reducer_append_right( GlobalAddress<JoinReducer<K,VL,VR,OutType>> r, K key, VR val ) {
  Grappa::delegate::call<Grappa::async, GCE>(r.core(), [=] {
    VLOG(5) << "add (" << key << ", " << val << ") at " << r.pointer();
    std::vector<VR>& slot = (*(r->groupsR))[key];
    slot.push_back(val);
  });
}

template <typename K, typename VL, typename VR, typename OutType>
struct HashJoinContext {
  GlobalAddress<JoinReducer<K,VL,VR,OutType>> reducers;
  size_t num_reducers;

  HashJoinContext(GlobalAddress<JoinReducer<K,VL,VR,OutType>> reducers, size_t num_reducers) 
    : reducers(reducers)
    , num_reducers(num_reducers) {}
  
    template < Grappa::GlobalCompletionEvent * GCE = &default_join_left_gce >
    void emitIntermediateLeft( K key, VL val ) const {
      auto index = std::hash<K>()(key) % num_reducers;
      auto target = reducers + index;
      reducer_append_left<K,VL,VR,OutType,GCE>( target, key, val );
    }

    template < Grappa::GlobalCompletionEvent * GCE = &default_join_right_gce >
    void emitIntermediateRight( K key, VR val ) const {
      auto index = std::hash<K>()(key) % num_reducers;
      auto target = reducers + index;
      reducer_append_right<K,VL,VR,OutType,GCE>( target, key, val );
    }

    template < Grappa::GlobalCompletionEvent * GCE = &default_join_reduce_gce >
    void reduceExecute() {
      // equijoin
      Grappa::forall<GCE>(reducers, num_reducers, [=]( int64_t i, JoinReducer<K,VL,VR,OutType>& reducer) {
          // for all keys on left
          for ( auto local_it = reducer.groupsL->begin(); local_it!= reducer.groupsL->end(); ++local_it ) {
            std::vector<VL>& leftGroup = local_it->second;
            // get match key on right 
            std::vector<VR>& rightGroup = (*(reducer.groupsR))[local_it->first]; // NOTE: allocates
            // cross product
            for (auto right_it = rightGroup.begin(); right_it!=rightGroup.end(); ++right_it) {
              for (auto left_it = leftGroup.begin(); left_it!=leftGroup.end(); ++left_it) {
                join_coarse_result_count++;
                reducer.result->push_back( combine<OutType,VL,VR>(*left_it,*right_it) );
              }
            }
          }

          // deallocate the group
          reducer.groupsR->clear();
          reducer.groupsL->clear();
      });
    }
};


