#ifndef MATCHES_DHT_HPP
#define MATCHES_DHT_HPP

#include <Grappa.hpp>
#include <GlobalAllocator.hpp>
#include <ParallelLoop.hpp>
#include <BufferVector.hpp>
#include <Statistics.hpp>
#include <AsyncDelegate.hpp>
#include <Delegate.hpp>
#include <Reducer.hpp>

#include <vector>

// for all hash tables
GRAPPA_DEFINE_STAT(MaxStatistic<uint64_t>, mh_max_cell_length, 0);
GRAPPA_DEFINE_STAT(SummarizingStatistic<uint64_t>, mh_cell_traversal_length, 0);


// for naming the types scoped in MatchesDHT
#define MDHT_TYPE(type) typename MatchesDHT<K,V,HF,GCE>::type


AllReducer<uint64_t, collective_add> size_reducer(0);


// Hash table for joins
// * allows multiple copies of a Key
// * lookups return all Key matches
template <typename K, typename V, uint64_t (*HF)(K), Grappa::GlobalCompletionEvent * GCE=&Grappa::impl::local_gce> 
class MatchesDHT {

  private:
    struct Entry {
      K key;
      BufferVector<V> * vs;
      Entry( K key ) : key(key), vs(new BufferVector<V>( 16 )) {}
    };

    struct Cell {
      std::vector<Entry> entries;
      char padding[64-sizeof(std::vector<Entry>)];

      Cell() : entries() {
        entries.reserve(2);
      }
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

        uint64_t sum_size = 0;
        // for all keys, set match vector to RO
        int64_t j;
        int64_t sz = c.entries.size();
        for (j = 0; j<sz; ++j) {
          Entry e = c.entries[j];
          e.vs->setReadMode();
          sum_size+=e.vs->getLength();
        }
        //max_cell_length.add(sum_size);
      });
    }

    uint64_t lookup ( K key, GlobalAddress<V> * vals ) {          
      uint64_t index = computeIndex( key );
      GlobalAddress< Cell > target = base + index; 

      lookup_result result = Grappa::delegate::call( target.node(), [key,target]() {

        Cell * c = target.pointer();
        MDHT_TYPE(lookup_result) lr;
        lr.num = 0;

        uint64_t steps = 0;

        int64_t i;
        int64_t sz = c->entries.size();
        for (i = 0; i<sz; ++i) {
          steps+=1;
          const Entry e = c->entries[i];
          if ( e.key == key ) {  // typename K must implement operator==
            lr.matches = e.vs->getReadBuffer();
            lr.num = e.vs->getLength();
            DVLOG(5) << "key " << key << " --> " << lr.num;
            mh_cell_traversal_length += steps;
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
        
        Cell * c = target.pointer();

        // find matching key in the list
        int64_t i;
        int64_t sz = c->entries.size();
        for (i = 0; i<sz; ++i) {
          Entry e = c->entries[i];
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
        c->entries.push_back( newe );

        return 0; 
      });
    }
    
    void insert_async( K key, V val, Grappa::MessagePool& pool ) {
      uint64_t index = computeIndex( key );
      GlobalAddress< Cell > target = base + index; 

      Grappa::delegate::call_async<GCE>( pool, target.node(), [key, val, target]() {   // TODO: upgrade to call_async
        // list of entries in this cell
        Cell * c = target.pointer();

        // find matching key in the list
        int64_t i;
        int64_t sz = c->entries.size();
        for (i = 0; i<sz; i++) {
          Entry e = c->entries[i];
          if ( e.key == key ) {
            // key found so add to matches
            DVLOG(5) << key << " already in so insert " << val;
            e.vs->insert( val );
            return;
          }
        }

        // this is the first time the key has been seen
        // so add it to the list
        DVLOG(5) << key << " new so add for " << val;
        Entry newe( key );
        newe.vs->insert( val );
        c->entries.push_back( newe );
        mh_max_cell_length.add( sz+1 );
      });
    }
   
    
    // FIXME: this is fragile if AsyncDelegate changes or
   // insert_async changes. Relying on writes being same
   // size as insert (you are sending one value in a call)
    size_t insertion_pool_size( uint64_t numInsertions ) {
      return (16+sizeof(Grappa::delegate::write_msg_proxy<K>))*numInsertions;
    }
    
    uint64_t numUniqueKeys() {
      // alternative is running count in insert,
      // but will require always sending the globally_valid_pointer (this)

      Grappa::on_all_cores([] { size_reducer.reset(); });

      Grappa::forall_localized( base, capacity, []( int64_t i, Cell& c ) {
        int64_t j;
        int64_t sz = c.entries.size();
        size_reducer.accumulate(sz);
      });

      Core master = Grappa::mycore();
      uint64_t total;
      Grappa::on_all_cores([&total,master] { 
        uint64_t x = size_reducer.finish();
        if ( Grappa::mycore() == master ) {
          total = x;
        }
      });

      return total;
    }
    
    uint64_t size() {
      // alternative is running count in insert,
      // but will require always sending the globally_valid_pointer (this)

      Grappa::on_all_cores([] { size_reducer.reset(); });

      Grappa::forall_localized( base, capacity, []( int64_t i, Cell& c ) {
        int64_t j;
        int64_t sz = c.entries.size();
        for (j = 0; j<sz; j++) {
          Entry e = c.entries[j];
          size_reducer.accumulate(e.vs->getLength());
        }
          
      });


      Core master = Grappa::mycore();
      uint64_t total;
      Grappa::on_all_cores([&total,master] { 
        uint64_t x = size_reducer.finish();
        if ( Grappa::mycore() == master ) {
          total = x;
        }
      });

      return total;
    }


};


#endif // MATCHES_DHT_HPP
