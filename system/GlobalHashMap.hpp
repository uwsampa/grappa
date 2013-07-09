#pragma once

#include "Grappa.hpp"
#include "GlobalAllocator.hpp"
#include "ParallelLoop.hpp"
#include "Statistics.hpp"
#include "FlatCombiner.hpp"
#include <utility>
#include <unordered_map>
#include <vector>

GRAPPA_DECLARE_STAT(SimpleStatistic<size_t>, hashmap_insert_ops);
GRAPPA_DECLARE_STAT(SimpleStatistic<size_t>, hashmap_insert_msgs);
GRAPPA_DECLARE_STAT(SimpleStatistic<size_t>, hashmap_lookup_ops);
GRAPPA_DECLARE_STAT(SimpleStatistic<size_t>, hashmap_lookup_msgs);


namespace Grappa {

template< typename K, typename V > 
class GlobalHashMap {
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
  
  struct ResultEntry {
    bool found;
    ResultEntry * next;
    V val;
  };

  struct Proxy {
    static const size_t LOCAL_HASH_SIZE = 1<<10;
    
    GlobalHashMap * owner;
    std::unordered_map<K,V> map;
    std::unordered_map<K,ResultEntry*> lookups;
    
    Proxy(GlobalHashMap * owner): owner(owner)
      , map(LOCAL_HASH_SIZE)
      , lookups(LOCAL_HASH_SIZE)
    {
      clear();
    }
    
    void clear() { map.clear(); lookups.clear(); }
    
    Proxy * clone_fresh() { return locale_new<Proxy>(owner); }
    
    bool is_full() {
      return map.size() >= LOCAL_HASH_SIZE
          || lookups.size() >= LOCAL_HASH_SIZE;
    }
    
    void insert(const K& newk, const V& newv) {
      if (map.count(newk) == 0) {
        map[newk] = newv;
      }
    }
    
    void sync() {
      CompletionEvent ce(map.size()+lookups.size());
      auto cea = make_global(&ce);
      
      for (auto& e : map) { auto& k = e.first; auto& v = e.second;
        ++hashmap_insert_msgs;
        auto cell = owner->base+owner->computeIndex(k);
        send_heap_message(cell.core(), [cell,cea,k,v]{
          cell->insert(k, v);
          complete(cea);
        });
      }
      
      for (auto& e : lookups) { auto k = e.first;
        ++hashmap_lookup_msgs;
        auto re = e.second;
        DVLOG(3) << "lookup " << k << " with re = " << re;
        auto cell = owner->base+owner->computeIndex(k);
        
        send_heap_message(cell.core(), [cell,k,cea,re]{
          Cell * c = cell.localize();
          bool found = false;
          V val;
          for (auto& e : c->entries) if (e.key == k) {
            found = true;
            val = e.val;
            break;
          }
          send_heap_message(cea.core(), [cea,re,found,val]{
            ResultEntry * r = re;
            while (r != nullptr) {
              r->found = found;
              r->val = val;
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
  GlobalAddress<GlobalHashMap> self;
  GlobalAddress< Cell > base;
  size_t capacity;
  
  FlatCombiner<Proxy> proxy;

  char _pad[2*block_size - sizeof(self)-sizeof(base)-sizeof(capacity)-sizeof(proxy)];

  uint64_t computeIndex(K key) {
    static std::hash<K> hasher;
    return hasher(key) % capacity;
  }

  // for creating local GlobalHashMap
  GlobalHashMap( GlobalAddress<GlobalHashMap> self, GlobalAddress<Cell> base, size_t capacity )
    : self(self), base(base), capacity(capacity)
    , proxy(locale_new<Proxy>(this))
  {
    CHECK_LT(sizeof(self)+sizeof(base)+sizeof(capacity)+sizeof(proxy), 2*block_size);
  }
  
public:
  // for static construction
  GlobalHashMap( ) {}
  
  static GlobalAddress<GlobalHashMap> create(size_t total_capacity) {
    auto base = global_alloc<Cell>(total_capacity);
    auto self = mirrored_global_alloc<GlobalHashMap>();
    call_on_all_cores([self,base,total_capacity]{
      new (self.localize()) GlobalHashMap(self, base, total_capacity);
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
    call_on_all_cores([self]{ self->~GlobalHashMap(); });
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
    if (FLAGS_flat_combining) {
      ResultEntry re{false,nullptr};
      DVLOG(3) << "lookup[" << key << "] = " << &re;
      
      proxy.combine([&re,key](Proxy& p){
        // if (p.map.count(key) > 0) {
        //   re.found = true;
        //   re.val = p.map[key];
        //   return FCStatus::SATISFIED;
        // } else {
          if (p.lookups.count(key) == 0) p.lookups[key] = nullptr;
          re.next = p.lookups[key];
          p.lookups[key] = &re;
          DVLOG(3) << "p.lookups[" << key << "] = " << &re;
          return FCStatus::BLOCKED;
        // }
      });
      *val = re.val;
      return re.found;
    } else {
      ++hashmap_lookup_msgs;
      auto result = delegate::call(base+computeIndex(key), [key](Cell* c){
        return c->lookup(key);
      });
      *val = result.second;
      return result.first;
    }
  }
  
  void insert(K key, V val) {
    ++hashmap_insert_ops;
    if (FLAGS_flat_combining) {
      proxy.combine([key,val](Proxy& p){ p.map[key] = val; return FCStatus::BLOCKED; });
    } else {
      ++hashmap_insert_msgs;
      delegate::call(base+computeIndex(key), [key,val](Cell * c) { c->insert(key,val); });
    }
  }

  template< GlobalCompletionEvent * GCE, int64_t Threshold,
            typename TT, typename VV, typename F >
  friend void forall_localized(GlobalAddress<GlobalHashMap<TT,VV>> self, F func);  
  
};

template< GlobalCompletionEvent * GCE = &impl::local_gce,
          int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG,
          typename T = decltype(nullptr),
          typename V = decltype(nullptr),
          typename F = decltype(nullptr) >
void forall_localized(GlobalAddress<GlobalHashMap<T,V>> self, F visit) {
  forall_localized<GCE,Threshold>(self->base, self->capacity,
  [visit](typename GlobalHashMap<T,V>::Cell& c){
    for (auto& e : c.entries) {
      visit(e.key, e.val);
    }
  });
}

} // namespace Grappa
