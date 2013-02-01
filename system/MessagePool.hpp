#include "Grappa.hpp"
#include "Message.hpp"

namespace Grappa {

  template<size_t Bytes>
  class MessagePool {
    char buffer[Bytes];
    size_t allocated;
  protected:
    impl::MessageBase* allocate(size_t sz) {
      LOG(ERROR) << "allocating " << sz;
      impl::MessageBase* p = reinterpret_cast<impl::MessageBase*>(buffer+allocated);
      allocated += sz;
      
      // TODO: implement heap allocation fallback
      if (allocated > Bytes) { LOG(ERROR) << "MessagePool overflow!"; exit(1); }
      return p;
    }
    
  public:
    MessagePool(): allocated(0) {}
    
    ~MessagePool() {
      // call destructors of everything in MessagePool
      iterate([](impl::MessageBase* bp){ bp->~MessageBase(); });
    }
    
    /// Takes a lambda (or really any callable) that is called repeated for
    /// each allocated impl::MessageBase*
    template<typename F>
    void iterate(F f) {
      char * p = buffer;
      while (p < buffer+allocated) {
        impl::MessageBase* bp = reinterpret_cast<impl::MessageBase*>(p);
        p += bp->size();
        f(bp);
      }
    }
    
    template<size_t B>
    friend void* ::operator new(size_t, Grappa::MessagePool<B>&);
    
    template<typename T>
    inline Message<T>* message(Core dest, T t) {
      return new (*this) Message<T>(dest, t);
    }
    
    template<typename T>
    inline SendMessage<T>* send_message(Core dest, T t) {
      return new (*this) SendMessage<T>(dest, t);
    }
  };

}

template<size_t Bytes>
void* operator new(size_t size, Grappa::MessagePool<Bytes>& a) {
  return a.allocate(size);
}
