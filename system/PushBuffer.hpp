#include "Addressing.hpp"
#include "Cache.hpp"
#include "Delegate.hpp"

#define fetch_add SoftXMT_delegate_fetch_and_add_word

template<typename T, size_t BUFSIZE=(1L<<14) >
struct PushBuffer {
  GlobalAddress<T> target_array;
  GlobalAddress<int64_t> shared_index;
  T buf[2*BUFSIZE];
  T * curr_buf;
  int64_t curr_size;
  bool saving;
  
  PushBuffer(): curr_buf(buf), curr_size(0), saving(false) {}
  ~PushBuffer() {
    flush();
  }
  void push(const T& o) {
    CHECK(target_array.pointer() != NULL);
    curr_buf[curr_size] = o;
    curr_size++;
    CHECK(curr_size <= BUFSIZE);
    if (curr_size == BUFSIZE) {
      CHECK(!saving);
      flush();
    }
  }
  void setup(GlobalAddress<T> _target_array, GlobalAddress<int64_t> _shared_index) {
    target_array = _target_array;
    shared_index = _shared_index;
    curr_size = 0;
    curr_buf = buf;
  }
  void flush() {
    saving = true;
    T * save_buf = curr_buf;
    int64_t save_size = curr_size;
    
    curr_buf = (curr_buf == buf)? buf+BUFSIZE : buf;
    curr_size = 0;
    
    if (save_size > 0) {
      int64_t offset = fetch_add(shared_index, save_size);
      typename Incoherent<T>::WO c(target_array+offset, save_size, save_buf);
      c.block_until_released();
    }
    saving = false;
  }
};

