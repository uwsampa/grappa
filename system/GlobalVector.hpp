#include "Addressing.hpp"
#include "Communicator.hpp"
#include "Collective.hpp"
#include "GlobalAllocator.hpp"

namespace Grappa {
/// @addtogroup Containers
/// @{

template< typename T, Core MASTER_CORE = 0 >
class GlobalVector {
protected:
  struct Master {
    size_t offset;
  } master;
  
  struct Shared {
    size_t max_elems;
    GlobalAddress<GlobalVector> self;
    GlobalAddress<T> base;    
  } shared;
  
  char pad[block_size-sizeof(Shared)-sizeof(Master)];
  
  void _init(Shared s) {
    shared = s;
    master.offset = 0;
  }
  
public:
  GlobalVector() {}
  
  static GlobalAddress<GlobalVector> create(size_t max_elems) {
    static_assert(sizeof(GlobalVector) % block_size == 0,
                  "must pad global proxy to multiple of block_size");
    // allocate enough space that we are guaranteed to get one on each core at same location
    auto qac = global_alloc<char>(cores()*(sizeof(GlobalVector)+block_size));
    while (qac.core() != MASTER_CORE) qac++;
    auto qa = static_cast<GlobalAddress<GlobalVector>>(qac);
    CHECK_EQ(qa, qa.block_min());
    CHECK_EQ(qa.core(), MASTER_CORE);
    
    // intialize all cores' local versions
    Shared s;
    s.self = qa;
    s.max_elems = max_elems;
    s.base = global_alloc<T>(max_elems);    
    call_on_all_cores([s]{ s.self->_init(s); });

    return qa;
  }
  
  static void destroy(GlobalAddress<GlobalVector> q) {
    global_free(q->shared.base);
    global_free(q);
  }
    
  void push(T e) {
    if (mycore() == MASTER_CORE) {
      auto offset = master.offset;
      master.offset++;
      delegate::write(shared.base+offset, e);
    } else {
      auto self = shared.self;
      auto offset = delegate::call(MASTER_CORE, [self]{
        return self->master.offset++;
      });
      delegate::write(shared.base+offset, e);
    }
  }
  
  GlobalAddress<T> storage() { return shared.base; }
};

/// @}
} // namespace Grappa
