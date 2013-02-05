#ifndef MATCHES_DHT_HPP
#define MATCHES_DHT_HPP

#include "Grappa.hpp"
#include "GlobalAllocator.hpp"
#include "ForkJoin.hpp"
#include "BufferVector.hpp"

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
      GlobalAddress<const V> matches;
      size_t num;
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

    struct construct_each : ForkJoinIteration {
      MatchesDHT<K,V,HF> * loc;
      GlobalAddress<Cell> base;
      size_t capacity;

      construct_each( MatchesDHT<K,V,HF> * loc, GlobalAddress<Cell> base, size_t capacity ) 
          : loc(loc), base(base), capacity(capacity) {}
      construct_each() {}
        
      inline void operator() (int64_t nid) const {
        *loc = MatchesDHT<K,V,HF>( base, capacity );
      }
    };

    static void init_storage( Cell * cell ) {
      Cell c;
      *cell = c;
    }

    static void setRO( Cell * cell ) {
      // list of entries in this cell
      std::list<MDHT_TYPE(Entry)> * entries = cell->entries;

      // if cell is not hit then no action
      if ( entries == NULL ) {
        return;
      }

      // for all keys, set match vector to RO
      typename std::list<MDHT_TYPE(Entry)>::iterator i;
      for (i = entries->begin(); i!=entries->end(); ++i) {
        Entry e = *i;
        e.vs->setReadMode();
      }
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

      // TODO: could use on_all_nodes here if use BOOST_PP_COMMA
      // actually I probably want to change the on_all_node macro to a true function call
      MatchesDHT<K,V,HF>::construct_each f(globally_valid_local_pointer, base, capacity );
      fork_join_custom(&f);

      // initialize all cells to be empty
      forall_local<Cell, MatchesDHT<K,V,HF>::init_storage>( globally_valid_local_pointer->base, globally_valid_local_pointer->capacity );
    }

    static void set_RO_global( MatchesDHT<K,V,HF> * globally_valid_local_pointer ) {
      forall_local<Cell, MatchesDHT<K,V,HF>::setRO>( globally_valid_local_pointer->base, globally_valid_local_pointer->capacity );
    }


    uint64_t lookup ( K key, GlobalAddress<V> * vals ) {           // TODO: want lookup to actually cause copy of multiple rows
      uint64_t index = computeIndex( key );
      GlobalAddress< Cell > target = base + index; 

      ld_args args(key, target);
      lookup_result result = Grappa_delegate_func<ld_args, lookup_result, MatchesDHT<K,V,HF>::lookup_delegated >( args, target.node() );
      *vals = result.matches;
      return result.num;
    } 


    void insert( K key, V val ) {
      uint64_t index = computeIndex( key );
      GlobalAddress< Cell > target = base + index; 

      id_args args(key, val, target);
      Grappa_delegate_func<id_args, ignore_t, MatchesDHT<K,V,HF>::insert_delegated >( args, target.node() );  // TODO: async inserts
    }


};


template <typename K, typename V, uint64_t (*HF)(K)> 
MDHT_TYPE(ignore_t) MatchesDHT<K,V,HF>::insert_delegated( MDHT_TYPE(id_args) args ) {
  // list of entries in this cell
  std::list<MDHT_TYPE(Entry)> * entries = args.cell.pointer()->entries;

  // if first time the cell is hit then initialize
  if ( entries == NULL ) {
    entries = new std::list<Entry>();
    args.cell.pointer()->entries = entries;
  }

  // find matching key in the list
  typename std::list<MDHT_TYPE(Entry)>::iterator i;
  for (i = entries->begin(); i!=entries->end(); ++i) {
    Entry e = *i;
    if ( e.key == args.key ) {
      // key found so add to matches
      e.vs->insert( args.val );
      return 0;
    }
  }

  // this is the first time the key has been seen
  // so add it to the list
  Entry newe( args.key );
  newe.vs->insert( args.val );
  entries->push_back( newe );

  return 0; // TODO: see "delegate that..."
}

template <typename K, typename V, uint64_t (*HF)(K)> 
MDHT_TYPE(lookup_result) MatchesDHT<K,V,HF>::lookup_delegated( MDHT_TYPE(ld_args) args ) {
  std::list<MDHT_TYPE(Entry)> * entries = args.cell.pointer()->entries;
  MDHT_TYPE(lookup_result) lr;
  lr.num = 0;

  // first check if the cell has any entries;
  // if not then the lookup returns nothing
  if ( entries == NULL ) return lr;

  typename std::list<MDHT_TYPE(Entry)>::iterator i;
  for (i = entries->begin(); i!=entries->end(); ++i) {
    const Entry e = *i;
    if ( e.key == args.key ) {  // typename K must implement operator==
      lr.matches = e.vs->getReadBuffer();
      lr.num = e.vs->getLength();
      break;
    }
  }

  return lr;
}


#endif // MATCHES_DHT_HPP
