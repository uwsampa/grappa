#pragma once

#include <Grappa.hpp>
#include <GlobalAllocator.hpp>
#include <ParallelLoop.hpp>
#include <Statistics.hpp>
#include <Reducer.hpp>
#include <MessagePool.hpp>

#include <vector>

// for all hash tables
//GRAPPA_DEFINE_STAT(MaxStatistic<uint64_t>, max_cell_length, 0);
GRAPPA_DEFINE_STAT(SummarizingStatistic<uint64_t>, cell_traversal_length, 0);

// for naming the types scoped in HashSet
#define HS_TYPE(type) typename HashSet<K,HF,GCE>::type


AllReducer<uint64_t, collective_add> size_reducer(0);

// Hash table for joins
// * allows multiple copies of a Key
// * lookups return all Key matches
template <typename K, uint64_t (*HF)(K), Grappa::GlobalCompletionEvent * GCE=&Grappa::impl::local_gce> // TODO: pick GCE on insert call 
class HashSet {

  private:
    struct Entry {
      K key;
      Entry(){}//default for list
      Entry(K key) : key(key) {}
    };

    struct Cell {
      std::vector<Entry> entries;
      char padding[64-sizeof(std::vector<Entry>)];

      Cell() : entries(2) {}
    };

    // private members
    GlobalAddress< Cell > base;
    size_t capacity;

    uint64_t computeIndex( K key ) {
      return HF(key) % capacity;
    }

    // for creating local HashSet
    HashSet( GlobalAddress<Cell> base, size_t capacity ) {
      this->base = base;
      this->capacity = capacity;
    }
    
  public:
    // for static construction
    HashSet( ) {}

    static void init_global_DHT( HashSet<K,HF,GCE> * globally_valid_local_pointer, size_t capacity ) {
      GlobalAddress<Cell> base = Grappa_typed_malloc<Cell>( capacity );

      Grappa::on_all_cores( [globally_valid_local_pointer,base,capacity] {
        *globally_valid_local_pointer = HashSet<K,HF,GCE>( base, capacity );
      });

      Grappa::forall_localized( base, capacity, []( int64_t i, Cell& c ) {
        new(&c) Cell();
      });
    }

    bool lookup ( K key ) {          
      uint64_t index = computeIndex( key );
      GlobalAddress< Cell > target = base + index; 

      return Grappa::delegate::call( target.node(), [key,target]() {

        Cell * c = target.pointer();

        int64_t i;
        int64_t sz = c->entries.size();
        for (i = 0; i<sz; ++i) {
          if ( c->entries[i].key == key ) {  // typename K must implement operator==
            return true;
          }
        }

        return false;
      });
    } 

    // Inserts the key if not already in the set
    //
    // returns true if the set already contains the key
    //
    // synchronous operation
    bool insert( K key ) {
      uint64_t index = computeIndex( key );
      GlobalAddress< Cell > target = base + index; 

      return Grappa::delegate::call( target.node(), [key, target]() {   // TODO: have an additional version that returns void
                                                                 // to upgrade to call_async
        Cell * c = target.pointer();

        // find matching key in the list
        int64_t i;
        int64_t sz = c->entries.size();
        for (i = 0; i<sz; ++i) {
          Entry e = c->entries[i];
          if ( e.key == key ) {
            // key found so no insert
            return true;
          }
        }

        // this is the first time the key has been seen
        // so add it to the list
        Entry newe( key );        // TODO: cleanup since sharing insert* code here, we are just going to store an empty vector
                                  // perhaps a different module
        c->entries.push_back( newe );

        return false; 
     });
    }
    

    // Inserts the key if not already in the set
    //
    // asynchronous operation
    void insert_async( K key, Grappa::MessagePool& pool ) {
      uint64_t index = computeIndex( key );
      GlobalAddress< Cell > target = base + index; 

      Grappa::delegate::call_async<GCE>(pool, target.node(), [key,target]() {

        Cell * c = target.pointer();
        
        uint64_t steps=0;
        
        // find matching key in the list
        int64_t i;
        int64_t sz = c->entries.size();
        for (i = 0; i<sz; i++) {
          steps+=1;
          Entry e = c->entries[i];
          if ( e.key == key ) {
            cell_traversal_length+=steps;
            return;
          }

      //     return;
        }
        cell_traversal_length+=steps;
        
     
        // this is the first time the key has been seen
        // so add it to the list
        Entry newe( key );        
        c->entries.push_back( newe );
        //max_cell_length.add( sz+1 );
     });
    }

    uint64_t size() {
      // alternative is running count in insert,
      // but will require always sending the globally_valid_pointer (this)

      Grappa::on_all_cores([] { size_reducer.reset(); });

      Grappa::forall_localized( base, capacity, []( int64_t i, Cell& c ) {
        size_reducer.accumulate(c.entries.size());
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

   
   // FIXME: this is fragile if AsyncDelegate changes or
   // insert_async changes. Relying on writes being same
   // size as insert (you are sending one value in a call)
    size_t insertion_pool_size( uint64_t numInsertions ) {
      return sizeof(Grappa::delegate::write_msg_proxy<K>)*numInsertions;
    }

};// </HashTable>

