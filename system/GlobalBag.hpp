#include <Grappa.hpp>

namespace Grappa {
  
  /// Global unordered queue with local insert and iteration.
  /// 
  /// Useful for situations where intermediate values may be produced 
  /// from anywhere and iterated over later. We use this mostly for 
  /// places like BFS's "frontier", where things to process next phase
  /// are stored in a bag so they can be processed without communicating.
  template< typename T >
  class GlobalBag {
    GlobalAddress<GlobalBag> self;
    T* l_storage;
    size_t l_size;
    size_t l_capacity;
  public:
    GlobalBag(): l_storage(nullptr), l_size(0) {}
    GlobalBag(GlobalAddress<GlobalBag> self, size_t n):
      self(self), l_storage(new T[n]), l_size(0), l_capacity(n) {}
      ~GlobalBag() { if (l_storage) delete[] l_storage; }
    
    static GlobalAddress<GlobalBag> create(size_t total_capacity) {
      auto self = symmetric_global_alloc<GlobalBag>();
      auto n = total_capacity / cores()
               + total_capacity % cores();
      call_on_all_cores([=]{
        new (self.localize()) GlobalBag(self, n);
      });
      return self;
    }
    
    void destroy() {
      auto self = this->self;
      call_on_all_cores([self]{ self->~GlobalBag(); });
      global_free(self);
    }
    
    void add(const T& o) {
      CHECK_LT(l_size, l_capacity);
      l_storage[l_size] = o;
      l_size++;
    }
    
    void clear() {
      auto b = self;
      call_on_all_cores([=]{
        for (T& e : util::iterate(b->l_storage, b->l_size)) e.~T();
        b->l_size = 0;
      });
    }
    
    size_t local_size() { return l_size; }
    
    size_t size() {
      auto b = self;
      return sum_all_cores([=]{ return b->l_size; });
    }
    
    bool empty() { return size() == 0; }
    
    template< SyncMode S = SyncMode::Blocking,
              GlobalCompletionEvent * C = &impl::local_gce,
              int64_t Th = impl::USE_LOOP_THRESHOLD_FLAG,
              typename F = nullptr_t >
    static void impl_iterator(GlobalAddress<GlobalBag> b, F body) {
      on_all_cores([=]{
        Grappa::forall_here<TaskMode::Bound,SyncMode::Async,C,Th>(0, b->l_size, [=](int64_t i){
          body(b->l_storage[i]);
        });
      });
      if (S == SyncMode::Blocking && C) C->wait();
    }
    
  } GRAPPA_BLOCK_ALIGNED;
    
  template< SyncMode S = SyncMode::Blocking,
            GlobalCompletionEvent * C = &impl::local_gce,
            int64_t Th = impl::USE_LOOP_THRESHOLD_FLAG,
            typename F = nullptr_t, typename T >
  void forall(GlobalAddress<GlobalBag<T>> b, F body) {
    GlobalBag<T>::template impl_iterator<S,C,Th>(b, body);
  }
}