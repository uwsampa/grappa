#pragma once

#include "Grappa.hpp"
#include "GlobalAllocator.hpp"
#include "ParallelLoop.hpp"
#include "BufferVector.hpp"
#include "Statistics.hpp"
#include "FlatCombiner.hpp"
#include <utility>
#include <unordered_map>
#include <vector>

// for all hash tables
//GRAPPA_DEFINE_STAT(MaxStatistic<uint64_t>, max_cell_length, 0);

GRAPPA_DECLARE_STAT(SimpleStatistic<size_t>, hashmap_insert_ops);
GRAPPA_DECLARE_STAT(SimpleStatistic<size_t>, hashmap_insert_msgs);
GRAPPA_DECLARE_STAT(SimpleStatistic<size_t>, hashmap_lookup_ops);
GRAPPA_DECLARE_STAT(SimpleStatistic<size_t>, hashmap_lookup_msgs);


namespace Grappa {

// Hash table for joins
// * allows multiple copies of a Key
// * lookups return all Key matches
template< typename K, typename V > 
class GlobalHashTable {
protected:
  struct Entry {
    K key;
    V val;
    Entry( K key ) : key(key), val() {}
    Entry( K key, V val): key(key), val(val) {}
  };
  
  struct Cell { // TODO: keep first few in the 64-byte block, then go to secondary storage
    std::vector<Entry> entries;
    char padding[64-sizeof(std::vector<Entry>)];
    
    Cell() : entries() { entries.reserve(10); }
    
    void clear() { entries.clear(); }
    
    std::pair<bool,V> lookup(K key) {
      
      for (auto& e : this->entries) if (e.key == key) return std::pair<bool,K>{true, e.val};
      return std::pair<bool,K>(false,V());
    }
    void insert(const K& key, const V& val) {
      bool found = false;
      for (auto& e : entries) {
        if (e.key == key) {
          e.val = val;
          found = true;
          break;
        }
      }
      if (!found) {
        entries.emplace_back(key, val);
      }
    }
  };

  struct Proxy {
    static const size_t LOCAL_HASH_SIZE = 1<<10;
    
    GlobalHashTable * owner;
    std::unordered_map<K,V> map;
    
    Proxy(GlobalHashTable * owner): owner(owner), map(LOCAL_HASH_SIZE) { clear(); }
    
    void clear() { map.clear(); }
    
    Proxy * clone_fresh() { return locale_new<Proxy>(owner); }
    
    bool is_full() { return map.size() >= LOCAL_HASH_SIZE; }
    
    void insert(const K& newk, const V& newv) {
      if (map.count(newk) == 0) {
        map[newk] = newv;
      }
    }
    
    void sync() {
      CompletionEvent ce(map.size());
      auto cea = make_global(&ce);
      
      for (auto& e : map) { auto& k = e.first; auto& v = e.second;
        ++hashmap_insert_msgs;
        auto cell = owner->base+owner->computeIndex(k);
        send_heap_message(cell.core(), [cell,cea,k,v]{
          cell->insert(k, v);
          complete(cea);
        });
      }
      ce.wait();
    }
  };

  // private members
  GlobalAddress<GlobalHashTable> self;
  GlobalAddress< Cell > base;
  size_t capacity;
  
  FlatCombiner<Proxy> proxy;

  char _pad[2*block_size - sizeof(self)-sizeof(base)-sizeof(capacity)-sizeof(proxy)];

  uint64_t computeIndex(K key) {
    static std::hash<K> hasher;
    return hasher(key) % capacity;
  }

  // for creating local GlobalHashTable
  GlobalHashTable( GlobalAddress<GlobalHashTable> self, GlobalAddress<Cell> base, size_t capacity )
    : self(self), base(base), capacity(capacity)
    , proxy(locale_new<Proxy>(this))
  {
    CHECK_LT(sizeof(self)+sizeof(base)+sizeof(capacity)+sizeof(proxy), 2*block_size);
  }
  
public:
  // for static construction
  GlobalHashTable( ) {}
  
  static GlobalAddress<GlobalHashTable> create(size_t total_capacity) {
    auto base = global_alloc<Cell>(total_capacity);
    auto self = mirrored_global_alloc<GlobalHashTable>();
    call_on_all_cores([self,base,total_capacity]{
      new (self.localize()) GlobalHashTable(self, base, total_capacity);
    });
    forall_localized(base, total_capacity, [](int64_t i, Cell& c) { new (&c) Cell(); });
    return self;
  }
  
  void clear() {
    forall_localized(base, capacity, [](Cell& c){ c.clear(); });
  }
  
  void destroy() {
    auto self = this->self;
    forall_localized(this->base, this->capacity, [](Cell& c){ c.~Cell(); });
    global_free(this->base);
    call_on_all_cores([self]{ self->~GlobalHashTable(); });
    global_free(self);
  }
  
  template< typename F >
  void forall_entries(F visit) {
    forall_localized(base, capacity, [visit](int64_t i, Cell& c){
      // if cell is not hit then no action
      if (c.entries == nullptr) return;
      DVLOG(3) << "c<" << &c << "> entries:" << c.entries << " size: " << c.entries->size();
      for (Entry& e : *c.entries) {
        visit(e.key, *e.val);
      }
    });
  }
  
  bool lookup(K key, V * val) {
    ++hashmap_lookup_ops;
    std::pair<bool,V> result;
    if (FLAGS_flat_combining) {
      proxy.combine([&result,key,this](Proxy& p){
        if (p.map.count(key) > 0) {
          result = std::make_pair(true, p.map[key]);
        } else {
          ++hashmap_lookup_msgs;
          auto cell = this->base+this->computeIndex(key);
          result = delegate::call(cell, [key](Cell* c){ return c->lookup(key); });
        }
      });
    } else {
      ++hashmap_lookup_msgs;
      auto result = delegate::call(base+computeIndex(key), [key](Cell* c){
        return c->lookup(key);
      });
    }
    *val = result.second;
    return result.first;
  }
  
  void insert(K key, V val) {
    ++hashmap_insert_ops;
    if (FLAGS_flat_combining) {
      proxy.combine([key,val](Proxy& p){ p.map[key] = val; });
    } else {
      ++hashmap_insert_msgs;
      delegate::call(base+computeIndex(key), [key,val](Cell * c) { c->insert(key,val); });
    }
  }

  template< GlobalCompletionEvent * GCE, int64_t Threshold,
            typename TT, typename VV, typename F >
  friend void forall_localized(GlobalAddress<GlobalHashTable<TT,VV>> self, F func);  
  
};

template< GlobalCompletionEvent * GCE = &impl::local_gce,
          int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG,
          typename T = decltype(nullptr),
          typename V = decltype(nullptr),
          typename F = decltype(nullptr) >
void forall_localized(GlobalAddress<GlobalHashTable<T,V>> self, F visit) {
  forall_localized<GCE,Threshold>(self->base, self->capacity,
  [visit](typename GlobalHashTable<T,V>::Cell& c){
    for (auto& e : c.entries) {
      visit(e.key, e.val);
    }
  });
}

} // namespace Grappa
