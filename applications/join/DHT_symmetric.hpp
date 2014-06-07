#pragma once

#include <Grappa.hpp>
#include <GlobalAllocator.hpp>
#include <GlobalCompletionEvent.hpp>
#include <ParallelLoop.hpp>
#include <Metrics.hpp>

#include <cmath>


//GRAPPA_DECLARE_METRIC(MaxMetric<uint64_t>, max_cell_length);
GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, hash_tables_size);
GRAPPA_DECLARE_METRIC(SummarizingMetric<uint64_t>, hash_tables_lookup_steps);


// for naming the types scoped in DHT_symmetric
#define DHT_symmetric_TYPE(type) typename DHT_symmetric<K,V,HF>::type
#define DHT_symmetric_T DHT_symmetric<K,V,HF>

// Hash table for joins
// * allows multiple copies of a Key
// * lookups return all Key matches
template <typename K, typename V, uint64_t (*HF)(K)> 
class DHT_symmetric {

  private:
    // private members
    GlobalAddress< DHT_symmetric_T > self;
    std::unordered_map<K, V> * local_map;
    size_t partitions;

    uint64_t computeIndex( K key ) {
      return HF(key) % partitions;
    }

    // for creating local DHT_symmetric
    DHT_symmetric( GlobalAddress<DHT_symmetric_T> self ) 
      : self(self)
      , partitions(Grappa::cores())
      , local_map(new std::unordered_map<K,V>())
      {}

  public:
    // for static construction
    DHT_symmetric( ) {}

    static GlobalAddress<DHT_symmetric_T> create_DHT_symmetric( ) {
      auto object = Grappa::symmetric_global_alloc<DHT_symmetric_T>();

      Grappa::on_all_cores( [object] {
        new(object.pointer()) DHT_symmetric_T(object);
      });
      
      return object;
    }

    template< GlobalCompletionEvent * GCE, typename UV, V (*UpF)(V oldval, UV incVal), V Init, SyncMode S = SyncMode::Async >
    void update( K key, UV val ) {
      uint64_t index = computeIndex( key );
      auto target = this->self;

      Grappa::delegate::call<S,GCE>(index, [key, val, target]() {   
        // inserts initial value only if the key is not yet present
        std::pair<K,V> entry(key, Init);
        auto res = target->local_map->insert(entry); auto resIt = res.first; //auto resNew = res.second;

        // perform the update in place
        resIt->second = UpF(resIt->second, val);
      });
    }

    template< GlobalCompletionEvent * GCE, SyncMode S = SyncMode::Async >
    void insert_unique( K key, V val ) {
      uint64_t index = computeIndex( key );
      auto target = this->self;

      Grappa::delegate::call<S,GCE>(index, [key, val, target]() {   
        // inserts initial value only if the key is not yet present
        std::pair<K,V> entry(key, val);
        target->local_map->insert(entry); 
      });
    }
  
    template < GlobalCompletionEvent * GCE, typename CF >
    void forall_entries( CF f ) {
      auto target = this->self;
      Grappa::on_all_cores([target, f] {
          // TODO: cannot use forall_here because unordered_map->begin() is a forward iterator (std::advance is O(n))
          // TODO: for now the serial loop is only performant if the continuation code is also in CPS
          // TODO: best solution is a forall_here where loop decomposition is just linear continuation instead of divide and conquer
          auto m = target->local_map;
          for (auto it = m->begin(); it != m->end(); it++) {
            // continuation takes a mapping
            f(*it);
          }
      }); 
      // TODO GCE->wait(); // block until all tasks are done
    }


} GRAPPA_BLOCK_ALIGNED;

