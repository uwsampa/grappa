
#include "NTMessage.hpp"

namespace Grappa {
namespace impl {

std::ostream& operator<<( std::ostream& o, const NTMessageBase& m ) {
  uint64_t fp = m.fp_;
  return o << "<Amessage core:" << m.dest_ << " size:" << m.size_ << " fp:" << (void*) fp << ">";
}

char * deaggregate_amessage_buffer( char * buf, size_t size ) {
  const char * end = buf + size;
  while( buf < end ) {
    auto mb = reinterpret_cast<NTMessageBase*>(buf);
    uint64_t fp_int = mb->fp_;
    auto fp = reinterpret_cast<deserializer_t>(fp_int);
    char * next = (*fp)(buf);
    buf = next;
  }
  return buf;
}

} // namespace impl
} // namespace Grappa

