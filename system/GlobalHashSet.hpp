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
#include <unordered_map>

GRAPPA_DECLARE_STAT(SummarizingStatistic<uint64_t>, cell_traversal_length);

GRAPPA_DECLARE_STAT(SimpleStatistic<size_t>, hashset_insert_ops);
GRAPPA_DECLARE_STAT(SimpleStatistic<size_t>, hashset_insert_msgs);
GRAPPA_DECLARE_STAT(SimpleStatistic<size_t>, hashset_lookup_ops);
GRAPPA_DECLARE_STAT(SimpleStatistic<size_t>, hashset_lookup_msgs);

namespace Grappa {

template <typename K>
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

  struct ResultEntry {
    bool result;
    ResultEntry * next;
  };

  struct Proxy {
    static const size_t LOCAL_HASH_SIZE = 1<<10;
    
    GlobalHashSet * owner;
    std::unordered_set<K> keys_to_insert;
    std::unordered_map<K,ResultEntry*> lookups;
    
    Proxy(GlobalHashSet * owner): owner(owner)
      , keys_to_insert(LOCAL_HASH_SIZE)
      , lookups(LOCAL_HASH_SIZE)
    { }
    
    void clear() { keys_to_insert.clear(); lookups.clear(); }
    
    Proxy * clone_fresh() { return locale_new<Proxy>(owner); }
    
    bool is_full() {
      return keys_to_insert.size() >= LOCAL_HASH_SIZE
          || lookups.size() >= LOCAL_HASH_SIZE;
    }
    
    void insert(const K& newk) {
      if (keys_to_insert.count(newk) == 0) {
        keys_to_insert.insert(newk);
      }
    }

    void sync() {
      CompletionEvent ce(keys_to_insert.size()+lookups.size());
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
      for (auto& e : lookups) { auto k = e.first;
        ++hashset_lookup_msgs;
        auto re = e.second;
        DVLOG(3) << "lookup " << k << " with re = " << re;
        auto cell = owner->base+owner->computeIndex(k);
        
        send_heap_message(cell.core(), [cell,k,cea,re]{
          Cell * c = cell.localize();
          bool found = false;
          for (auto& e : c->entries) if (e.key == k) { found = true; break; }
          
          send_heap_message(cea.core(), [cea,re,found]{
            ResultEntry * r = re;
            while (r != nullptr) {
              r->result = found;
              r = r->next;
            }
            complete(cea);
          });
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
    ++hashset_lookup_ops;
    if (FLAGS_flat_combining) {
      ResultEntry re{false,nullptr};
      DVLOG(3) << "lookup[" << key << "] = " << &re;
      
      proxy.combine([&re,key,this](Proxy& p){
        if (p.keys_to_insert.count(key) > 0) {
          re.result = true;
          return true;
        } else {
          if (p.lookups.count(key) == 0) p.lookups[key] = nullptr;
          re.next = p.lookups[key];
          p.lookups[key] = &re;
          DVLOG(3) << "p.lookups[" << key << "] = " << &re;
          return false;
        }
      });
      return re.result;
    } else {
      ++hashset_lookup_msgs;
      return delegate::call(base+computeIndex(key), [key](Cell* c){
        for (auto& e : c->entries) if (e.key == key) return true;
        return false;
      });
    }
  }


  // Inserts the key if not already in the set
  //
  // returns true if the set already contains the key
  //
  // synchronous operation
  void insert( K key ) {
    ++hashset_insert_ops;
    if (FLAGS_flat_combining) {
      proxy.combine([key](Proxy& p){ p.insert(key); return false; });
    } else {
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
  template< typename F >
  void insert_async( K key, F sync) {
    ++hashset_insert_ops;
    proxy->insert(key);
    if (proxy->is_full()) {
      ++hashset_insert_msgs;
      privateTask([this,sync]{
        this->proxy.combine([](Proxy& p){ return false; });
        sync();
      });
    } else {
      sync();
    }
  }
  
  void sync_all_cores() {
    auto self = this->self;
    on_all_cores([self]{
      self->proxy.combine([](Proxy& p){ return false; });
    });
  }
  
  template< GlobalCompletionEvent * GCE = &impl::local_gce, typename F = decltype(nullptr) >
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

