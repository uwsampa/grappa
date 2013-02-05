#ifndef BUFFERED_VECTOR
#define BUFFERED_VECTOR

#include <cstring>
#include <Addressing.hpp>

/* Vector that exposes its storage array as read-only.
 * increases in size as elements are added.
 *
 * Only supports insertions. Insertions allowed only
 * while vector is in write only (WO) mode and 
 * getting the buffer pointer only allowed in read only mode
 */

template < typename V >
class BufferVector {

  private:
    V * buf;
    size_t size;
    uint64_t nextIndex;

    enum OpMode { RO, WO };
    OpMode mode;

  public:
    BufferVector( size_t capacity = 2 ) 
      : buf ( static_cast<V*>( malloc( capacity * sizeof(V) ) ) )
      , mode( WO )
      , nextIndex( 0 )
      , size( capacity ) { }

    void setWriteMode() {
      CHECK( mode != WO ) << "already in WO mode";
      mode = WO;
    }

    void setReadMode() {
      CHECK( mode != RO ) << "already in RO mode";
      mode = RO;
    }

    void insert( const V& v ) {
      CHECK( mode == WO ) << "Must be in WO mode to insert";

      if ( nextIndex == size ) {
        // expand the size of the buffer
        uint64_t newsize = size * 2;
        V * newbuf = static_cast<V*>( malloc( capacity * sizeof(V) ) );
        std::memcpy( newbuf, buf, size );
        std::free( buf );
        size = newsize;
        buf = newbuf;
      }

      // store element and increment index
      buf[nextIndex++] = v;
    }

    GlobalAddress<const V> getReadBuffer() const {
      CHECK( mode == RO ) << "Must be in RO mode to see buffer";
      return make_global( static_cast<const V*>(buf) );
    }

    size_t getSize() const {
      return size;
    }

    /* to close the safety loop, RO clients should release buffers,
     * allowing setWriteMode() to check that all buffers are released
     * but currently we will avoid this extra communication
     *
     * If we want more fine-grained concurrent R/W then it is better
     * not to expose this getBuffer type of interface 
     */
};


#endif // BUFFERED_VECTOR
