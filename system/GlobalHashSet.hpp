#pragma once

#include "Grappa.hpp"
#include "GlobalAllocator.hpp"
#include "ParallelLoop.hpp"
#include "AsyncDelegate.hpp"
#include "BufferVector.hpp"
#include "Statistics.hpp"
#include "Array.hpp"

#include <list>

// for all hash tables
// GRAPPA_DECLARE_STAT(MaxStatistic<uint64_t>, max_cell_length);
GRAPPA_DECLARE_STAT(SummarizingStatistic<uint64_t>, cell_traversal_length);

namespace Grappa {

// Hash table for joins
// * allows multiple copies of a Key
// * lookups return all Key matches
template <typename K, uint64_t (*HF)(K)> 
class GlobalHashSet {
protected:
  struct Entry {
    K key;
    Entry(){}
    Entry(K key) : key(key) {}
  };
  
  struct Cell {
    std::vector<Entry> entries;
    char padding[64-sizeof(std::vector<Entry>)];
    Cell() { entries.reserve(16); }
  };

  // private members
  GlobalAddress<GlobalHashSet> self;
  GlobalAddress< Cell > base;
  size_t capacity;

  char _pad[block_size - sizeof(self)-sizeof(base)-sizeof(capacity)];

  uint64_t computeIndex( K key ) {
    return HF(key) % capacity;
  }

  // for creating local GlobalHashSet
  GlobalHashSet( GlobalAddress<GlobalHashSet> self, GlobalAddress<Cell> base, size_t capacity ) {
    this->self = self;
    this->base = base;
    this->capacity = capacity;
  }
  
public:
  // for static construction
  GlobalHashSet( ) {}
  
  static GlobalAddress<GlobalHashSet> create(size_t total_capacity) {
    auto base = global_alloc<Cell>(total_capacity);
    auto self = mirrored_global_alloc<GlobalHashSet>();
    call_on_all_cores([self,base,total_capacity]{
      new (self.localize()) GlobalHashSet(self, base, total_capacity);
    });
    forall_localized(base, total_capacity, [](int64_t i, Cell& c) { new (&c) Cell(); });
    return self;
  }
  
  void destroy() {
    auto self = this->self;
    forall_localized(this->base, this->capacity, [](int64_t i, Cell& c){
      if (c.entries != nullptr) { delete c.entries; }
    });
    global_free(this->base);
    call_on_all_cores([self]{ self->~GlobalHashSet(); });
    global_free(self);
  }
  
  bool lookup ( K key ) {
    uint64_t index = computeIndex( key );
    GlobalAddress< Cell > target = base + index; 
    
    return delegate::call(target, [key](Cell* c) {
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

    return Grappa::delegate::call( target, [key](Cell * c) {
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
      c->entries.emplace_back( key );
      VLOG(1) << "[" << key << "] size:" << c->entries.size() << ", last:" << c->entries.back().key;
      return false; 
    });
  }

  // Inserts the key if not already in the set
  //
  // asynchronous operation
  template< GlobalCompletionEvent * GCE = impl::local_gce >
  void insert_async( K key ) {
    uint64_t index = computeIndex( key );
    GlobalAddress< Cell > target = base + index; 

    delegate::call_async<GCE>(shared_pool, target.node(), [key,target] {
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
      }
      cell_traversal_length+=steps;
      
      // this is the first time the key has been seen
      // so add it to the list
      c->entries.emplace_back( key );
      // max_cell_length.add( sz+1 );
   });
  }

};

} // namespace Grappa
