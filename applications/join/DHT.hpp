#ifndef DHT_HPP
#define DHT_HPP

#include <Grappa.hpp>
#include <GlobalAllocator.hpp>
#include <GlobalCompletionEvent.hpp>
#include <ParallelLoop.hpp>
#include <Metrics.hpp>

#include <cmath>


//GRAPPA_DECLARE_METRIC(MaxMetric<uint64_t>, max_cell_length);
GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, hash_tables_size);
GRAPPA_DECLARE_METRIC(SummarizingMetric<uint64_t>, hash_tables_lookup_steps);


// for naming the types scoped in DHT
#define DHT_TYPE(type) typename DHT<K,V,HF>::type

// Hash table for joins
// * allows multiple copies of a Key
// * lookups return all Key matches
template <typename K, typename V, uint64_t (*HF)(K)> 
class DHT {

  private:
    struct Entry {
      K key;
      V value;
      Entry( K key, V value ) : key(key), value(value) {}
      Entry ( ) {}
    };

    struct Cell {
      std::list<Entry> * entries;

      Cell() : entries( NULL ) {}
    };

    // private members
    GlobalAddress< Cell > base;
    size_t capacity;

    uint64_t computeIndex( K key ) {
      return HF(key) & (capacity - 1);
    }

    // for creating local DHT
    DHT( GlobalAddress<Cell> base, uint32_t capacity_pow2 ) {
      this->base = base;
      this->capacity = capacity_pow2;
    }

    struct lookup_result {
      V result;
      bool valid;
    }

    static bool lookup_local( K key, Cell * target, Entry * result ) {
        std::list<DHT_TYPE(Entry)> * entries = target->entries;

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
    DHT( ) {}

    static void init_global_DHT( DHT<K,V,HF> * globally_valid_local_pointer, size_t capacity ) {

      uint32_t capacity_exp = log2(capacity);
      size_t capacity_powerof2 = pow(2, capacity_exp);
      GlobalAddress<Cell> base = Grappa::global_alloc<Cell>( capacity_powerof2 );

      Grappa::on_all_cores( [globally_valid_local_pointer,base,capacity_powerof2] {
        *globally_valid_local_pointer = DHT<K,V,HF>( base, capacity_powerof2 );
      });

      Grappa::forall( base, capacity_powerof2, []( int64_t i, Cell& c ) {
        Cell empty;
        c = empty;
      });
    }

    static void set_RO_global( DHT<K,V,HF> * globally_valid_local_pointer ) {
      Grappa::forall( globally_valid_local_pointer->base, globally_valid_local_pointer->capacity, []( int64_t i, Cell& c ) {
        // list of entries in this cell
        std::list<DHT_TYPE(Entry)> * entries = c.entries;

        // if cell is not hit then no action
        if ( entries == NULL ) {
          return;
        }


        uint64_t sum_size = 0;
        // for all keys, set match vector to RO
        typename std::list<DHT_TYPE(Entry)>::iterator it;
        for (it = entries->begin(); it!=entries->end(); ++it) {
          Entry e = *it;
          //e.vs->setReadMode();
          sum_size+=e.vs->size();
        }
        //max_cell_length.add(sum_size);
      });
    }


    bool lookup ( K key, V * val ) {          
      uint64_t index = computeIndex( key );
      GlobalAddress< Cell > target = base + index; 

      // FIXME: remove 'this' capture when using gcc4.8, this is just a bug in 4.7
      lookup_result result = Grappa::delegate::call( target.core(), [key,target,this]() {

        DHT_TYPE(lookup_result) lr;

        Entry e;
        if (lookup_local( key, target.pointer(), &e)) {
          lr.valid = true;
          lr.result = e.value;
        }

        return lr;
      });

      *val = result.result;
      return result.valid;
    } 


    // version of lookup that takes a continuation instead of returning results back
    template< typename CF, Grappa::GlobalCompletionEvent * GCE = &Grappa::impl::local_gce >
    void lookup ( K key, CF f ) {
      uint64_t index = computeIndex( key );
      GlobalAddress< Cell > target = base + index; 

      // FIXME: remove 'this' capture when using gcc4.8, this is just a bug in 4.7
      //TODO optimization where only need to do remotePrivateTask instead of call_async
      //if you are going to do more suspending ops (comms) inside the loop
      Grappa::spawnRemote<GCE>( target.core(), [key, target, f, this]() {
        Entry e;
        if (lookup_local( key, target.pointer(), &e)) {
          f(e.value);
        }
      });
    }
    template< Grappa::GlobalCompletionEvent * GCE, typename CF >
    void lookup_cps ( K key, CF f ) {
      lookup_cps<CF, GCE>(key, f);
    }
    
    template< typename UV, V (*UpF)(V oldval, UV incVal), V Init >
    void update( K key, UV val ) {
      uint64_t index = computeIndex( key );
      GlobalAddress< Cell > target = base + index; 

      Grappa::delegate::call( target.core(), [key, val, target]() {   // TODO: upgrade to call_async; using GCE
        // list of entries in this cell
        std::list<DHT_TYPE(Entry)> * entries = target.pointer()->entries;

        // if first time the cell is hit then initialize
        if ( entries == NULL ) {
          entries = new std::list<Entry>();
          target.pointer()->entries = entries;
        }

        // find matching key in the list
        typename std::list<DHT_TYPE(Entry)>::iterator i;
        for (i = entries->begin(); i!=entries->end(); ++i) {
          if ( i->key == key ) {
            // key found so update
            i->value = UpF(i->value, val);
            hash_tables_size+=1;
            return 0;
          }
        }

        // this is the first time the key has been seen
        // so add it to the list
        Entry newe( key, UpF(Init, val));

        return 0; 
      });
    }


};


#endif // MATCHES_DHT_HPP
