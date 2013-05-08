#include "Addressing.hpp"
#include "Communicator.hpp"
#include "Collective.hpp"
#include "GlobalAllocator.hpp"
#include "Cache.hpp"

GRAPPA_DECLARE_STAT(SimpleStatistic<uint64_t>, global_vector_push_ops);
GRAPPA_DECLARE_STAT(SimpleStatistic<uint64_t>, global_vector_push_msgs);

DECLARE_string(flat_combining);
DECLARE_uint64(global_vector_buffer);


namespace Grappa {
/// @addtogroup Containers
/// @{

template< typename T, Core MASTER_CORE = 0 >
class GlobalVector {
public:
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
    bool to_be_sent;
    Worker * sender;
    
    PushCombiner(GlobalVector* owner, size_t capacity)
      : owner(owner)
      , buffer(locale_alloc<T>(capacity))
      , capacity(capacity)
      , n(0)
      , to_be_sent(false)
      , sender(nullptr)
    { }
    
    ~PushCombiner() {
      locale_free(buffer);
    }
    
    bool has_waiters() { return cv.waiters_ != 0; }
    
    void push(const T& e) {
      global_vector_push_ops++;
      buffer[n] = e;
      n++;
      if (n == capacity || owner->inflight == nullptr) {
        owner->inflight = this;
        owner->push_combiner = new PushCombiner(owner, capacity);
        if (sender == nullptr) {
          flush();
        } // otherwise someone else is already assigned and will send when ready
      } else {
        Grappa::wait(&cv);
        if (&current_worker() == sender) {
          if (this == owner->push_combiner) owner->push_combiner = new PushCombiner(owner, capacity);
          flush();
        }
      }
    }
    
    void flush() {
      auto ta = make_global(this);
      this->sender = &current_worker(); // (if not set already)
      global_vector_push_msgs++;
      
      auto self = owner->shared.self;
      auto nelem = this->n;
      auto offset = delegate::call(MASTER_CORE, [self,ta,nelem]{
        auto o = self->master.offset;
        self->master.offset += nelem;
        return o;
      });
      typename Incoherent<T>::WO c(owner->shared.base+offset, n, buffer);
      c.block_until_released();
      broadcast(&cv); // wake our people
      if (owner->push_combiner->has_waiters()) {
        // atomically claim it so no one else tries to send in the meantime
        owner->inflight = owner->push_combiner;
        // wake someone and tell them to send
        owner->inflight->sender = impl::get_waiters(&owner->inflight->cv);
        signal(&owner->inflight->cv);
      } else {
        owner->inflight = nullptr;
      }
      this->sender = nullptr;
      delete this;
    }
    
  } *push_combiner, *inflight;
  
  char pad[block_size-sizeof(shared)-sizeof(master)-sizeof(push_combiner)*2];

  GlobalVector(Shared s, size_t buffer_capacity) {
    shared = s;
    master.offset = 0;
    push_combiner = new PushCombiner(this, buffer_capacity);
    inflight = nullptr;
  }
  ~GlobalVector() { delete push_combiner; if (inflight) delete inflight; }
  
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
  
  void destroy() {
    auto q = shared.self;
    global_free(q->shared.base);
    call_on_all_cores([q]{ q->~GlobalVector(); });
    global_free(q);
  }
  
  /// Return number of elements currently in vector
  size_t size() {
    auto self = shared.self;
    return delegate::call(MASTER_CORE, [self]{ return self->master.offset; });
  }
  
  size_t capacity() { return shared.capacity; }

  bool empty() { return size() == 0; }

  /// Return a Linear GlobalAddress to the first element of the vector.
  GlobalAddress<T> begin() { return shared.base; }
  
  /// Return a Linear GlobalAddress to the end of the vector, that is, one past the last element.
  GlobalAddress<T> end() { return shared.base + size(); }
  
  void clear() {
    auto self = shared.self;
    return delegate::call(MASTER_CORE, [self]{ self->master.offset = 0; });
  }
  
  /// Push element on the back (queue or stack)
  void push(T e) {
    push_combiner->push(e);
  }
  
  GlobalAddress<T> storage() { return shared.base; }
};

/// @}
} // namespace Grappa
