#ifndef MATCHES_DHT_HPP
#define MATCHES_DHT_HPP

#include <Grappa.hpp>
#include <GlobalAllocator.hpp>
#include <ParallelLoop.hpp>
#include <BufferVector.hpp>
#include <Metrics.hpp>

#include <list>

// for all hash tables
//GRAPPA_DEFINE_METRIC(MaxMetric<uint64_t>, max_cell_length, 0);


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
      BufferVector<V> * vs;
      Entry( K key ) : key(key), vs(new BufferVector<V>( 16 )) {}
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
      return HF(key) % capacity;
    }

    // for creating local MatchesDHT
    MatchesDHT( GlobalAddress<Cell> base, size_t capacity ) {
      this->base = base;
      this->capacity = capacity;
    }

    static bool lookup_local( K key, Cell * target, Entry * result ) {
        std::list<MDHT_TYPE(Entry)> * entries = target->entries;

        // first check if the cell has any entries;
        // if not then the lookup returns nothing
        if ( entries == NULL ) return false;

        typename std::list<MDHT_TYPE(Entry)>::iterator i;
        for (i = entries->begin(); i!=entries->end(); ++i) {
          const Entry e = *i;
          if ( e.key == key ) {  // typename K must implement operator==
            *result = e;
            return true;
          }
        }

        return false;
    }
    
  public:
    // for static construction
    MatchesDHT( ) {}

    static void init_global_DHT( MatchesDHT<K,V,HF> * globally_valid_local_pointer, size_t capacity ) {
      GlobalAddress<Cell> base = Grappa::global_alloc<Cell>( capacity );

      Grappa::on_all_cores( [globally_valid_local_pointer,base,capacity] {
        *globally_valid_local_pointer = MatchesDHT<K,V,HF>( base, capacity );
      });

      Grappa::forall( base, capacity, []( int64_t i, Cell& c ) {
        Cell empty;
        c = empty;
      });
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
          e.vs->setReadMode();
          sum_size+=e.vs->getLength();
        }
        //max_cell_length.add(sum_size);
      });
    }


    uint64_t lookup ( K key, GlobalAddress<V> * vals ) {          
      uint64_t index = computeIndex( key );
      GlobalAddress< Cell > target = base + index; 

      // FIXME: remove 'this' capture when using gcc4.8, this is just a bug in 4.7
      lookup_result result = Grappa::delegate::call( target.core(), [key,target,this]() {

        MDHT_TYPE(lookup_result) lr;
        lr.num = 0;

        Entry e;
        if (lookup_local( key, target.pointer(), &e)) {
          lr.matches = static_cast<GlobalAddress<V>>(e.vs->getReadBuffer());
          lr.num = e.vs->getLength();
        }

        return lr;
      });

      *vals = result.matches;
      return result.num;
    } 


    // version of lookup that takes a continuation instead of returning results back
    template< typename CF, GlobalCompletionEvent * GCE = &Grappa::impl::local_gce >
    void lookup_iter ( K key, CF f ) {
      uint64_t index = computeIndex( key );
      GlobalAddress< Cell > target = base + index; 

      // FIXME: remove 'this' capture when using gcc4.8, this is just a bug in 4.7
      //TODO optimization where only need to do remotePrivateTask instead of call_async
      //if you are going to do more suspending ops (comms) inside the loop
      spawnRemote<GCE>( target.core(), [key, target, f, this]() {
        Entry e;
        if (lookup_local( key, target.pointer(), &e)) {
          V * results = static_cast<GlobalAddress<V>>(e.vs->getReadBuffer()).pointer(); // global address to local array
          forall_here<async,GCE>(0, e.vs->getLength(), [f,results](int64_t start, int64_t iters) {
            for  (int64_t i=start; i<start+iters; i++) {
              // call the continuation with the lookup result
              f(results[i]); 
            }
          });
        }
      });
    }

    // version of lookup that takes a continuation instead of returning results back
    template< typename CF, GlobalCompletionEvent * GCE = &Grappa::impl::local_gce >
    void lookup ( K key, CF f ) {
      uint64_t index = computeIndex( key );
      GlobalAddress< Cell > target = base + index; 

      Grappa::delegate::call<async>( target.core(), [key, target, f]() {
        Entry e;
        if (lookup_local( key, target.pointer(), &e)) {
          V * results = static_cast<GlobalAddress<V>>(e.vs->getReadBuffer()).pointer(); // global address to local array
          uint64_t len = e.vs->getLength();
          f(results, len); 
        }
      });
    }


    // Inserts the key if not already in the set
    // Shouldn't be used with `insert`.
    //
    // returns true if the set already contains the key
    bool insert_unique( K key ) {
      uint64_t index = computeIndex( key );
      GlobalAddress< Cell > target = base + index; 
//FIXME: remove index capture
      bool result = Grappa::delegate::call( target.core(), [index,key, target]() {   // TODO: have an additional version that returns void
                                                                 // to upgrade to call_async
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
            return true;
          }
        }

        // this is the first time the key has been seen
        // so add it to the list
        Entry newe( key );        // TODO: cleanup since sharing insert* code here, we are just going to store an empty vector
                                  // perhaps a different module
        entries->push_back( newe );

        return false; 
     });

      return result;
    }


    void insert( K key, V val ) {
      uint64_t index = computeIndex( key );
      GlobalAddress< Cell > target = base + index; 

      Grappa::delegate::call( target.core(), [key, val, target]() {   // TODO: upgrade to call_async
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
            e.vs->insert( val );
            return 0;
          }
        }

        // this is the first time the key has been seen
        // so add it to the list
        Entry newe( key );
        newe.vs->insert( val );
        entries->push_back( newe );

        return 0; 
      });
    }


};


#endif // MATCHES_DHT_HPP
