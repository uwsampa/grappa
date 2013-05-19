#pragma once

#include "Grappa.hpp"
#include "GlobalAllocator.hpp"
#include "ParallelLoop.hpp"
#include "BufferVector.hpp"
#include "Statistics.hpp"

#include <list>

// for all hash tables
//GRAPPA_DEFINE_STAT(MaxStatistic<uint64_t>, max_cell_length, 0);

namespace Grappa {

// Hash table for joins
// * allows multiple copies of a Key
// * lookups return all Key matches
template <typename K, typename V, uint64_t (*HF)(K)> 
class GlobalHashTable {
protected:
  struct Entry {
    K key;
    BufferVector<V> * vs;
    Entry( K key ) : key(key), vs(new BufferVector<V>( 16 )) {}
    Entry( K key, BufferVector<V> * vs ): key(key), vs(vs) {}
    ~Entry() { if (vs != nullptr) delete vs; }
  };
  
  struct Cell {
    std::list<Entry> * entries;
    Cell() : entries( nullptr ) {}
  };

  // private members
  GlobalAddress<GlobalHashTable> self;
  GlobalAddress< Cell > base;
  size_t capacity;

  char _pad[block_size - sizeof(self)-sizeof(base)-sizeof(capacity)];

  uint64_t computeIndex( K key ) {
    return HF(key) % capacity;
  }

  // for creating local GlobalHashTable
  GlobalHashTable( GlobalAddress<GlobalHashTable> self, GlobalAddress<Cell> base, size_t capacity ) {
    this->self = self;
    this->base = base;
    this->capacity = capacity;
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
  
  void destroy() {
    auto self = this->self;
    forall_localized(this->base, this->capacity, [](int64_t i, Cell& c){
      if (c.entries != nullptr) { delete c.entries; }
    });
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
        visit(e.key, *e.vs);
      }
    });
  }

  void global_set_RO() { forall_entries([](K k, BufferVector<V>& vs){ VLOG(1) << "vs = " << &vs; vs.setReadMode(); }); }
  void global_set_WO() { forall_entries([](K k, BufferVector<V>& vs){ vs.setWriteMode(); }); }
  
  uint64_t lookup ( K key, GlobalAddress<V> * vals ) {          
    uint64_t index = computeIndex( key );
    GlobalAddress< Cell > target = base + index; 
    
    struct lookup_result {
      GlobalAddress<V> matches;
      size_t num;
    };

    auto result = delegate::call(target, [key](Cell* c) {
      lookup_result lr;
      lr.num = 0;

      // first check if the cell has any entries;
      // if not then the lookup returns nothing
      if (c->entries == nullptr) return lr;
      
      for (Entry& e : *c->entries) {
        if ( e.key == key ) {  // typename K must implement operator==
          lr.matches = e.vs->getReadBuffer();
          lr.num = e.vs->size();
          break;
        }
      }

      return lr;
    });

    *vals = result.matches;
    return result.num;
  }

  // Inserts the key if not already in the set
  // Shouldn't be used with `insert`.
  //
  // returns true if the set already contains the key
  bool insert_unique( K key ) {
    uint64_t index = computeIndex( key );
    GlobalAddress< Cell > target = base + index; 
    return delegate::call( target, [key](Cell* c) {
      // TODO: have an additional version that returns void to upgrade to call_async

      // if first time the cell is hit then initialize
      if ( c->entries == nullptr ) {
        c->entries = new std::list<Entry>();
      }

      // find matching key in the list
      for (Entry& e : *c->entries) {
        if ( e.key == key ) {
          // key found so no insert
          return true;
        }
      }

      // this is the first time the key has been seen so add it to the list
      // TODO: cleanup since sharing insert* code here, we are just going to store an empty vector perhaps a different module
      c->entries->emplace_back(key,nullptr);

      return false;
    });
  }


  void insert( K key, V val ) {
    uint64_t index = computeIndex( key );
    GlobalAddress< Cell > target = base + index; 

    delegate::call(target, [key, val](Cell* c) { // TODO: upgrade to call_async
      // list of entries in this cell

      // if first time the cell is hit then initialize
      if ( c->entries == nullptr ) {
        c->entries = new std::list<Entry>();
      }

      // find matching key in the list
      for (Entry& e : *c->entries) {
        if ( e.key == key ) {
          // key found so add to matches
          e.vs->insert( val );
          return;
        }
      }

      // this is the first time the key has been seen
      // so add it to the list
      c->entries->emplace_back(key);
      c->entries->back().vs->insert(val);
    });
  }

};

} // namespace Grappa
