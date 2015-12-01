#ifndef MATCHES_DHT_HPP
#define MATCHES_DHT_HPP

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

GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, hash_remote_lookups);
GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, hash_remote_inserts);
GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, hash_local_lookups);
GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, hash_local_inserts);
GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, hash_called_lookups);
GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, hash_called_inserts);


// for naming the types scoped in MatchesDHT
#define MDHT_TYPE(type) typename MatchesDHT<K,V,HF>::type

// Hash table for joins
// * allows multiple copies of a Key
// * lookups return all Key matches
template <typename K, typename V, uint64_t (*HF)(K)> 
class MatchesDHT {

  private:
    struct Entry {
      K key;
      std::vector<V> * vs;
      //BufferVector<V> * vs;
      Entry( K key ) : key(key), vs(new std::vector<V>()) {}
      //Entry( K key ) : key(key), vs(new BufferVector<V>( 16 )) {}
      Entry ( ) {}
    };

    struct Cell {
      std::list<Entry> * entries;

      Cell() : entries( NULL ) {}
    };

    struct lookup_result {
      GlobalAddress<V> matches;
      size_t num;
    };
    
    // private members
    GlobalAddress< Cell > base;
    size_t capacity;

    uint64_t computeIndex( K key ) {
      return HF(key) & (capacity - 1);
    }

    // for creating local MatchesDHT
    MatchesDHT( GlobalAddress<Cell> base, uint32_t capacity_pow2 ) {
      this->base = base;
      this->capacity = capacity_pow2;
    }

    static bool lookup_local( K key, Cell * target, Entry * result ) {
        std::list<MDHT_TYPE(Entry)> * entries = target->entries;

        // first check if the cell has any entries;
        // if not then the lookup returns nothing
        if ( entries == NULL ) return false;

        typename std::list<MDHT_TYPE(Entry)>::iterator i;
        uint64_t steps = 1;
        for (i = entries->begin(); i!=entries->end(); ++i) {
          const Entry e = *i;
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
    
  public:
    // for static construction
    MatchesDHT( ) {}

    static void init_global_DHT( MatchesDHT<K,V,HF> * globally_valid_local_pointer, size_t capacity ) {

      uint32_t capacity_exp = log2(capacity);
      size_t capacity_powerof2 = pow(2, capacity_exp);
      GlobalAddress<Cell> base = Grappa::global_alloc<Cell>( capacity_powerof2 );

      Grappa::on_all_cores( [globally_valid_local_pointer,base,capacity_powerof2] {
        *globally_valid_local_pointer = MatchesDHT<K,V,HF>( base, capacity_powerof2 );
      });

      Grappa::forall( base, capacity_powerof2, []( int64_t i, Cell& c ) {
        Cell empty;
        c = empty;
      });
    }


    struct size_aligned {
      size_t s;

      size_aligned operator+(const size_aligned& o) const {
        size_aligned v;
        v.s = this->s + o.s;
        return v;
      }
    } GRAPPA_BLOCK_ALIGNED;

   size_t size() {
     auto l_size = Grappa::symmetric_global_alloc<size_aligned>();
     Grappa::forall( base, capacity, [l_size](int64_t i, Cell& c) {
       if (c.entries != NULL) {
         for (auto eiter = c.entries->begin(); eiter != c.entries->end(); ++eiter) {
           l_size->s += eiter->vs->size();
         }
       }
     });
    return Grappa::reduce<size_aligned, COLL_ADD>(l_size ).s;
        //FIXME: mem leak l_size
   }


    static void set_RO_global( MatchesDHT<K,V,HF> * globally_valid_local_pointer ) {
      Grappa::forall( globally_valid_local_pointer->base, globally_valid_local_pointer->capacity, []( int64_t i, Cell& c ) {
        // list of entries in this cell
        std::list<MDHT_TYPE(Entry)> * entries = c.entries;

        // if cell is not hit then no action
        if ( entries == NULL ) {
          return;
        }


        uint64_t sum_size = 0;
        // for all keys, set match vector to RO
        typename std::list<MDHT_TYPE(Entry)>::iterator it;
        for (it = entries->begin(); it!=entries->end(); ++it) {
          Entry e = *it;
          //e.vs->setReadMode();
          sum_size+=e.vs->size();
        }
        //max_cell_length.add(sum_size);
      });
    }

    uint64_t lookup ( K key, GlobalAddress<V> * vals ) {          
      uint64_t index = computeIndex( key );
      GlobalAddress< Cell > target = base + index; 

      // FIXME: remove 'this' capture when using gcc4.8, this is just a bug in 4.7
      lookup_result result = Grappa::delegate::call( target.core(), [key,target,this]() {
        hash_called_lookups++;

        MDHT_TYPE(lookup_result) lr;
        lr.num = 0;

        Entry e;
        if (lookup_local( key, target.pointer(), &e)) {
         // lr.matches = static_cast<GlobalAddress<V>>(e.vs->getReadBuffer());
          lr.num = e.vs->size();
        }

        return lr;
      });

      *vals = result.matches;
      return result.num;
    } 


    // version of lookup that takes a continuation instead of returning results back
    template< typename CF, Grappa::GlobalCompletionEvent * GCE = &Grappa::impl::local_gce >
    void lookup_iter ( K key, CF f ) {
      uint64_t index = computeIndex( key );
      GlobalAddress< Cell > target = base + index; 

      // FIXME: remove 'this' capture when using gcc4.8, this is just a bug in 4.7
      //TODO optimization where only need to do remotePrivateTask instead of call_async
      //if you are going to do more suspending ops (comms) inside the loop
      if (target.core() == Grappa::mycore()) {
        hash_local_lookups++;
      } else {
        hash_remote_lookups++;
      }
      Grappa::spawnRemote<GCE>( target.core(), [key, target, f, this]() {
        hash_called_lookups++;
        Entry e;
        if (lookup_local( key, target.pointer(), &e)) {
          auto resultsptr = e.vs;
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
    template< Grappa::GlobalCompletionEvent * GCE, typename CF >
    void lookup_iter ( K key, CF f ) {
      lookup_iter<CF, GCE>(key, f);
    }

    // version of lookup that takes a continuation instead of returning results back
    template< typename CF, Grappa::GlobalCompletionEvent * GCE = &Grappa::impl::local_gce >
    void lookup ( K key, CF f ) {
      uint64_t index = computeIndex( key );
      GlobalAddress< Cell > target = base + index; 

      Grappa::delegate::call<Grappa::async>( target.core(), [key, target, f]() {
        hash_called_lookups++;
        Entry e;
        if (lookup_local( key, target.pointer(), &e)) {
          uint64_t len = e.vs->size();
          f(e.vs, len); 
        }
      });
    }


    // Inserts the key if not already in the set
    // Shouldn't be used with `insert`.
    //
    // returns true if the set already contains the key
    void insert_unique( K key, V val ) {
      uint64_t index = computeIndex( key );
      GlobalAddress< Cell > target = base + index; 
      Grappa::delegate::call( target.core(), [key,val,target]() {   // TODO: have an additional version that returns void
                                                                 // to upgrade to call_async
        hash_called_inserts++;

        // list of entries in this cell
        std::list<MDHT_TYPE(Entry)> * entries = target.pointer()->entries;

        // if first time the cell is hit then initialize
        if ( entries == NULL ) {
          entries = new std::list<Entry>();
          target.pointer()->entries = entries;
        }

        // find matching key in the list
        typename std::list<MDHT_TYPE(Entry)>::iterator i;
        for (i = entries->begin(); i!=entries->end(); ++i) {
          Entry e = *i;
          if ( e.key == key ) {
            // key found so no insert
            return;
          }
        }

        // this is the first time the key has been seen
        // so add it to the list
        Entry newe( key );        // TODO: cleanup since sharing insert* code here, we are just going to store an empty vector
        newe.vs->push_back( val );
                                  // perhaps a different module
        entries->push_back( newe );

        return; 
     });
    }

    template< Grappa::GlobalCompletionEvent * GCE = &Grappa::impl::local_gce >
    void insert_async( K key, V val ) {
      uint64_t index = computeIndex( key );
      GlobalAddress< Cell > target = base + index; 

      if (target.core() == Grappa::mycore()) {
        hash_local_inserts++;
      } else {
        hash_remote_inserts++;
      }
      Grappa::delegate::call<Grappa::async, GCE>( target.core(), [key, val, target]() {   // TODO: upgrade to call_async; using GCE
        hash_called_inserts++;

        // list of entries in this cell
        std::list<MDHT_TYPE(Entry)> * entries = target.pointer()->entries;

        // if first time the cell is hit then initialize
        if ( entries == NULL ) {
          entries = new std::list<Entry>();
          target.pointer()->entries = entries;
        }

        // find matching key in the list
        typename std::list<MDHT_TYPE(Entry)>::iterator i;
        for (i = entries->begin(); i!=entries->end(); ++i) {
          Entry e = *i;
          if ( e.key == key ) {
            // key found so add to matches
            e.vs->push_back( val );
            hash_tables_size+=1;
            return;
          }
        }

        // this is the first time the key has been seen
        // so add it to the list
        Entry newe( key );
        newe.vs->push_back( val );
        entries->push_back( newe );

        return; 
      });
    }


};


#endif // MATCHES_DHT_HPP
