
#include "NTBuffer.hpp"

namespace Grappa {
namespace impl {

int NTBuffer::initial_offset_    = 0;
uint8_t * NTBuffer::buffer_pool_ = nullptr;
} // namespace impl
} // namespace Grappa
