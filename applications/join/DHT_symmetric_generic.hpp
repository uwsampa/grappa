#pragma once

#include <Grappa.hpp>
#include <GlobalAllocator.hpp>
#include <GlobalCompletionEvent.hpp>
#include <ParallelLoop.hpp>
#include <Metrics.hpp>

#include <cmath>
#include <unordered_map>


GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, dht_inserts);

// for naming the types scoped in DHT_symmetric_generic
#define DHT_symmetric_generic_TYPE(type) typename DHT_symmetric_generic<K,V,UV,Hash>::type
#define DHT_symmetric_generic_T DHT_symmetric_generic<K,V,UV,Hash>

// TODO: The functionality of this class is covered by DHT_symmetric already.
//       The only difference is that update_f/init_f are template parameters
//       versus instance parameters. We can just add these instance parameters
//       to DHT_symmetric and have two versions of update() method.

// Hash table for joins
// * allows multiple copies of a Key
// * lookups return all Key matches
template <typename K, typename V, typename UV, typename Hash> 
class DHT_symmetric_generic {
  public:
    typedef V (*update_f)(const V& oldval, const UV& incVal);
    typedef V (*init_f)(void);

  private:
    // private members
    GlobalAddress< DHT_symmetric_generic_T > self;
    std::unordered_map<K, V, Hash > * local_map;
    size_t partitions;

    update_f UpF;
    init_f Init;

    size_t computeIndex( K key ) {
      return Hash()(key) % partitions;
    }

    // for creating local DHT_symmetric_generic
    DHT_symmetric_generic( GlobalAddress<DHT_symmetric_generic_T> self, update_f upf, init_f initf ) 
      : self(self)
      , UpF(upf)
      , Init(initf)
      , partitions(Grappa::cores())
      , local_map(new std::unordered_map<K,V,Hash>())
      {}

  public:
    // for static construction
    DHT_symmetric_generic( ) {}

    static GlobalAddress<DHT_symmetric_generic_T> create_DHT_symmetric( update_f upf, init_f initf ) {
      auto object = Grappa::symmetric_global_alloc<DHT_symmetric_generic_T>();

      Grappa::on_all_cores( [object, upf, initf] {
        new(object.pointer()) DHT_symmetric_generic_T(object, upf, initf);
      });
      
      return object;
    }

    void update_partition(K key, UV val) {
      std::pair<K,V> entry(key, Init());
      
      auto res = this->local_map->insert(entry); auto resIt = res.first; //auto resNew = res.second;

      // perform the update in place
      resIt->second = this->UpF(resIt->second, val);
      dht_partition_inserts++;
    }

    template< GlobalCompletionEvent * GCE, SyncMode S = SyncMode::Async >
    void update( K key, UV val ) {
      auto index = computeIndex( key );
      auto target = this->self;

      Grappa::delegate::call<S,GCE>(index, [key, val, target]() {   
        // inserts initial value only if the key is not yet present
        std::pair<K,V> entry(key, target->Init());

        auto res = target->local_map->insert(entry); auto resIt = res.first; //auto resNew = res.second;

        // perform the update in place
        resIt->second = target->UpF(resIt->second, val);
        dht_inserts++;
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

    std::unordered_map<K,V,Hash> * get_local_map() {
      return local_map;
    }
    


} GRAPPA_BLOCK_ALIGNED;

