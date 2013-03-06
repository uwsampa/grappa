#include "MessagePool.hpp"

void* operator new(size_t size, Grappa::MessagePoolBase& a) {
  if (a.remaining() < size) {
    CHECK(size <= a.buffer_size) << "Pool (" << a.buffer_size << ") is not large enough to allocate " << size << " bytes.";
    VLOG(3) << "MessagePool full, blocking until all previous ones sent.";
    a.block_until_all_sent();
  }
  return a.allocate(size);
}
