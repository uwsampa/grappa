#ifndef MATCHES_DHT_HPP
#define MATCHES_DHT_HPP

#include <Grappa.hpp>
#include <GlobalAllocator.hpp>
#include <ParallelLoop.hpp>
#include <BufferVector.hpp>

#include <list>


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
      Entry( K key ) : key(key), vs(new BufferVector<V>()) {}
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
  public:
    // for static construction
    MatchesDHT( ) {}

    static void init_global_DHT( MatchesDHT<K,V,HF> * globally_valid_local_pointer, size_t capacity ) {
      GlobalAddress<Cell> base = Grappa_typed_malloc<Cell>( capacity );

      Grappa::on_all_cores( [globally_valid_local_pointer,base,capacity] {
        *globally_valid_local_pointer = MatchesDHT<K,V,HF>( base, capacity );
      });

      Grappa::forall_localized( base, capacity, []( int64_t i, Cell& c ) {
        Cell empty;
        c = empty;
      });
    }

    static void set_RO_global( MatchesDHT<K,V,HF> * globally_valid_local_pointer ) {
      Grappa::forall_localized( globally_valid_local_pointer->base, globally_valid_local_pointer->capacity, []( int64_t i, Cell& c ) {
        // list of entries in this cell
        std::list<MDHT_TYPE(Entry)> * entries = c.entries;

        // if cell is not hit then no action
        if ( entries == NULL ) {
          return;
        }

        // for all keys, set match vector to RO
        typename std::list<MDHT_TYPE(Entry)>::iterator it;
        for (it = entries->begin(); it!=entries->end(); ++it) {
          Entry e = *it;
          e.vs->setReadMode();
        }
      });
    }

    uint64_t lookup ( K key, GlobalAddress<V> * vals ) {          
      uint64_t index = computeIndex( key );
      GlobalAddress< Cell > target = base + index; 

      lookup_result result = Grappa::delegate::call( target.node(), [key,target]() {

        std::list<MDHT_TYPE(Entry)> * entries = target.pointer()->entries;
        MDHT_TYPE(lookup_result) lr;
        lr.num = 0;

        // first check if the cell has any entries;
        // if not then the lookup returns nothing
        if ( entries == NULL ) return lr;

        typename std::list<MDHT_TYPE(Entry)>::iterator i;
        for (i = entries->begin(); i!=entries->end(); ++i) {
          const Entry e = *i;
          if ( e.key == key ) {  // typename K must implement operator==
            lr.matches = e.vs->getReadBuffer();
            lr.num = e.vs->getLength();
            break;
          }
        }

        return lr;
      });

      *vals = result.matches;
      return result.num;
    } 


    void insert( K key, V val ) {
      uint64_t index = computeIndex( key );
      GlobalAddress< Cell > target = base + index; 

      Grappa::delegate::call( target.node(), [key, val, target]() {   // TODO: upgrade to call_async
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

        return 0; // TODO: see "delegate that..."
      });
    }


};


#endif // MATCHES_DHT_HPP
