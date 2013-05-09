#include <utility>

namespace Grappa {

template <typename T>
class FlatCombiner {
  
  class Instance : public T {    
  public:
    template <typename... Args>
    explicit Instance(size_t buffer_size, Args&&... args)
      : buffer_size(buffer_size)
      , T(std::forward<Args...>(args...))
    { }

    // in case it's a no-arg constructor
    explicit FlatCombiner(size_t buffer_size)
      : buffer_size(buffer_size)
    { }

    // for arbitrary return type defined by parent
    // auto operator()(Args&&... args) -> decltype(this->T::operator()(std::forward<Args...>(args...))) {

    template <typename... Args>
    void op(Args&&... args) {
      T::op(std::forward<Args...>(args...));
    }

    void sync() {
      T::sync();
    }
  };
  
  Instance *current, *inflight;
public:
  
  FlatCombiner() {
    
  }
};

} // namespace Grappa