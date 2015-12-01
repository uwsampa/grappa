#pragma once

#include <Grappa.hpp>
#include <GlobalAllocator.hpp>
#include <GlobalCompletionEvent.hpp>
#include <ParallelLoop.hpp>
#include <BufferVector.hpp>
#include <Metrics.hpp>

#include <list>
#include <cmath>

//GRAPPA_DECLARE_METRIC(MaxMetric<uint64_t>, max_cell_length);
GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, hash_tables_size);
GRAPPA_DECLARE_METRIC(SummarizingMetric<uint64_t>, hash_tables_lookup_steps);

// for naming the types scoped in DoubleDHT
#define DDHT_TYPE(type) typename DoubleDHT<K,VL,VR,HF>::type
#define _DDHT_TYPE(type) DoubleDHT<K,VL,VR,HF>::type

enum class Direction { LEFT, RIGHT };

// Hash table for joins
// * allows multiple copies of a Key
// * lookups return all Key matches
template <typename K, typename VL, typename VR, uint64_t (*HF)(K)> 
class DoubleDHT {

  private:
    template <typename V>
    struct Entry {
      K key;
      std::vector<V> * vs;
      //BufferVector<V> * vs;
      Entry( K key ) : key(key), vs(new std::vector<V>()) {}
      //Entry( K key ) : key(key), vs(new BufferVector<V>( 16 )) {}
      Entry ( ) {}
    };

    struct PairCell {
      std::list<Entry<VL>> * entriesLeft;
      std::list<Entry<VR>> * entriesRight;

      PairCell() : entriesLeft( NULL ), entriesRight( NULL ) {}
    };

    template <typename V>
    struct lookup_result {
      GlobalAddress<V> matches;
      size_t num;
    };
    
    // private members
    GlobalAddress< PairCell > base;
    size_t capacity;

    uint64_t computeIndex( K key ) {
      return HF(key) & (capacity - 1);
    }

    // for creating local DoubleDHT
    DoubleDHT( GlobalAddress<PairCell> base, uint32_t capacity_pow2 ) {
      this->base = base;
      this->capacity = capacity_pow2;
    }

    template <typename V>
    static bool lookup_local_helper( K key, std::list<_DDHT_TYPE(Entry<V>)> * entries, Entry<V> * result ) {
      // first check if the cell has any entries;
      // if not then the lookup returns nothing
      if ( entries == NULL ) return false;

      uint64_t steps = 1;
      for (auto i = entries->begin(); i!=entries->end(); ++i) {
        const Entry<V> e = *i;
        if ( e.key == key ) {  // typename K must implement operator==
          *result = e;
          hash_tables_lookup_steps += steps;
          return true;
        }
        ++steps;
      }
      hash_tables_lookup_steps += steps;
      return false;
    }

    static bool lookup_local_left( K key, PairCell * target, Entry<VL> * result) {
      return lookup_local_helper(key, target->entriesLeft, result);
    }
    static bool lookup_local_right( K key, PairCell * target, Entry<VR> * result) {
      return lookup_local_helper(key, target->entriesRight, result);
    }

    template <bool Unique, typename V>
    static void insert_local_helper( K key, std::list<_DDHT_TYPE(Entry<V>)> * entries, V val) {
      // find matching key in the list
      for (auto i = entries->begin(); i!=entries->end(); ++i) {
        Entry<V> e = *i;
        if ( e.key == key ) {
          // key found so add to matches
          if (!Unique) {
            e.vs->push_back( val );
            hash_tables_size+=1;
          }
          return;
        }
      }

      // this is the first time the key has been seen
      // so add it to the list
      Entry<V> newe( key );
      newe.vs->push_back( val );
      entries->push_back( newe );
      hash_tables_size+=1;
    }
   
    template < bool Unique >
    static void insert_local_left( K key, PairCell * target, VL val ) {
      // if first time the cell is hit then initialize
      if ( target->entriesLeft == NULL ) {
        target->entriesLeft = new std::list<Entry<VL>>();
      }

      insert_local_helper<Unique>( key, target->entriesLeft, val ); 
    }

    template < bool Unique >
    static void insert_local_right( K key, PairCell * target, VR val ) {
      // if first time the cell is hit then initialize
      if ( target->entriesRight == NULL ) {
        target->entriesRight = new std::list<Entry<VR>>();
      }

      insert_local_helper<Unique>( key, target->entriesRight, val ); 
    }
    
  public:
    // for static construction
    DoubleDHT( ) {}

    static void init_global_DHT( DoubleDHT<K,VL,VR,HF> * globally_valid_local_pointer, size_t capacity ) {

      uint32_t capacity_exp = log2(capacity);
      size_t capacity_powerof2 = pow(2, capacity_exp);
      GlobalAddress<PairCell> base = Grappa::global_alloc<PairCell>( capacity_powerof2 );

      Grappa::on_all_cores( [globally_valid_local_pointer,base,capacity_powerof2] {
        *globally_valid_local_pointer = DoubleDHT<K,VL,VR,HF>( base, capacity_powerof2 );
      });

      Grappa::forall( base, capacity_powerof2, []( int64_t i, PairCell& c ) {
        PairCell empty;
        c = empty;
      });
    }

    static void set_RO_global( DoubleDHT<K,VL,VR,HF> * globally_valid_local_pointer ) {
          //noop
      //Grappa::forall( globally_valid_local_pointer->base, globally_valid_local_pointer->capacity, []( int64_t i, Cell& c ) {
      //});
    }

    /* ineset_lookup*():
     *   Does an insert to hash table D and lookup in hash table ~D.
     *   This insert-lookup is atomic. This means concurrent insert-lookups
     *   will soundly compute the join of two datasets. Note that insert_lookup
     *   is not idempotent because of the insert.
     */

/* Uncomment to duplicate left/right
    template< Direction D, typename VLkp, typename VIns >
    uint64_t insert_lookup ( K key, VIns val, GlobalAddress<VLkp> * resultsAddr ) {          
      uint64_t index = computeIndex( key );
      GlobalAddress< PairCell > target = base + index; 

      // FIXME: remove 'this' capture when using gcc4.8, this is just a bug in 4.7
      lookup_result<VLkp> result = Grappa::delegate::call( target.core(), [key,val,target,this]() {

        // this is atomic { insert_local; lookup_local }
        insert_local<D, VIns>( key, target.pointer(), val);

        _DDHT_TYPE(lookup_result<VLkp>) lr;
        lr.num = 0;

        Entry<VLkp> e;
        if (lookup_local<D, VLkp>( key, target.pointer(), &e)) {
         // XXX: only valid for localeShared mem version.
         // lr.matches = static_cast<GlobalAddress<V>>(e.vs->getReadBuffer());
          lr.num = e.vs->size();
        }

        return lr;
      });

      *resultsAddr = result.matches;
      return result.num;
    } 
    */

    // Left and right version instead of with templates because of type checker preceeds dead code elim.
    // VL == VR, or might be different, so we can't check on them either.
    
    // version of lookup that takes a continuation instead of returning results back
    template< typename CF, Grappa::GlobalCompletionEvent * GCE = &Grappa::impl::local_gce, bool Unique=false >
    void insert_lookup_iter_left ( K key, VL val, CF f ) {
      uint64_t index = computeIndex( key );
      GlobalAddress< PairCell > target = base + index; 

      // FIXME: remove 'this' capture when using gcc4.8, this is just a bug in 4.7
      //TODO optimization where only need to do remotePrivateTask instead of call_async
      //if you are going to do more suspending ops (comms) inside the loop
      Grappa::spawnRemote<GCE>( target.core(), [key, val, target, f, this]() {
        
        // this is atomic { insert_local; lookup_local }
        insert_local_left<Unique>( key, target.pointer(), val );

        Entry<VR> e;
        if (lookup_local_right( key, target.pointer(), &e)) {
          auto resultsptr = e.vs;
          // Critical for correctness: uses the current size of the vector, so later inserts are not used
          Grappa::forall_here<Grappa::async,GCE>(0, e.vs->size(), [f,resultsptr](int64_t start, int64_t iters) {
            for  (int64_t i=start; i<start+iters; i++) {
              auto results = *resultsptr;
              // call the continuation with the lookup result
              f(results[i]); 
            }
          });
        }
      });
    }
    // overload for only specifying GCE
  template<Grappa::GlobalCompletionEvent * GCE, typename CF, bool Unique=false>
  void insert_lookup_iter_left ( K key, VL val, CF f ) {
    insert_lookup_iter_left<CF, GCE>( key, val, f );
  }

  // overload for only specifying GCE and Unique
  template<Grappa::GlobalCompletionEvent * GCE, bool Unique, typename CF>
  void insert_lookup_iter_left ( K key, VL val, CF f ) {
    insert_lookup_iter_left<CF, GCE>( key, val, f );
  }

    template< typename CF, Grappa::GlobalCompletionEvent * GCE = &Grappa::impl::local_gce, bool Unique=false >
    void insert_lookup_iter_right ( K key, VR val, CF f ) {
      uint64_t index = computeIndex( key );
      GlobalAddress< PairCell > target = base + index; 

      // FIXME: remove 'this' capture when using gcc4.8, this is just a bug in 4.7
      //TODO optimization where only need to do remotePrivateTask instead of call_async
      //if you are going to do more suspending ops (comms) inside the loop
      Grappa::spawnRemote<GCE>( target.core(), [key, val, target, f, this]() {
        
        // this is atomic { insert_local; lookup_local }
        insert_local_right<Unique>( key, target.pointer(), val );

        Entry<VL> e;
        if (lookup_local_left( key, target.pointer(), &e)) {
          auto resultsptr = e.vs;
          // Critical for correctness: uses the current size of the vector, so later inserts are not used
          Grappa::forall_here<Grappa::async,GCE>(0, e.vs->size(), [f,resultsptr](int64_t start, int64_t iters) {
            for  (int64_t i=start; i<start+iters; i++) {
              auto results = *resultsptr;
              // call the continuation with the lookup result
              f(results[i]); 
            }
          });
        }
      });
    }
    // overload for only specifying GCE
  template<Grappa::GlobalCompletionEvent * GCE, typename CF, bool Unique=false>
  void insert_lookup_iter_right ( K key, VR val, CF f ) {
    insert_lookup_iter_right<CF, GCE, Unique>( key, val, f );
  }
  // overload for only specifying GCE and Unique
  template<Grappa::GlobalCompletionEvent * GCE, bool Unique, typename CF>
  void insert_lookup_iter_right ( K key, VR val, CF f ) {
    insert_lookup_iter_right<CF, GCE, Unique>( key, val, f );
  }

    /* Uncomment to duplicate left right
    // version of lookup that takes a continuation instead of returning results back
    template< Direction D, typename VLkp, typename CF, typename VIns, GlobalCompletionEvent * GCE = &Grappa::impl::local_gce >
    void insert_lookup ( K key, VIns val, CF f ) {
      uint64_t index = computeIndex( key );
      GlobalAddress< PairCell > target = base + index; 

      Grappa::delegate::call<Grappa::async, GCE>( target.core(), [key, target, val, f]() {
        // this is atomic { insert_local; lookup_local }
        insert_local<D, VIns>( key, target.pointer(), val );

        Entry<VLkp> e;
        if (lookup_local<D, VLkp>( key, target.pointer(), &e)) {
          uint64_t len = e.vs->size();
          f(e.vs, len); 
        }
      });
    }
    */


};


