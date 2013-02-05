#ifndef DHT_HPP
#define DHT_HPP

#include "Grappa.hpp"
#include "GlobalAllocator.hpp"
#include "ForkJoin.hpp"

#include <list>




// Hash table for joins
// * allows multiple copies of a Key
// * lookups return all Key matches
template <typename K, typename V, uint64_t (*HF)(K)> 
class DHT {

  private:
    struct Entry {
      K key;
      V val;
      Entry( K key, V val ) : key(key), val(val) {}
      Entry() {}
    };

    struct Cell {
      std::list<Entry> * entries;
    };

    //TODO genertic delegate that uses just globaladdress
    //instead of Node
    struct id_args {
      K key;
      V val;
      GlobalAddress<Cell> cell;
      id_args(K key, V val, GlobalAddress<Cell> cell) : key(key), val(val), cell(cell) {}
      id_args() {}
    };
    struct ld_args {
      K key;
      GlobalAddress<Cell> cell;
      ld_args(K key, GlobalAddress<Cell> cell) : key(key), cell(cell) {}
      ld_args() {}
    };
    struct lookup_result {
      V val;
      bool valid;
    };
    typedef bool ignore_t; // TODO: delegate that does not return a value

    // private members
    GlobalAddress< Cell > base;
    size_t capacity;

    static ignore_t insert_delegated( id_args args );
    static lookup_result lookup_delegated( ld_args args );
   
    uint64_t computeIndex( K key ) {
      return HF(key) % capacity;
    }

    struct DHT_construct_each : ForkJoinIteration {
      DHT<K,V,HF> * loc;
      GlobalAddress<Cell> base;
      size_t capacity;

      DHT_construct_each( DHT<K,V,HF> * loc, GlobalAddress<Cell> base, size_t capacity ) 
          : loc(loc), base(base), capacity(capacity) {}
      DHT_construct_each() {}
        
      inline void operator() (int64_t nid) {
        *loc = DHT<K,V,HF>( base, capacity );
      }
    };

    // for creating local DHT
    DHT( GlobalAddress<Cell> base, size_t capacity ) {
      this.base = base;
      this.capacity = capacity;
    }
  public:
    // for static construction
    DHT( ) {}

    static void init_global_DHT( DHT * globally_valid_local_pointer, size_t capacity ) {
      GlobalAddress<Cell> base = Grappa_typed_malloc<Cell>( capacity );

      // TODO: could use on_all_nodes here if use BOOST_PP_COMMA
      // actually I probably want to change the on_all_node macro to a true function call
      DHT<K,V,HF>::DHT_construct_each f(globally_valid_local_pointer, base, capacity );
      fork_join_custom(&f);
    }


    bool lookup ( K key, V * val ) {           // TODO: want lookup to actually cause copy of multiple rows
      uint64_t index = computeIndex( key );
      GlobalAddress< Cell > target = base + index; 

      ld_args args(key, target);
      lookup_result result = Grappa_delegate_func<ld_args, lookup_result, DHT<K,V,HF>::lookup_delegated >( args, target.node() );
      *val = result.val;
      return result.valid;
    } 


    void insert( K key, V val ) {
      uint64_t index = computeIndex( key );
      GlobalAddress< Cell > target = base + index; 

      id_args args(key, val, target);
      Grappa_delegate_func<id_args, ignore_t, DHT<K,V,HF>::insert_delegated >( args, target.node() );  // TODO: async inserts
    }


};

// for naming the types scoped in DHT
#define DHT_TYPE(type) typename DHT<K,V,HF>::type

template <typename K, typename V, uint64_t (*HF)(K)> 
DHT_TYPE(ignore_t) DHT<K,V,HF>::insert_delegated( DHT_TYPE(id_args) args ) {
  std::list<DHT_TYPE(Entry)> * entries = args.cell.pointer()->entries;
  if ( entries == NULL ) {
    entries = new std::list<Entry>();
    args.cell.pointer()->entries = entries;
  }

  Entry e(args.key, args.val);
  entries->push_back( e );

  return 0; // TODO: see "delegate that..."
}

template <typename K, typename V, uint64_t (*HF)(K)> 
DHT_TYPE(lookup_result) DHT<K,V,HF>::lookup_delegated( DHT_TYPE(ld_args) args ) {
  std::list<DHT_TYPE(Entry)> * entries = args.cell.pointer()->entries;
  DHT_TYPE(lookup_result) lr;
  lr.valid = false;
  typename std::list<DHT_TYPE(Entry)>::iterator i;
  for (i = entries->begin(); i!=entries->end(); ++i) {
    Entry e = *i;
    if ( e.key == args.key ) {  // typename K must implement operator==
      lr.val = e.val;
      lr.valid = true;  // TODO: return Multiple not just first match!!
      break;
    }
  }

  return lr;
}


#endif // DHT_HPP
