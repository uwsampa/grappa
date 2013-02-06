#ifndef DESCRIPTOR_HPP
#define DESCRIPTOR_HPP

#include "LegacySignaler.hpp"
#include "Grappa.hpp"


/**
 * A one-time-use, one-way producer-consumer
 * syncrhonizing mailbox
 *
 * Most typically created by the consumer, since the
 * buffer must be provided in the constructor.
 */
template < typename T >
class Descriptor {
  private:
    Signaler signaler;
    T * data;
  
  public:
    Descriptor( T * buf ) 
      : signaler()
      , data ( buf )
  { }

    /**
     * Block until the data buffer is filled.
     *
     * Returns anytime after the mailbox has been filled.
     */
    void wait() {
      signaler.wait();
    }

    /**
     * Copies the given data into the descriptor result buffer
     * and notifies the waiter.
     *
     * This function is NOT a yield point.
     */
    void fillData( T * result, size_t num_bytes ) {
      memcpy( data, result, num_bytes );
      signaler.signal();
    }
};

template < typename T >
static void descriptor_reply_am( GlobalAddress< Descriptor<T> > * desc, size_t ds, T * payload, size_t payload_size ) {
  Descriptor<T> * d = desc->pointer();
  d->fillData(payload, payload_size);
}

template < typename T >
void descriptor_reply_one ( GlobalAddress< Descriptor<T> > desc, T * data ) {
  size_t data_size = sizeof(T); // 1 element of data allowed

  Grappa_call_on_x( desc.node(), descriptor_reply_am<T>, &desc, sizeof(GlobalAddress< Descriptor<T> >), data, data_size );
}

template < typename T >
size_t Grappa_sizeof_descriptor_reply_one( GlobalAddress< Descriptor<T> > desc, T * data ) {
  size_t data_size = sizeof(T); // 1 element of data allowed
  return Grappa_sizeof_message( &desc, sizeof(GlobalAddress< Descriptor<T> >), data, data_size );
}


#endif // DESCRIPTOR_HPP
