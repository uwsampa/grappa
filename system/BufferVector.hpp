#pragma once

#include "Grappa.hpp"
#include "LocaleSharedMemory.hpp"

#include <stdlib.h>

namespace Grappa {

/// Vector that exposes its storage array as read-only.
/// increases in size as elements are added.
/// 
/// Only supports insertions. Insertions allowed only
/// while vector is in write only (OpMode::WO) mode and 
/// getting the buffer pointer only allowed in read only mode
/// 
template < typename T >
class BufferVector {

  private:
    T * buf;
    size_t size;
    uint64_t nextIndex;

    enum class OpMode { RO, WO };
    OpMode mode;

  public:
    BufferVector( size_t capacity = 2 ) 
      : buf ( locale_alloc<T>(capacity) )
      , mode( OpMode::WO )
      , nextIndex( 0 )
      , size( capacity ) { }

    ~BufferVector() {
      locale_free(buf);
    }

    void setWriteMode() {
      CHECK( mode != OpMode::WO ) << "already in OpMode::WO mode";
      mode = OpMode::WO;
    }

    void setReadMode() {
      CHECK( mode != OpMode::RO ) << "already in OpMode::RO mode (" << this << ")";
      DVLOG(3) << "(" << this << ").setReadMode()";
      mode = OpMode::RO;
    }

    void insert( const T& v ) {
      CHECK( mode == OpMode::WO ) << "Must be in OpMode::WO mode to insert";

      if ( nextIndex == size ) {
        // expand the size of the buffer
        size_t newsize = size * 2;
        T * newbuf = locale_alloc<T>(newsize);
        memcpy( newbuf, buf, size*sizeof(T) );
        locale_free( buf );
        size = newsize;
        buf = newbuf;
      }

      // store element and increment index
      buf[nextIndex++] = v;
    }

    GlobalAddress<T> getReadBuffer() const {
      CHECK( mode == OpMode::RO ) << "Must be in OpMode::RO mode to see buffer";
      return make_global( /*static_cast<const T*>*/(buf) );
    }

    size_t getLength() const {
      return nextIndex; 
    }

    /* to close the safety loop, OpMode::RO clients should release buffers,
     * allowing setWriteMode() to check that all buffers are released
     * but currently we will avoid this extra communication
     *
     * If we want more fine-grained concurrent R/W then it is better
     * not to expose this getBuffer type of interface 
     */

    template < typename T1 >
    friend std::ostream& operator<<( std::ostream& o, const BufferVector<T1>& v );
};

template< typename T >
std::ostream& operator<<( std::ostream& o, const BufferVector<T>& v ) {
  o << "BV(" 
             << "size=" << v.size << ", "
             << "nextIndex=" << v.nextIndex << /*", "
             << "mode=" << v.mode==BufferVector<T>::OpMode::RO?"RO":"WO" <<*/ ")[";

  // print contents; ignores RO/WO functionality
  for (uint64_t i=0; i<v.nextIndex; i++) {
    int64_t r;
    o << v.buf[i] << ",";
  }
  o << "]";
  return o;
}

} // namespace Grappa
