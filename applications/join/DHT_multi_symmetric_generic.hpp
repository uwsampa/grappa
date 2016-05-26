#pragma once

#include <Grappa.hpp>
#include <GlobalAllocator.hpp>
#include <GlobalCompletionEvent.hpp>
#include <ParallelLoop.hpp>
#include <Metrics.hpp>

#include <cmath>
#include <unordered_map>
#include <vector>


GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, dht_inserts);

// for naming the types scoped in DHT_multi_symmetric_generic
#define DHT_multi_symmetric_generic_TYPE(type) typename DHT_multi_symmetric_generic<K,VL,VP,Hash>::type
#define DHT_multi_symmetric_generic_T DHT_multi_symmetric_generic<K,VL,VR,Hash>

// Hash table for joins
// * allows multiple copies of a Key
// * lookups return all Key matches
template <typename K, typename VL, typename VR, typename Hash> 
class DHT_multi_symmetric_generic {
  private:
    struct PairCell {
      VL only_left;
      VR only_right;
      std::vector<VL> * left;
      std::vector<VR> * right;
      bool occupied_left;
      bool occupied_right;

      PairCell() : left(NULL), right(NULL), occupied_left(false), occupied_right(false) { }
    };

    // private members
    GlobalAddress< DHT_multi_symmetric_generic_T > self;
    std::unordered_map<K, PairCell, Hash > * local_map;
    size_t partitions;

    size_t computeIndex( K key ) {
      return Hash()(key) % partitions;
    }

    // for creating local DHT_multi_symmetric_generic
    DHT_multi_symmetric_generic( GlobalAddress<DHT_multi_symmetric_generic_T> self ) 
      : self(self)
      , partitions(Grappa::cores())
      , local_map(new std::unordered_map<K,PairCell,Hash>())
      {}

  public:
    // for static construction
    DHT_multi_symmetric_generic( ) {}

    static GlobalAddress<DHT_multi_symmetric_generic_T> create_DHT_symmetric( ) {
      auto object = Grappa::symmetric_global_alloc<DHT_multi_symmetric_generic_T>();

      Grappa::on_all_cores( [object] {
        new(object.pointer()) DHT_multi_symmetric_generic_T(object);
      });
      
      return object;
    }

    template<Grappa::GlobalCompletionEvent * GCE>
    void insert_left(K key, VL val) {
      auto index = computeIndex(key);
      auto target = this->self;
      Grappa::delegate::call<async, GCE>( index, [key, val, target]() {
        auto& slot = (*(target->local_map))[key];
        //// if right is NULL then no need to insert any on the left; requires right->left insert scheduling
        //if (slot.right != NULL) {
          // avoid dynamic allocation if key is unique
          if (slot.occupied_left) {
            if (slot.left == NULL) {
                slot.left = new std::vector<VL>();
                slot.left->push_back(slot.only_left);
                VLOG(4) << "new list (left) " << slot.left;
            }
            slot.left->push_back(val);
         } else {
          slot.only_left = val; 
          slot.occupied_left = true;
        }
       // }
      });
    }
    
    template<Grappa::GlobalCompletionEvent * GCE>
    void insert_right(K key, VR val) {
      auto index = computeIndex(key);
      auto target = this->self;
      Grappa::delegate::call<async, GCE>( index, [key, val, target]() {
        auto& slot = (*(target->local_map))[key];
        // avoid dynamic allocation if key is unique
        if (slot.occupied_right) {
          if (slot.right == NULL) {
              slot.right = new std::vector<VR>();
              slot.right->push_back(slot.only_right);
              VLOG(4) << "new list (right) " << slot.right;
          }
          slot.right->push_back(val);
        } else {
          slot.only_right = val;
          slot.occupied_right = true;
        }
      });
    }

    // alternative to generator pattern where state is explicit
    class MatchesIterator { 
      private:
        DHT_multi_symmetric_generic_T * dht;
        decltype(dht->local_map->begin()) entry_iter;
        PairCell current_cell;
        uint64_t left_ind;
        uint64_t right_ind;
        
      public:
        MatchesIterator(DHT_multi_symmetric_generic_T * dht) 
          : dht(dht)
          , entry_iter(dht->local_map->begin())
          , current_cell()
          , left_ind(0)
          , right_ind(0) { 
}

        bool next(std::pair<VL,VR>& result) {
          while (true) {
            if (!current_cell.occupied_left || !current_cell.occupied_right) {
              // ready for next cell if either side is empty
              if (entry_iter != dht->local_map->end()) {
                current_cell = entry_iter->second;
                ++entry_iter;
              } else {
                // reached the end
                return false;
              }
            } else {
              // get next match
              if ((current_cell.left==NULL && left_ind==0) 
                  || (current_cell.left != NULL && (left_ind < current_cell.left->size()))) {
                // get left tuple
                VL l;
                if (current_cell.left==NULL) { l = current_cell.only_left; } 
                else { l = (*(current_cell.left))[left_ind]; }

                if ((current_cell.right==NULL && right_ind==0)
                    || (current_cell.right != NULL && (right_ind < current_cell.right->size()))) {
                  // get right tuple
                  VR r;
                  if (current_cell.right==NULL) { r = current_cell.only_right; }
                  else { r = (*(current_cell.right))[right_ind]; }

                  result = std::make_pair(l, r);
                  right_ind++;
                  return true;
                } else {
                  left_ind++;
                  right_ind=0;
                }
              } else {
                // done with this cell
                left_ind=0;
                CHECK( right_ind == 0 ) << "unexpected index value right"; 
                current_cell = PairCell();
              }
            }
          }
        } 
    };
    
    MatchesIterator * matches() {
      return new MatchesIterator(this);
    }

    void delete_matches(MatchesIterator * p) {
      delete p;
    }

} GRAPPA_BLOCK_ALIGNED;

