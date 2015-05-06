#ifndef MATCHES_DHT_PG_HPP
#define MATCHES_DHT_PG_HPP

#include <Grappa.hpp>
#include <GlobalAllocator.hpp>
#include <Metrics.hpp>

#include <list>
#include <cmath>
#include <tuple>


//GRAPPA_DECLARE_METRIC(MaxMetric<uint64_t>, max_cell_length);
GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, hash_tables_size);
GRAPPA_DECLARE_METRIC(SummarizingMetric<uint64_t>, hash_tables_lookup_steps);

GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, hash_remote_lookups);
GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, hash_remote_inserts);
GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, hash_local_lookups);
GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, hash_local_inserts);
GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, hash_called_lookups);
GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, hash_called_inserts);


// for naming the types scoped in MatchesDHT
#define MDHT_TYPE(type) typename MatchesDHT<K,V,Hash>::type

// Hash table for joins
// * allows multiple copies of a Key
// * lookups return all Key matches
template <typename K, typename V, typename Hash> 
class MatchesDHT_pg {


  private:
    struct Entry {
      K key;
      std::vector<V> * vs;
      Entry( K key ) : key(key), vs(new std::vector<V>()) {}
      Entry ( ) {}
    };

    struct ListNode {
      Entry data;
      GlobalAddress<ListNode> next;

      ListNode(Entry data, GlobalAddress<ListNode> next) : data(data), next(next) {}
    };

    struct Cell {
      GlobalAddress<ListNode> entries;

      Cell() : entries( make_global(NULL) ) {}
    };

    struct lookup_result {
      GlobalAddress<V> matches;
      size_t num;
    };
    
    // private members
    GlobalAddress< FullEmpty<Cell> > base;
    size_t capacity;

    size_t computeIndex( K key ) {
      return Hash()(key) & (capacity - 1);
    }

    // for creating local MatchesDHT
    MatchesDHT_pg( GlobalAddress<Cell> base, uint32_t capacity_pow2 ) {
      this->base = base;
      this->capacity = capacity_pow2;
    }

  public:
    // for static construction
    MatchesDHT_pg( ) {}

    static void init_global_DHT( MatchesDHT<K,V,Hash> * globally_valid_local_pointer, size_t capacity ) {

      uint32_t capacity_exp = log2(capacity);
      size_t capacity_powerof2 = pow(2, capacity_exp);
      GlobalAddress<Cell> base = Grappa::global_alloc<FullEmpty<Cell>>( capacity_powerof2 );

      Grappa::on_all_cores( [globally_valid_local_pointer,base,capacity_powerof2] {
        *globally_valid_local_pointer = MatchesDHT<K,V,Hash>( base, capacity_powerof2 );
      });

      Grappa::forall( base, capacity_powerof2, []( int64_t i, FullEmpty& c ) {
        Cell empty;
        c.writeXF(empty);
      });
    }


    std::tuple< size_t, GlobalAddress<std::vector<V>>> lookup_get_size( K key ) {
      auto index = computeIndex( key );
      GlobalAddress< FullEmpty<Cell> > target = base + index;

      auto cell = readFF(target);
      auto lnp = cell.entries;

      if (lnp.pointer() == NULL) return 0;

      while (true) {
        ListNode ln = Grappa::delegate::read(lnp);
        if (ln.data.key == key) {
          // found the matching key so get list of tuples size
          auto r = Grappa::delegate::call<sync>(target.core(), [lnp, val] {
            return std::make_tuple(lnp.pointer().data.vs->size(), make_global(lnp.pointer.data.vs));
          });
          return r;
        } else {
          if (lnp.next.pointer() == NULL) {
            // no matching keys so size 0
            auto r = std::make_tuple(0, make_global(target.core(), NULL));
            return r;
          }
          lnp = ln.next;
        }
      }
    }

    V lookup_put_get( GlobalAddress<std::vector<V>> vsp, int64_t cursor ) {
      return Grappa::delegate::call( vsp.core(), [vsp, cursor] {
        return vsp.pointer()->at(cursor);
      });
    }

  // This "put/get" implementation of insert currently goes everything
  // with read/write except creation of and insertion into vectors
  void insert_put_get( K key, V val ) {
    auto index = computeIndex(key);
    GlobalAddress <FullEmpty<Cell>> target = base + index;

    // get the cell
    // LOCK
    auto cell = readFE(target);

    // if it is empty then allocate a list
    if (cell.entries.pointer() == NULL) {
      // allocate
      auto newe = Grappa::delegate::call<sync>(target.core(), [key] {
        return newe(key);
      });

      // add entry to join tuples
      Grappa::delegate::call<sync>(target.core(), [val, newe] {
        newe.vs->push_back(val);
      });

      // add new entry as head of the cell list
      // TODO: could be async delegate
      Grappa::delegate::call<sync>(target.core(), [target, newe] {
        target.pointer()->entries = Grappa::make_global(new ListNode(newe, NULL));
      });

      // UNLOCK
      writeXF(target, cell);
      return;
    }

    // traverse the list of colliding entries at this cell
    GlobalAddress <ListNode> lnp = cell.entries;
    while (true) {
      ListNode ln = Grappa::delegate::read(lnp);
      if (ln.data.key == key) {
        // found the matching key so just insert into join tuples
        // TODO could be async
        Grappa::delegate::call<sync>(target.core(), [lnp, val] {
          lnp.pointer().data.vs->push_back(val);
        });

        // UNLOCK (TODO: could be earlier, before push_back)
        writeXF(target, cell);
        return;
      } else {
        if (lnp.next.pointer() == NULL) {
          // no matching keys so we will insert a new entry...
          break;
        }
        lnp = ln.next;
      }
    }

    // allocate
    auto newe = Grappa::delegate::call<sync>(target.core(), [key] {
      return newe(key);
    });

    // add entry to join tuples
    Grappa::delegate::call<sync>(target.core(), [val, newe] {
      newe.vs->push_back(val);
    });

    // add new entry as tail of cell list
    Grappa::delegate::call<sync>(target.core(), [target, newe] {
      lnp.pointer().next = new ListNode(newe, make_global(NULL));
    });

    // UNLOCK
    writeXF(target, cell);
    return;
  }

};


#endif // MATCHES_DHT_PG_HPP
