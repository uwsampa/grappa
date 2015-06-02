#ifndef MATCHES_DHT_PG_HPP
#define MATCHES_DHT_PG_HPP

#include <Grappa.hpp>
#include <GlobalAllocator.hpp>
#include <Metrics.hpp>
#include <FullEmpty.hpp>
#include <Addressing.hpp>

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

GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, fer_in);
GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, fer_out);



// for naming the types scoped in MatchesDHT_pg
#define MDHT_TYPE(type) typename MatchesDHT_pg<K,V,Hash>::type

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

      ListNode(Entry data, GlobalAddress<ListNode> next=make_global((ListNode*)NULL)) : data(data), next(next) {}
      ListNode() { }
    };

    template <typename T>
    static bool is_null(GlobalAddress<T> p) {
      return p.pointer() == NULL;
    }

    struct Cell {
      GlobalAddress<ListNode> entries;
      std::vector<GlobalAddress<FullEmpty<Cell>>> waiters; // TODO: don't want to communicate this

      Cell() : entries( make_global((ListNode*)NULL) ) , waiters() {}
    };

    struct lookup_result {
      GlobalAddress<V> matches;
      size_t num;
    };
    
    // private members
    GlobalAddress<Cell> base;
    size_t capacity;

    size_t computeIndex( K key ) {
      return Hash()(key) & (capacity - 1);
    }

    // for creating local MatchesDHT_pg
    MatchesDHT_pg( GlobalAddress<Cell> base, uint32_t capacity_pow2 ) {
      this->base = base;
      this->capacity = capacity_pow2;
    }

  public:
    // for static construction
    MatchesDHT_pg( ) {}

    static void init_global_DHT( MatchesDHT_pg<K,V,Hash> * globally_valid_local_pointer, size_t capacity ) {

      uint32_t capacity_exp = log2(capacity);
      size_t capacity_powerof2 = pow(2, capacity_exp);
      auto base = Grappa::global_alloc<Cell>( capacity_powerof2 );

      Grappa::on_all_cores( [globally_valid_local_pointer,base,capacity_powerof2] {
        *globally_valid_local_pointer = MatchesDHT_pg<K,V,Hash>( base, capacity_powerof2 );
      });

      Grappa::forall( base, capacity_powerof2, []( int64_t i, Cell& c ) {
        Cell empty;
        c = empty;
      });
    }


    std::tuple< size_t, GlobalAddress<std::vector<V>>> lookup_get_size( K key ) {
      //VLOG(5) << "called lookup_get_size " << key;
      auto index = computeIndex( key );
      GlobalAddress< Cell> target = base + index;

      auto cell = Grappa::delegate::read(target);
      auto lnp = cell.entries;

      if (is_null(lnp)) return std::make_tuple(0, make_global((std::vector<V>*)NULL, target.core()));

      while (true) {
        ListNode ln = Grappa::delegate::read(lnp);
        //VLOG(5) << "looking for " << key << "; got " << ln.data.key;
        if (ln.data.key == key) {
          // found the matching key so get list of tuples size
          auto r = Grappa::delegate::call(target.core(), [lnp] {
            return std::make_tuple(lnp.pointer()->data.vs->size(), make_global(lnp.pointer()->data.vs));
          });
          return r;
        } else {
          if (is_null(ln.next)) {
            // no matching keys so size 0
            auto r = std::make_tuple(0, make_global((std::vector<V>*)NULL, target.core()));
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

  void readCell(GlobalAddress<Cell> target) {
    FullEmpty<Cell> ce;
    auto ce_addr = make_global(&ce);
    auto cell = Grappa::send_heap_message(target.core(), [=] {
        if (target->locked) {
          target->waiters.push_back(ce_addr);
        } else {
          target->locked = true;
          auto cell = *(target->pointer());
          Grappa::send_heap_message(ce_addr.core(), [=] {
            ce_addr->writeXF(cell);
          });
        }
    });
  }

  void writeCell(GlobalAddress<Cell> target, const Cell& c) {
    Grappa::send_heap_message(target.core(), [=] {
        if (target->waiters.size() >= 1) {
          auto w = target->waiters.back();
          target->waiters.pop_back();
          Grappa::send_heap_message(w.core(), [=] {
            w->writeXF(c);
          });
        }
     });
  }


  // This "put/get" implementation of insert currently goes everything
  // with read/write except creation of and insertion into vectors
  void insert_put_get( K key, V val ) {
    auto index = computeIndex(key);
    GlobalAddress <Cell> target = base + index;

    // get the cell
    // LOCK
    fer_in++;
    auto cell = readCell(target); 
    fer_out++;

    // if it is empty then allocate a list
    if (is_null(cell.entries)) {
      // allocate
      auto newe = Grappa::delegate::call(target.core(), [key] {
        return Entry(key);
      });

      // add entry to join tuples
      Grappa::delegate::call(target.core(), [val, newe] {
        newe.vs->push_back(val);
      });

      // add new entry as head of the cell list
      auto lnp = Grappa::delegate::call(target.core(), [target, newe] {
        return make_global(new ListNode(newe));
      });
      // just writing to the cell here since we need writeXF unlock anyway
      Cell newcell;
      newcell.entries = lnp;
      // UNLOCK
      writeCell(target, newcell);
      //VLOG(5) << "empty cell: added new entry " << newe.key << " " << newe.vs;
      return;
    }

    // traverse the list of colliding entries at this cell
    GlobalAddress <ListNode> lnp = cell.entries;
    while (true) {
      ListNode ln = Grappa::delegate::read(lnp);
      if (ln.data.key == key) {
        // found the matching key so just insert into join tuples
        // TODO could be async if writeXF is before it
        Grappa::delegate::call(target.core(), [lnp, val] {
          lnp.pointer()->data.vs->push_back(val);
        });

        // UNLOCK (TODO: could be earlier, before push_back)
        while_in++;
        writeCell(target, cell);
        while_out++;

        return;
      } else {
        if (is_null(ln.next)) {
          // no matching keys so we will insert a new entry...
          break;
        }
        lnp = ln.next;
      }
    }

    // allocate
    auto newe = Grappa::delegate::call(target.core(), [key] {
      return Entry(key);
    });

    // add entry to join tuples
    Grappa::delegate::call(target.core(), [val, newe] {
      newe.vs->push_back(val);
    });

    // add new entry as tail of cell list
    Grappa::delegate::call(target.core(), [lnp, newe] {
      lnp.pointer()->next = make_global(new ListNode(newe));
    });

    // UNLOCK
    writeCell(target, cell);
    return;
  }

};


#endif // MATCHES_DHT_PG_HPP
