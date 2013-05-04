#include "Addressing.hpp"
#include "Communicator.hpp"
#include "Collective.hpp"
#include "GlobalAllocator.hpp"
#include "Cache.hpp"

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
    size_t capacity;
    GlobalAddress<GlobalVector> self;
    GlobalAddress<T> base;    
  } shared;
  
  struct PushCombiner {
    GlobalVector* owner;
    T* buffer;
    size_t capacity;
    size_t n;
    ConditionVariable cv;
    
    PushCombiner(GlobalVector* owner, size_t capacity)
      : owner(owner)
      , buffer(locale_alloc<T>(capacity))
      , capacity(capacity)
      , n(0) 
    { }
    
    ~PushCombiner() {
      VLOG(1) << "freeing PushCombiner";
      locale_free(buffer);
    }
    
    void push(const T& e) {
      buffer[n] = e;
      n++;
      if (n == capacity) {
        owner->push_combiner = new PushCombiner(owner, capacity);
        flush();
        broadcast(&cv);
        delete this;
      } else {
        Grappa::wait(&cv);
      }
    }
    
    void flush() {
      auto self = owner->shared.self;
      auto offset = delegate::call(MASTER_CORE, [self]{ return self->master.offset++; });
      typename Incoherent<T>::WO c(owner->shared.base+offset, n, buffer);
      c.block_until_released();
    }
    
  } *push_combiner;
  
  char pad[block_size-sizeof(shared)-sizeof(master)-sizeof(push_combiner)];

  GlobalVector(Shared s, size_t buffer_capacity) {
    shared = s;
    master.offset = 0;
    push_combiner = new PushCombiner(this, buffer_capacity);
  }
  ~GlobalVector() { delete push_combiner; }
  
public:
  
  static GlobalAddress<GlobalVector> create(size_t total_capacity, size_t buffer_capacity = 1L<<16) {
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
    s.capacity = total_capacity;
    s.base = global_alloc<T>(total_capacity);
    call_on_all_cores([s,buffer_capacity]{
      new (s.self.localize()) GlobalVector(s, buffer_capacity);
    });

    return qa;
  }
  
  static void destroy(GlobalAddress<GlobalVector> q) {
    call_on_all_cores([q]{ q->~GlobalVector(); });
    global_free(q->shared.base);
    global_free(q);
  }
    
  void push(T e) {
    push_combiner->push(e);
    // if (mycore() == MASTER_CORE) {
    //   auto offset = master.offset;
    //   master.offset++;
    //   delegate::write(shared.base+offset, e);
    // } else {
    //   auto self = shared.self;
    //   auto offset = delegate::call(MASTER_CORE, [self]{
    //     return self->master.offset++;
    //   });
    //   delegate::write(shared.base+offset, e);
    // }
  }
  
  GlobalAddress<T> storage() { return shared.base; }
};

/// @}
} // namespace Grappa
