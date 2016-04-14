
#include "NTMessage.hpp"
#include "NTMessage_devel.hpp"

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


/// Extract address field from NTMessage header and prefetch it.
static inline void prefetch_header( NTHeader * header_p ) {
  // remind system this cacheline and the next are nontemporal
  _mm_prefetch( header_p  , _MM_HINT_NTA );
  _mm_prefetch( header_p+1, _MM_HINT_NTA );

  // get address field from header and prefetch
  int64_t addr_int = header_p->addr_;
  auto addr        = reinterpret_cast< void * >( addr_int );
  _mm_prefetch( addr, _MM_HINT_NTA ); // TODO: is T0 or NTA better here?
}

/// Deserialize and execute message given by header pointer.
static inline void execute_header( NTHeader * header_p ) {
  // remind system this cacheline is nontemporal
  _mm_prefetch( header_p, _MM_HINT_NTA );

  if( 0 == header_p->count_ ) { // if message is empty, skip it.
    DVLOG(5) << "Skipping " << header_p << ": " << *header_p;
  } else {                      // otherwise, deserialize and execute it.
    DVLOG(5) << "Deserializing at " << header_p << ": " << *header_p;
    uint64_t fp_int = header_p->fp_;
    auto deserializer = reinterpret_cast< deserializer_t >( fp_int );
    (*deserializer)( reinterpret_cast<char*>( header_p ) );
  }
}

char * deaggregate_new_nt_buffer( char * buf, size_t size ) {
  DVLOG(5) << "Deserializing " << size << " bytes at " << (void*) buf;

  char * prefetch  = buf;
  const char * end = buf + size;

  const int PREFETCH_DISTANCE = 5; // TODO: measure and tune

  // start by prefetching some cachelines from the buffer
  _mm_prefetch( buf +   0, _MM_HINT_NTA );
  _mm_prefetch( buf +  64, _MM_HINT_NTA );
  _mm_prefetch( buf + 128, _MM_HINT_NTA );
  _mm_prefetch( buf + 192, _MM_HINT_NTA );
  
  // issue initial prefetches of address fields
  for( int i = 0; i < PREFETCH_DISTANCE; ++i ) {
    auto header_p = reinterpret_cast< NTHeader * >( prefetch );
    prefetch_header( header_p );
    prefetch += header_p->next_offset();
  }

  // now start deserializing from the beginning while still prefetching ahead.
  while( buf < end ) {
    // deserialize and execute
    auto header_p = reinterpret_cast< NTHeader * >( buf );
    execute_header( header_p );
    buf += header_p->next_offset();

    // prefetch farther ahead
    if( prefetch < end ) {
      auto header_p = reinterpret_cast< NTHeader * >( prefetch );
      prefetch_header( header_p );
      prefetch += header_p->next_offset();
    }
  }
  
  return buf;
}

} // namespace impl
} // namespace Grappa

