#include <utility>

namespace Grappa {

template <typename T>
class FlatCombiner {
  
  struct Flusher {
    T * id;
    Worker * sender;
    ConditionVariable cv;
    Flusher(T * id): id(id), sender(nullptr) {}
    ~Flusher() { delete id; }
  };
  
  Flusher * current;
  Flusher * inflight;
  
public:
  FlatCombiner(T * initial): current(new Flusher(initial)), inflight(nullptr) {}
  ~FlatCombiner() { delete current; delete inflight; }
  
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
    
    if (s->id->is_full() || inflight == nullptr) {
      inflight = current;
      current = new Flusher(s->id->T::clone_fresh());
      if (s->sender == nullptr) {
        flush(s);
      } // otherwise someone else is already assigned and will send when ready
    } else {
      Grappa::wait(&s->cv);
      if (&current_worker() == s->sender) {
        if (s == current) current = new Flusher(s->id->clone_fresh());
        flush(s);
      }
    }
    
  }

  void flush(Flusher * s) {
    s->sender = &current_worker(); // (if not set already)
    
    s->id->sync();
    
    broadcast(&s->cv); // wake our people
    if (current->cv.waiters_ != 0) {
      // atomically claim it so no one else tries to send in the meantime
      inflight = current;
      // wake someone and tell them to send
      inflight->sender = impl::get_waiters(&inflight->cv);
      signal(&inflight->cv);
    } else {
      inflight = nullptr;
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