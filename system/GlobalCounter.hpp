#include "Addressing.hpp"
#include "FlatCombiner.hpp"
#include "LocaleSharedMemory.hpp"
#include "GlobalAllocator.hpp"
#include "Collective.hpp"

namespace Grappa {

class GlobalCounter {
  static const Core MASTER = 0;
public:
  
  struct Master {
    long count;
  } master;
  
  GlobalAddress<GlobalCounter> self;
  
  struct Proxy {
    GlobalCounter* outer;
    long delta;
    
    Proxy(GlobalCounter* outer): outer(outer), delta(0) {}
    Proxy* clone_fresh() { return locale_new<Proxy>(outer); }
    
    void sync() {
      auto s = outer->self;
      auto d = delta;
      delegate::call(MASTER,[s,d]{ s->master.count += d; });
    }
    
    bool is_full() { return false; }
  };
  FlatCombiner<Proxy> comb;
  
  char pad[block_size - sizeof(comb)-sizeof(self)-sizeof(master)];
  
public:
  GlobalCounter(long initial_count = 0): comb(locale_new<Proxy>(this)) {
    master.count = initial_count;
  }
  
  void incr(long d = 1) {
    comb.combine([d](Proxy& p){
      p.delta += d;
    });
  }
  
  long count() {
    auto s = self;
    return delegate::call(MASTER, [s]{ return s->master.count; });
  }
  
  static GlobalAddress<GlobalCounter> create(long initial_count = 0) {
    auto a = mirrored_global_alloc<GlobalCounter>();
    call_on_all_cores([a]{ new (a.localize()) GlobalCounter(0); });
    return a;
  }
  
  void destroy() {
    auto a = self;
    call_on_all_cores([a]{ a->~GlobalCounter(); });
  }
};

} // namespace Grappa
