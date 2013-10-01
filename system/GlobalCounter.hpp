#include "Addressing.hpp"
#include "FlatCombiner.hpp"
#include "LocaleSharedMemory.hpp"
#include "GlobalAllocator.hpp"
#include "Collective.hpp"

namespace Grappa {

class GlobalCounter {
public:
  
  struct Master {
    long count;
    Core core;
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
      delegate::call(outer->master.core, [s,d]{ s->master.count += d; });
    }
    
    void clear() { delta = 0; }
    bool is_full() { return false; }
  };
  FlatCombiner<Proxy> comb;
    
  GlobalCounter(long initial_count = 0, Core master_core = 0): comb(locale_new<Proxy>(this)) {
    master.count = initial_count;
    master.core = master_core;
  }
  
  void incr(long d = 1) {
    comb.combine([d](Proxy& p){
      p.delta += d;
      return FCStatus::BLOCKED;
    });
  }
  
  long count() {
    auto s = self;
    return delegate::call(master.core, [s]{ return s->master.count; });
  }
  
  static GlobalAddress<GlobalCounter> create(long initial_count = 0) {
    auto a = mirrored_global_alloc<GlobalCounter>();
    auto master_core = mycore();
    call_on_all_cores([a,master_core]{ new (a.localize()) GlobalCounter(0, master_core); });
    return a;
  }
  
  void destroy() {
    auto a = self;
    call_on_all_cores([a]{ a->~GlobalCounter(); });
  }
} GRAPPA_BLOCK_ALIGNED;

} // namespace Grappa
