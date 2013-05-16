#include "Addressing.hpp"
#include "Communicator.hpp"
#include "Collective.hpp"
#include "GlobalAllocator.hpp"
#include "Cache.hpp"
#include "FlatCombiner.hpp"

GRAPPA_DECLARE_STAT(SimpleStatistic<uint64_t>, global_vector_push_ops);
GRAPPA_DECLARE_STAT(SimpleStatistic<uint64_t>, global_vector_push_msgs);

DECLARE_bool(flat_combining);
DECLARE_uint64(global_vector_buffer);

namespace Grappa {
/// @addtogroup Containers
/// @{

const Core MASTER = 0;

template< typename T, int BUFFER_CAPACITY = (1<<10) >
class GlobalVector {
public:
  struct Master {
    size_t offset;
  };
  
  struct Proxy {
    GlobalVector * outer;
    T buffer[BUFFER_CAPACITY];
    size_t n;
    
    Proxy(GlobalVector* const outer): outer(outer), n(0) {}
    Proxy* clone_fresh() { return locale_new<Proxy>(outer); }
    
    bool is_full() { return n == BUFFER_CAPACITY; }
    
    void sync() {
      global_vector_push_msgs++;
      
      auto self = outer->self;
      auto n = this->n;
      VLOG(1) << "self = " << self;
      auto offset = delegate::call(MASTER, [self,n]{
        auto o = self->master.offset;
        VLOG(1) << "master.offset = " << o;
        self->master.offset += n;
        return o;
      });
      VLOG(1) << "\n  offset = " << offset << ", n = " << n << "\n  base = " << outer->base;
      typename Incoherent<T>::WO c(outer->base+offset, n, buffer);
      c.block_until_released();
    }
  };
  
  GlobalAddress<T> base;
  size_t capacity;
  
  GlobalAddress<GlobalVector> self;
  
  Master master;
  FlatCombiner<Proxy> comb;
  
  char _pad[block_size-sizeof(base)-sizeof(capacity)-sizeof(self)-sizeof(master)-sizeof(comb)];
  
public:
  GlobalVector(): comb(locale_new<Proxy>(this)) { master.offset = 0; }
  
  GlobalVector(GlobalAddress<GlobalVector> self, GlobalAddress<T> storage_base, size_t total_capacity)
    : comb(locale_new<Proxy>(this))
  {
    master.offset = 0;
    this->self = self;
    base = storage_base;
    capacity = total_capacity;
  }
  ~GlobalVector() {}
  
  static GlobalAddress<GlobalVector> create(size_t total_capacity) {
    auto base = global_alloc<T>(total_capacity);
    auto self = mirrored_global_alloc<GlobalVector>();
    VLOG(1) << "create:\n  self = " << self << "\n  base = " << base;
    call_on_all_cores([self,base,total_capacity]{
      new (self.localize()) GlobalVector(self, base, total_capacity);
    });
    return self;
  }
  
  void destroy() {
    auto self = this->self;
    global_free(this->base);
    call_on_all_cores([self]{ self->~GlobalVector(); });
    global_free(self);
  }  
  
  /// Push element on the back (queue or stack)
  void push(const T& e) {
    if (FLAGS_flat_combining) {
      this->comb.combine([&e](Proxy& p) {
        global_vector_push_ops++;
        p.buffer[p.n] = e;
        p.n++;
      });
    } else {
      global_vector_push_ops++; global_vector_push_msgs++;
      auto self = this->self;
      auto offset = delegate::call(MASTER, [self]{ return self->master.offset++; });
      delegate::write(this->base+offset, e);
    }
  }
  
  /// Return number of elements currently in vector
  size_t size() { auto self = this->self;
    return delegate::call(MASTER, [self]{ return self->master.offset; });
  }
  
  bool empty() { return size() == 0; }
  
  /// Return a Linear GlobalAddress to the first element of the vector.
  GlobalAddress<T> begin() { return this->base; }
  
  /// Return a Linear GlobalAddress to the end of the vector, that is, one past the last element.
  GlobalAddress<T> end() { return this->base + size(); }
  
  void clear() { auto self = this->self; delegate::call(MASTER, [self]{ self->master.offset = 0; }); }
  
  GlobalAddress<T> storage() { return this->base; }
  
};

/// @}
} // namespace Grappa
