#include "Addressing.hpp"
#include "Cache.hpp"
#include "Delegate.hpp"

#define fetch_add SoftXMT_delegate_fetch_and_add_word

template<typename T, size_t BUFSIZE=(1L<<14), size_t NBUFS=4 >
struct PushBuffer {
  GlobalAddress<T> target_array;
  GlobalAddress<int64_t> shared_index;
  T buf[NBUFS][BUFSIZE];
  int curr_buf;
  int64_t curr_size[NBUFS];
  
  PushBuffer(): curr_buf(0) {
    for (int64_t i=0; i<NBUFS; i++) curr_size[i] = 0;
  }
  ~PushBuffer() {
    flush();
  }
  void push(const T& o) {
    CHECK(target_array.pointer() != NULL) << "buffer not initialized!";
    buf[curr_buf][curr_size[curr_buf]] = o;
    curr_size[curr_buf]++;
    CHECK(curr_size[curr_buf] <= BUFSIZE);
    if (curr_size[curr_buf] == BUFSIZE) {
      flush();
    }
  }
  void setup(GlobalAddress<T> _target_array, GlobalAddress<int64_t> _shared_index) {
    target_array = _target_array;
    shared_index = _shared_index;
    for (int64_t i=0; i<NBUFS; i++) curr_size[i] = 0;
    curr_buf = 0;
  }
  void flush() {
    int save_buf = curr_buf;
    int64_t save_size = curr_size[curr_buf];
    
    while (curr_size[curr_buf] != 0) {
      curr_buf = (curr_buf + 1) % NBUFS;
      CHECK(curr_buf != save_buf) << "out of buffers in PushBuffer!";
    }
    
    if (save_size > 0) {
      int64_t offset = fetch_add(shared_index, save_size);
      typename Incoherent<T>::WO c(target_array+offset, save_size, (int64_t*)&buf[save_buf]);
      c.block_until_released();
    }
    curr_size[save_buf] = 0;
  }
};

