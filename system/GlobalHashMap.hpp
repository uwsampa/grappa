////////////////////////////////////////////////////////////////////////
// Copyright (c) 2010-2015, University of Washington and Battelle
// Memorial Institute.  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//     * Redistributions of source code must retain the above
//       copyright notice, this list of conditions and the following
//       disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials
//       provided with the distribution.
//     * Neither the name of the University of Washington, Battelle
//       Memorial Institute, or the names of their contributors may be
//       used to endorse or promote products derived from this
//       software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// UNIVERSITY OF WASHINGTON OR BATTELLE MEMORIAL INSTITUTE BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
////////////////////////////////////////////////////////////////////////

#pragma once

#include "GlobalAllocator.hpp"
#include "ParallelLoop.hpp"
#include "Metrics.hpp"
#include "FlatCombiner.hpp"
#include <utility>
#include <unordered_map>
#include <vector>

GRAPPA_DECLARE_METRIC(SimpleMetric<size_t>, hashmap_insert_ops);
GRAPPA_DECLARE_METRIC(SimpleMetric<size_t>, hashmap_insert_msgs);
GRAPPA_DECLARE_METRIC(SimpleMetric<size_t>, hashmap_lookup_ops);
GRAPPA_DECLARE_METRIC(SimpleMetric<size_t>, hashmap_lookup_msgs);


namespace Grappa {

template< typename K, typename V > 
class GlobalHashMap {
public:
  struct Entry {
    K key;
    V val;
    Entry( K key ) : key(key), val() {}
    Entry( K key, V val): key(key), val(val) {}
  };
  
  struct ResultEntry {
    bool found;
    ResultEntry * next;
    V val;
  };
  
public:
  struct Cell { // TODO: keep first few in the 64-byte block, then go to secondary storage
    std::vector<Entry> entries;
    
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
  } GRAPPA_BLOCK_ALIGNED;

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
    auto self = symmetric_global_alloc<GlobalHashMap>();
    call_on_all_cores([self,base,total_capacity]{
      new (self.localize()) GlobalHashMap(self, base, total_capacity);
    });
    forall(base, total_capacity, [](int64_t i, Cell& c) { new (&c) Cell(); });
    return self;
  }
  
  GlobalAddress<Cell> begin() { return this->base; }
  size_t ncells() { return this->capacity; }
  
  void clear() {
    forall(base, capacity, [](Cell& c){ c.clear(); });
  }
  
  void destroy() {
    auto self = this->self;
    forall(this->base, this->capacity, [](Cell& c){ c.~Cell(); });
    global_free(this->base);
    call_on_all_cores([self]{ self->~GlobalHashMap(); });
    global_free(self);
  }
  
  template< typename F >
  void forall_entries(F visit) {
    forall(base, capacity, [visit](int64_t i, Cell& c){
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
    
} GRAPPA_BLOCK_ALIGNED;

template< SyncMode S = SyncMode::Blocking,
          GlobalCompletionEvent * C = &impl::local_gce,
          typename K = nullptr_t, typename V = nullptr_t,
          typename F = nullptr_t >
void insert(GlobalAddress<GlobalHashMap<K,V>> self, K key, F on_insert) {
  ++hashmap_insert_msgs;
  delegate::call<S,C>(self->base+self->computeIndex(key),
  [=](typename GlobalHashMap<K,V>::Cell& c){
    for (auto& e : c.entries) {
      if (e.key == key) {
        on_insert(e.val);
        return;
      }
    }
    c.entries.emplace_back(key);
    on_insert(c.entries.back().val);
  });
}

template< GlobalCompletionEvent * GCE = &impl::local_gce,
          int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG,
          typename T = decltype(nullptr),
          typename V = decltype(nullptr),
          typename F = decltype(nullptr) >
void forall(GlobalAddress<GlobalHashMap<T,V>> self, F visit) {
  forall<GCE,Threshold>(self->begin(), self->ncells(),
  [visit](typename GlobalHashMap<T,V>::Cell& c){
    for (auto& e : c.entries) {
      visit(e.key, e.val);
    }
  });
}

} // namespace Grappa
