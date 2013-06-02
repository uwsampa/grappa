#pragma once

#include "Grappa.hpp"
#include "LocaleSharedMemory.hpp"

#include <stdlib.h>

namespace Grappa {

/// Vector that exposes its storage array as read-only.
/// increases in capacity as elements are added.
/// 
/// Only supports insertions. Insertions allowed only
/// while vector is in write only (OpMode::WO) mode and 
/// getting the buffer pointer only allowed in read only mode
/// 
template < typename T >
class LocalVector {
protected:
  T * buf;
  size_t capacity;
  uint64_t nextIndex;

  void grow_if_needed() {
    if ( nextIndex == capacity ) {
      // expand the capacity of the buffer
      size_t newcapacity = capacity * 2;
      T * newbuf = locale_alloc<T>(newcapacity);
      memcpy( newbuf, buf, capacity*sizeof(T) );
      locale_free( buf );
      capacity = newcapacity;
      buf = newbuf;
    }
  }

public:
  LocalVector( size_t capacity = 2 ) 
    : buf ( locale_alloc<T>(capacity) )
    , nextIndex( 0 )
    , capacity( capacity ) { }

  ~LocalVector() {
    locale_free(buf);
  }

  void reserve(size_t m) {
    if (capacity < m) {
      auto old_buf = buf;
      buf = locale_alloc<T>(m);
      if (nextIndex > 0) {
        memcpy(buf, old_buf, capacity*sizeof(T));
      }
      locale_free(buf);
      capacity = m;
    }
  }
  
  void resize(size_t m) {
    reserve(m);
    nextIndex = m;
  }

  template< typename... Args >
  void emplace(Args&&... args) {
    grow_if_needed();
    
    new(buf+nextIndex) T(std::forward<Args...>(args...));
    nextIndex++
  }

  void insert( const T& v ) {
    grow_if_needed();
    buf[nextIndex++] = v;
  }

  GlobalAddress<T> storage() const {
    return make_global(buf);
  }

  size_t size() const {
    return nextIndex;
  }
  
  template < typename T1 >
  friend std::ostream& operator<<( std::ostream& o, const LocalVector<T1>& v );
};

template< typename T >
std::ostream& operator<<( std::ostream& o, const LocalVector<T>& v ) {
  o << "LocalVector(" 
             << "capacity=" << v.capacity << ", "
             << "nextIndex=" << v.nextIndex << ")[ ";
  
  // print contents; ignores RO/WO functionality
  for (uint64_t i=0; i<v.nextIndex; i++) {
    o << v.buf[i] << ", ";
  }
  o << "]";
  return o;
}

} // namespace Grappa
