#pragma once

#include "Communicator.hpp"
#include "Addressing.hpp"
#include "GlobalAllocator.hpp"
#include "LocaleSharedMemory.hpp"
#include "ParallelLoop.hpp"
#include <utility>
#include <queue>

DECLARE_bool(flat_combining);

namespace Grappa {

/// Mixin for adding common global data structure functionality, such as mirrored 
/// allocation on all cores.
// template< typename Base, Core MASTER = 0 >
// class MirroredGlobal : public Base {
//   char pad[block_size - sizeof(Base)];
// public:
//   template <typename... Args>
//   explicit MirroredGlobal(Args&&... args): Base(std::forward<Args...>(args...)) {}
//   explicit MirroredGlobal(): Base() {}
// 
//   /// Allocate and call init() on all instances of Base
//   ///
//   /// @param init Lambda of the form: void init(Base*)
//   ///
//   /// @b Example:
//   /// @code
//   ///     auto c = MirroredGlobal<Counter>::create([](Counter* c){ new (c) Counter(0); });
//   /// @endcode
//   template< typename F >
//   static GlobalAddress<MirroredGlobal<Base>> create(F init) {
//     auto a = mirrored_global_alloc<MirroredGlobal<Base>>();
//     call_on_all_cores([a,init]{ init(static_cast<Base*>(a.localize())); });
//     return a;
//   }
// 
//   /// Allocate an instance on all cores and initialize with default constructor.
//   /// 
//   /// @note Requires Base class to have a default constructor.
//   static GlobalAddress<MirroredGlobal<Base>> create() {
//     auto a = mirrored_global_alloc<MirroredGlobal>();
//     call_on_all_cores([a]{ new (a.localize()) MirroredGlobal<Base>(); });
//     return a;
//   }
// 
//   void destroy() {
//     auto a = this->self;
//     call_on_all_cores([a]{ a->~MirroredGlobal<Base>(); });
//     global_free(a);
//   }
// 
// };

template <typename T>
class FlatCombiner {
  
  struct Flusher {
    T * id;
    Worker * sender;
    ConditionVariable cv;
    Flusher(T * id): id(id), sender(nullptr) {}
    ~Flusher() { locale_free(id); }
  };
  
  Flusher * current;
  size_t inflight;
  
public:
  
  FlatCombiner(T * initial): current(new Flusher(initial)), inflight(0) {}
  ~FlatCombiner() { delete current; }
  
  // template <typename... Args>
  // explicit FlatCombiner(Args&&... args)
  //   : current(new Flusher(new T(std::forward<Args...>(args...))))
  //   , inflight(nullptr)
  //   , sender(nullptr)
  // { }
  // 
  // // in case it's a no-arg constructor
  // explicit FlatCombiner(): FlatCombiner() {}

  // for arbitrary return type defined by base class
  // auto operator()(Args&&... args) -> decltype(this->T::operator()(std::forward<Args...>(args...))) {
  
  template< typename F >
  void combine(F func) {
    auto s = current;
    
    func(*s->id);
    
    if (s->id->is_full()) {
      current = new Flusher(s->id->clone_fresh());
      if (s->sender == nullptr) {
        inflight++;
        DVLOG(3) << "flushing on full";
        flush(s);
        return;
      } // otherwise someone else assigned to send...
    } else if (inflight == 0) {
      inflight++;
      // always need at least one in flight
      if (s->sender == nullptr) {
        current = new Flusher(s->id->clone_fresh());
        DVLOG(3) << "flush because none in flight";
        flush(s);
        return;
      }
    }
    
    // not my turn yet
    Grappa::wait(&s->cv);
    // on wake...
    if (&current_worker() == s->sender) { // I was assigned to send
      if (s == current) {
        current = new Flusher(s->id->clone_fresh());
      }
      DVLOG(3) << "flush by waken worker";
      flush(s);
      return;
    }
    
  }

  void flush(Flusher * s) {
    s->sender = &current_worker(); // (if not set already)
    DVLOG(3) << "flushing (" << s->sender << "), s(" << s << ")";
    
    s->id->sync();
    
    broadcast(&s->cv); // wake our people
    if (current->cv.waiters_ != 0 && current->sender == nullptr) {
      // atomically claim it so no one else tries to send in the meantime
      // wake someone and tell them to send
      current->sender = impl::get_waiters(&current->cv);
      signal(&current->cv);
      DVLOG(3) << "signaled " << current->sender;
    } else {
      inflight--;
    }
    s->sender = nullptr;
    delete s;
  }
};

// template< T >
// class ProxiedGlobal : public T {
//   using Master = typename T::Master;
//   using Proxy = typename T::Proxy;
// public:  
//   GlobalAddress<ProxiedGlobal<T>> self;
//   
//   template< typename... Args >
//   static GlobalAddress<ProxiedGlobal<T>> create(Args&&... args) {
//     static_assert(sizeof(ProxiedGlobal<T>) % block_size == 0,
//                   "must pad global proxy to multiple of block_size");
//     // allocate enough space that we are guaranteed to get one on each core at same location
//     auto qac = global_alloc<char>(cores()*(sizeof(ProxiedGlobal<T>)+block_size));
//     while (qac.core() != MASTER_CORE) qac++;
//     auto qa = static_cast<GlobalAddress<ProxiedGlobal<T>>>(qac);
//     CHECK_EQ(qa, qa.block_min());
//     CHECK_EQ(qa.core(), MASTER_CORE);
//     
//     // intialize all cores' local versions
//     call_on_all_cores([=]{
//       new (s.self.localize()) ProxiedGlobal<T>(args...);
//     });
// 
//     return qa;
//   }
//   
//   void destroy() {
//     auto q = shared.self;
//     global_free(q->shared.base);
//     call_on_all_cores([q]{ q->~GlobalVector(); });
//     global_free(q);
//   }
//   
// };
// 

} // namespace Grappa

