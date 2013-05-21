#pragma once

#include "Grappa.hpp"
#include "GlobalAllocator.hpp"
#include "ParallelLoop.hpp"
#include "AsyncDelegate.hpp"
#include "BufferVector.hpp"
#include "Statistics.hpp"
#include "Array.hpp"
#include "FlatCombiner.hpp"

#include <vector>
#include <unordered_set>

// for all hash tables
// GRAPPA_DECLARE_STAT(MaxStatistic<uint64_t>, max_cell_length);
GRAPPA_DECLARE_STAT(SummarizingStatistic<uint64_t>, cell_traversal_length);

GRAPPA_DECLARE_STAT(SimpleStatistic<size_t>, hashset_insert_ops);
GRAPPA_DECLARE_STAT(SimpleStatistic<size_t>, hashset_insert_msgs);

namespace Grappa {

// Hash table for joins
// * allows multiple copies of a Key
// * lookups return all Key matches
template <typename K>
          // size_t NREQUESTS = 128, size_t HASH_SIZE = NREQUESTS<<3 >
class GlobalHashSet {
protected:
  struct Entry {
    K key;
    Entry(){}
    Entry(K key) : key(key) {}
  };
  
  struct Cell { // TODO: keep first few in the 64-byte block, then go to secondary storage
    std::vector<Entry> entries;
    char padding[64-sizeof(std::vector<Entry>)];
    Cell() { entries.reserve(16); }
  };

  struct Proxy {
    GlobalHashSet * owner;
    // size_t reqs[NREQUESTS];
    // size_t nreq;
    std::unordered_set<K> keys_to_insert; // K keys_to_insert[HASH_SIZE];
    
    Proxy(GlobalHashSet * owner): owner(owner) {}
    
    Proxy * clone_fresh() { return locale_new<Proxy>(owner); }
    
    bool is_full() { return false; }
    
    void insert(const K& newk) {
      ++hashset_insert_ops;
      if (keys_to_insert.count(newk) == 0) {
        keys_to_insert.insert(newk);
      }
    }
    
    void sync() {
      CompletionEvent ce(keys_to_insert.size());
      auto cea = make_global(&ce);
      
      for (auto& k : keys_to_insert) {
        ++hashset_insert_msgs;
        auto cell = owner->base+owner->computeIndex(k);
        send_heap_message(cell.core(), [cell,k,cea]{
          Cell * c = cell.localize();
          bool found = false;
          for (auto& e : c->entries) if (e.key == k) { found = true; break; }
          if (!found) c->entries.emplace_back(k);
          complete(cea);
        });
      }
      ce.wait();
    }
  };

  // private members
  GlobalAddress<GlobalHashSet> self;
  GlobalAddress< Cell > base;
  size_t capacity;
  
  size_t count; // not maintained, just need storage for size()
  
  FlatCombiner<Proxy> proxy;
  
  char _pad[block_size - sizeof(self)-sizeof(base)-sizeof(capacity)-sizeof(proxy)-sizeof(count)];

  uint64_t computeIndex( K key ) {
    static std::hash<K> hasher;
    return hasher(key) % capacity;
  }

  // for creating local GlobalHashSet
  GlobalHashSet( GlobalAddress<GlobalHashSet> self, GlobalAddress<Cell> base, size_t capacity )
    : self(self), base(base), capacity(capacity)
    , proxy(locale_new<Proxy>(this))
  { }
  
public:
  
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
    forall_localized(this->base, this->capacity, [](Cell& c){ c.~Cell(); });
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
  void insert( K key ) {
    if (FLAGS_flat_combining) {
      proxy.combine([key](Proxy& p){ p.insert(key); });
    } else {
      ++hashset_insert_ops;
      ++hashset_insert_msgs;
      delegate::call(base+computeIndex(key), [key](Cell * c) {
        // find matching key in the list, if found, no insert necessary
        for (auto& e : c->entries) if (e.key == key) return;
        
        // this is the first time the key has been seen so add it to the list
        c->entries.emplace_back( key );
        return; 
      });
    }
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

  template< GlobalCompletionEvent * GCE = &impl::local_gce, typename F >
  void forall_keys(F visit) {
    forall_localized<GCE>(base, capacity, [visit](int64_t i, Cell& c){
      for (auto& e : c.entries) {
        visit(e.key);
      }
    });
  }
  
  size_t size() {
    auto self = this->self;
    call_on_all_cores([self]{ self->count = 0; });
    forall_keys([self](K& k){ self->count++; });
    on_all_cores([self]{ self->count = allreduce<size_t,collective_add>(self->count); });
    return count;
  }
};

} // namespace Grappa
