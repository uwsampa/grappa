
#include "NTMessage.hpp"

#include <x86intrin.h>

namespace Grappa {
namespace impl {

std::ostream& operator<<( std::ostream& o, const NTMessageBase& m ) {
  uint64_t fp = m.fp_;
  return o << "<Amessage core:" << m.dest_ << " size:" << m.size_ << " fp:" << (void*) fp << ">";
}

char * deaggregate_nt_buffer( char * buf, size_t size ) {
  const char * end = buf + size;
  while( buf < end ) {
#ifdef USE_NT_OPS
    _mm_prefetch( buf, _MM_HINT_NTA );
    _mm_prefetch( buf+64, _MM_HINT_NTA );
#endif
    char * next = buf + 8;
    if( 0 != *(reinterpret_cast<uint64_t*>(buf)) ) {
      auto mb = reinterpret_cast<NTMessageBase*>(buf);
      uint64_t fp_int = mb->fp_;
      auto fp = reinterpret_cast<deserializer_t>(fp_int);
      DVLOG(5) << "Deserializing with " << (void*) fp << "/" << *mb << " at " << (void*) buf;
      next = (*fp)(buf);
    } else {
      DVLOG(5) << "Skipping a word at " << (void*) buf;
    }
    buf = next;
  }
  return buf;
}

} // namespace impl
} // namespace Grappa

