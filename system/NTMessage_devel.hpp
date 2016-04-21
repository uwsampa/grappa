#pragma once
////////////////////////////////////////////////////////////////////////
// Copyright (c) 2010-2015, University of Washington and Battelle
// Memorial Institute.  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//     * Redistributions of source code must retain the above
//       copyright notice, this list of conditions and the following
//       disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials
//       provided with the distribution.
//     * Neither the name of the University of Washington, Battelle
//       Memorial Institute, or the names of their contributors may be
//       used to endorse or promote products derived from this
//       software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// UNIVERSITY OF WASHINGTON OR BATTELLE MEMORIAL INSTITUTE BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
////////////////////////////////////////////////////////////////////////

// This file is for us to work in while developing the new NTMessage
// functionality, so that we don't have to rebuild all of Grappa every
// time we change something and want to run a test.  Everything here
// will be moved into NTMessage.hpp once it's ready for integration
// with the rest of Grappa.

#include <NTMessage.hpp>
#include <NTBuffer.hpp>
#include <Addressing.hpp>
#include <common.hpp>

namespace Grappa {
namespace impl {


//
// Second, more complete version of NTMessage
//

//
// Messages will consist of up to three parts:
// * an NTHeader, containing
//    * destination core, with enough bits for at least 1M cores (20 or more)
//    * address, with enough bits to represent virtual addresses (44 is enough for now)
//    * deserializer function pointer. If we assume "-mcmodel=small" or "medium", where all code is linked in the lower 2GB of address sapce, we need at most 31 bits.
//    * count: number of times handler should be executed; if the capture or payload are non-empty this will also be the number of data items to read from buffer
//    * message size: size of capture and/or payload for a single handler invocation, not including header (so that we can increment by this much for each invocation)
//
// One of these will be followed by <count> copies of these:
// * optional lambda capture storage, for messages whose lambda captures data
// * optional dynamic payload storage, for messages where payload is non-empty. Payload size must be the same when sharing a header
//
// This will allow us to support a number of different message signatures:
//
// Empty lambdas have a minimum sizeof() of 1, so specialize these to avoid that 1-byte-per-message overhead:
// * send_ntmessage( Core destination, []                                 { ; }                  );
// * send_ntmessage( Core destination, []        (       U * u, size_t s) { ; }, U * u, size_t s );
//
// With a capture, the message size should be sizeof(lambda) + optional size of dynamic argument:
// * send_ntmessage( Core destination, [capture]                          { ; }                  );
// * send_ntmessage( Core destination, [capture] (       U * u, size_t s) { ; }, U * u, size_t s );
//
// Messages with an address target should prefetch the destination address.
// Empty lambdas have a minimum sizeof() of 1, so specialize these to avoid that 1-byte-per-message overhead:
// * send_ntmessage( GlobalAddress<T>, []        (T * t) { ; } );
// * send_ntmessage( GlobalAddress<T>, []        (T & t) { ; } );
// * send_ntmessage( GlobalAddress<T>, []        (T * t, U * u, size_t s) { ; }, U * u, size_t s );
// * send_ntmessage( GlobalAddress<T>, []        (T & t, U * u, size_t s) { ; }, U * u, size_t s );
//
// With a capture, the message size should be sizeof(lambda) + optional sizeof(size of dynamic argument):
// * send_ntmessage( GlobalAddress<T>, [capture] (T & t                 ) { ; }                  );
// * send_ntmessage( GlobalAddress<T>, [capture] (T * t                 ) { ; }                  );
// * send_ntmessage( GlobalAddress<T>, [capture] (T & t, U * u, size_t s) { ; }, U * u, size_t s );
// * send_ntmessage( GlobalAddress<T>, [capture] (T * t, U * u, size_t s) { ; }, U * u, size_t s );
//
// Offset:
// * first message: new header, just store address
// * second message: compute difference between this address and first one, and store
// * third, ... message: compute difference between this address and last. if same, combine. otherwise, start new message
//
// Message combining:
// * for each destination, store: (or just for most recent destination)
//   * function pointer
//   * payload size
//   * last argument address
//   * last argument offset



// deaggregation declaration
char * deaggregate_new_nt_buffer( char * buf, size_t size );

//
// The struct layout is quite sensitive to field width and order. We
// use the script in system/utils/explore_struct_packing.sh to explore
// different layouts.
//

constexpr int NTMESSAGE_WORD_BITS        = 64;
constexpr int NTMESSAGE_ADDRESS_BITS     = 48;
constexpr int NTMESSAGE_DESTINATION_BITS = (NTMESSAGE_WORD_BITS - NTMESSAGE_ADDRESS_BITS);
constexpr int NTMESSAGE_SIZE_BITS        = 12;

struct NTHeader {
  union {
    struct { // TODO: reorder for efficiency? rebalance after collecting data?
      uint32_t dest_  : NTMESSAGE_DESTINATION_BITS; //< destination core
      uint16_t size_  : NTMESSAGE_SIZE_BITS; //< overall message size (capture + payload). May be zero for no-capture, no-payload messages

      uint32_t fp_    : 32; //< if positive, deserializer function pointer. if negative, direct FP

      // TODO: how many messages can we practically combine?
      uint16_t count_ : 10; // number of messages combined into this header

      int64_t addr_   : NTMESSAGE_ADDRESS_BITS; //< first argument address

      // TODO: do we need more than 0 or 1 for this? or do we need something more complicated?
      int16_t offset_ : 6; // for messages with arguments of type T, increment pointer by this much each time
    };
    uint64_t raw_[2]; // for size/alignment
  };

  inline const size_t next_offset() const {
    /// We want all message headers to start on 8-byte alignment, but
    /// captures and payloads may be only a byte; round up if
    /// necessary. NTBuffer takes care of this during aggregation.
    return round_up_to_n<8>( sizeof(NTHeader) + this->count_ * this->size_ );
  }
  
}  __attribute__((aligned(8))); // TODO: verify alignment?

static_assert( sizeof(NTHeader) == 16, "NTHeader seems to have grown beyond intended 16 bytes" );

static std::ostream & operator<<( std::ostream & o, const NTHeader & h ) {
  uint64_t fp   = h.fp_;
  uint64_t addr = h.addr_;
  return o << "<NT dest:" << h.dest_
           << " fp:"      << (void*) fp
           << " addr:"    << (void*) addr
           << " offset:"  << h.offset_
           << " addr:"    << (void*) addr
           << " invocation size:" << h.size_
           << " count:"   << h.count_
           << " total size:" << sizeof(h) + h.count_ * h.size_
           << ">";
}

template< typename T >
static constexpr uint32_t make_31bit( T t ) {
  static_assert( sizeof(t) == 8, "Function pointer not 8 bytes!" );
  // TODO: can we statically check that the function pointer can be represented in 31 bits?
  return reinterpret_cast< uintptr_t >( t );
}

//
// Messages without payload or address
//

template< typename H, // handler type
          // is this a no-capture functor or lambda?
          bool empty_capture = std::is_empty<H>::value >
struct NTMessageSpecializer {
  static void send_ntmessage( Core destination, H handler, NTBuffer * buffer );
  static char * deserialize_and_call( char * t );
};

// Specializer for no-payload no-capture no-address message
template< typename H >
struct NTMessageSpecializer<H, true> {

  static void send_ntmessage( Core destination, H handler, NTBuffer * buffer ) {
    LOG(INFO) << "No address; handler has empty capture: " << __PRETTY_FUNCTION__;

    // 32 bits is actually okay for our current message format, but 31 bits should be all we need.
    auto fp = make_31bit( &NTMessageSpecializer::deserialize_and_call );
    CHECK_EQ( 0, (~((1ULL << 31)-1)) & make_31bit( &NTMessageSpecializer::deserialize_and_call ) )
      << "Deserializer pointer can't be represented in 31 bits";

    auto previous = static_cast< NTHeader * >( nt_get_previous( buffer ) );

    if( previous &&
        previous->dest_ == destination &&
        previous->fp_   == fp ) {
      // aggregate with previous header
      previous->count_ += 1;
    } else {
      // aggregate new header
      NTHeader h;
      h.dest_ = destination;
      h.addr_ = 0; // don't care; unused in this message format
      h.offset_ = 0; // don't care; unused in this message format
      h.fp_ = fp;
      h.count_ = 1; // increment if previous call was the same
      h.size_ = 0; // no capture or payload

      // Enqueue byte with header flag set. No padding is necessary
      // since headers are always 8-byte aligned.
      Grappa::impl::nt_enqueue( buffer, &h, sizeof(h), true );
    }
  }

  // This routine will be called with a pointer to a message in a
  // buffer of aggregated messages. it returns a pointer to the next
  // message in the buffer.
  static char * deserialize_and_call( char * buf ) {
    auto header_p = reinterpret_cast< NTHeader * >( buf );
    LOG(INFO) << "Deserializing " << *header_p << " in " << __PRETTY_FUNCTION__;

    // Since this is a no-capture specialization, we don't store the
    // lambda. Instead, make a fake lambda we can cast to the real
    // lambda type.
    auto dummy         = [] { ; };
    auto fake_lambda_p = reinterpret_cast< H * >( &dummy );

    for( int i = 0; i < header_p->count_; ++i ) {
      (*fake_lambda_p)();
    }

    return buf + header_p->next_offset();
  }
};

// Specializer for no-payload no-address message with capture
template< typename H >
struct NTMessageSpecializer<H, false> {
  static void send_ntmessage( Core destination, H handler, NTBuffer * buffer ) {
    LOG(INFO) << "No address; handler of size " << sizeof(H) << " too big for address field: " << __PRETTY_FUNCTION__;
    handler();
  }
};


//
// Messages with address but without payload
//


// helper struct to check operator() type if H has class type (functor or lambda)
template< typename H, typename ARG >
struct NTAddressMessageHelper :
    std::integral_constant< bool,
                            ( std::is_same< decltype( &H::operator() ), void (H::*)(ARG) const >::value ||
                              std::is_same< decltype( &H::operator() ), void (H::*)(ARG) >::value ) > { };

// handler is function pointer, not functor or lambda; don't check operator() type
template< typename H, typename ARG >
struct NTAddressMessageHelper<H*,ARG> : std::false_type { };


template< typename H, // handler type
          typename T, // address type
          // is this a no-capture functor or lambda?
          bool empty_capture = std::is_empty<H>::value,
          // does it takes a pointer or reference argument?
          bool operator_takes_ptr  = (NTAddressMessageHelper<H,T*>::value),
          bool operator_takes_ref  = (NTAddressMessageHelper<H,T&>::value) >
struct NTAddressMessageSpecializer {
  static void send_ntmessage( GlobalAddress<T> address, H handler, NTBuffer * buffer );
  static char * deserialize_and_call( char * t );
};

// Specializer for no-payload no-capture message with address with pointer argument
template< typename H, typename T >
struct NTAddressMessageSpecializer<H, T, true, true, false> {
  static void send_ntmessage( GlobalAddress<T> address, H handler, NTBuffer * buffer ) {
    LOG(INFO) << "GlobalAddress; handler has pointer and empty capture: " << __PRETTY_FUNCTION__;
  }
};

// Specializer for no-payload no-capture message with address with reference argument
template< typename H, typename T >
struct NTAddressMessageSpecializer<H, T, true, false, true> {
  static void send_ntmessage( GlobalAddress<T> address, H handler, NTBuffer * buffer ) {
    LOG(INFO) << "GlobalAddress; handler has reference and empty capture: " << __PRETTY_FUNCTION__;
  }
};

// Specializer for no-payload message with address and capture with pointer argument
template< typename H, typename T >
struct NTAddressMessageSpecializer<H, T, false, true, false> {
  static void send_ntmessage( GlobalAddress<T> address, H handler, NTBuffer * buffer ) {
    LOG(INFO) << "GlobalAddress; handler has pointer and non-empty capture: " << __PRETTY_FUNCTION__;
  }
};

// Specializer for no-payload message with address and capture with reference argument
template< typename H, typename T >
struct NTAddressMessageSpecializer<H, T, false, false, true> {
  static void send_ntmessage( GlobalAddress<T> address, H handler, NTBuffer * buffer ) {
    LOG(INFO) << "GlobalAddress; handler has reference and non-empty capture: " << __PRETTY_FUNCTION__;
  }
};


//
// Messages with payload but without address
//

template< typename H, // handler type
          typename P, // payload type
          // is this a no-capture lambda?
          bool empty_capture = std::is_empty<H>::value >
struct NTPayloadMessageSpecializer : public NTHeader {
  static void send_ntmessage( Core destination, P * p, size_t size, H handler, NTBuffer * buffer );
  static char * deserialize_and_call( char * t );
};

// Specializer for no-capture no-address message with payload
template< typename H, typename P >
struct NTPayloadMessageSpecializer<H, P, true> {
  static void send_ntmessage( Core destination, P * p, size_t size, H handler, NTBuffer * buffer ) {
    LOG(INFO) << "Payload with no address; handler has empty capture: " << __PRETTY_FUNCTION__;
  }


};

// Specializer for no-address message with payload and capture that does not fit in address bits
template< typename H, typename P >
struct NTPayloadMessageSpecializer<H, P, false> {
  static void send_ntmessage( Core destination, P * p, size_t size, H handler, NTBuffer * buffer ) {
    LOG(INFO) << "Payload with no address; handler of size " << sizeof(H) << " too big for address field: " << __PRETTY_FUNCTION__;
  }
};

//
// Messages with address and payload
//

// helper struct to check operator() type if H has class type (functor or lambda)
template< typename H, typename ARG, typename P >
struct NTPayloadAddressMessageHelper :
    std::integral_constant< bool,
                            ( std::is_same< decltype( &H::operator() ), void (H::*)(ARG,P*,size_t) const >::value ||
                              std::is_same< decltype( &H::operator() ), void (H::*)(ARG,P*,size_t) >::value ) > { };

// handler is function pointer, not functor or lambda; don't check operator() type
template< typename H, typename ARG, typename P >
struct NTPayloadAddressMessageHelper<H*,ARG,P> : std::false_type { };


template< typename H, // handler type
          typename T, // address type
          typename P, // payload type
          // is this a no-capture lambda?
          bool empty_capture = std::is_empty<H>::value,
          // does it takes a pointer or reference argument?
          bool operator_takes_ptr  = (NTPayloadAddressMessageHelper<H,T*,P>::value),
          bool operator_takes_ref  = (NTPayloadAddressMessageHelper<H,T&,P>::value) >
struct NTPayloadAddressMessageSpecializer : public NTHeader {
  static void send_ntmessage( GlobalAddress<T> address, P * payload, size_t count, H handler, NTBuffer * buffer );
  static char * deserialize_and_call( char * t );
};

// Specializer for no-capture message with address and payload; lambda takes pointer
template< typename H, typename T, typename P >
struct NTPayloadAddressMessageSpecializer<H, T, P, true, true, false> {
  static void send_ntmessage( GlobalAddress<T> address, P * payload, size_t count, H handler, NTBuffer * buffer ) {
    LOG(INFO) << "Payload with GlobalAddress; handler takes pointer and has empty capture: " << __PRETTY_FUNCTION__;

    Core destination = address.core();

    auto fp = make_31bit( &NTPayloadAddressMessageSpecializer::deserialize_and_call );
    CHECK_EQ( 0, (~((1ULL << 31)-1)) & make_31bit( &NTPayloadAddressMessageSpecializer::deserialize_and_call ) )
      << "Deserializer pointer can't be represented in 31 bits";

    auto previous = static_cast < NTHeader * >( nt_get_previous( buffer ) );

    if( previous && 
        previous->dest_   == destination &&
        previous->fp_     == fp &&
        ( previous->offset_ == 0 ||
          false ) ) { // TODO: fix offset test
      // aggregate with previous header
      previous->count_ += 1;
      // TODO: update offset when appropriate
      // copy new lambda
      Grappa::impl::nt_enqueue( buffer, &handler, sizeof(handler) );
      // copy new payload
      Grappa::impl::nt_enqueue( buffer, payload, count * sizeof(P) );
    } else {
      // aggregate new header
      NTHeader h;
      h.dest_ = destination;
      h.addr_ = reinterpret_cast< intptr_t >( address.pointer() );
      h.offset_ = 0;
      h.fp_ = fp;
      h.count_ = 1;
      h.size_ = sizeof( P ) * count;

      // Enqueue byte with header flag set.
      Grappa::impl::nt_enqueue( buffer, &h, sizeof(h), true );
      Grappa::impl::nt_enqueue( buffer, payload, count * sizeof(P) );
    }
  }
  static char * deserialize_and_call( char * buf ) {
    auto header_p = reinterpret_cast< NTHeader * >( buf );
    const auto invocation_size = header_p->size_;
    const auto offset          = header_p->offset_;
    const auto count           = header_p->count_;
    
    auto data_p        = buf + sizeof(NTHeader);
    auto address       = reinterpret_cast< T * >( header_p->addr_ );
    auto dummy         = [] { ; };
    auto fake_lambda_p = reinterpret_cast< H * >( &dummy );
    
    for( int i = 0; i < count; ++i ) {
      auto payload_p = reinterpret_cast< P * >( data_p );
      const auto payload_size = invocation_size;
      (*fake_lambda_p)(address, payload_p, payload_size);
      data_p  += invocation_size;
      address += offset;
    }
    
    /// We want all message headers to start on 8-byte alignment, but
    /// captures and payloads may be only a byte; round up if
    /// necessary. Aggregation code should do the same.
    size_t increment = round_up_to_n<8>( sizeof(NTHeader) + header_p->count_ * header_p->size_ );
    return buf + increment;
  }

};

// Specializer for no-capture message with address and payload; lambda takes reference
template< typename H, typename T, typename P >
struct NTPayloadAddressMessageSpecializer<H, T, P, true, false, true> {
  static void send_ntmessage( GlobalAddress<T> address, P * payload, size_t count, H handler, NTBuffer * buffer ) {
    LOG(INFO) << "Payload with GlobalAddress; handler takes reference and has empty capture: " << __PRETTY_FUNCTION__;

  }
};

// Specializer for message with address, payload, and capture, lambda takes pointer
template< typename H, typename T, typename P >
struct NTPayloadAddressMessageSpecializer<H, T, P, false, true, false> {
  static void send_ntmessage( GlobalAddress<T> address, P * payload, size_t count, H handler, NTBuffer * buffer ) {
    LOG(INFO) << "Payload with GlobalAddress; handler takes pointer and has non-empty capture: " << __PRETTY_FUNCTION__;

    Core destination = address.core();
    
    // 32 bits is actually okay for our current message format, but 31 bits should be all we need.
    auto fp = make_31bit( &NTPayloadAddressMessageSpecializer::deserialize_and_call );
    CHECK_EQ( 0, (~((1ULL << 31)-1)) & make_31bit( &NTPayloadAddressMessageSpecializer::deserialize_and_call ) )
      << "Deserializer pointer can't be represented in 31 bits";

    auto previous = static_cast< NTHeader * >( nt_get_previous( buffer ) );

    if( previous && 
        previous->dest_   == destination &&
        previous->fp_     == fp &&
        ( previous->offset_ == 0 ||
          false ) ) { // TODO: fix offset test
      // aggregate with previous header
      previous->count_ += 1;
      // TODO: update offset when appropriate
      // copy new lambda
      Grappa::impl::nt_enqueue( buffer, &handler, sizeof(handler) );
      // copy new payload
      Grappa::impl::nt_enqueue( buffer, payload, count * sizeof(P) );
    } else {
      // aggregate new header
      NTHeader h;
      h.dest_ = destination;
      h.addr_ = reinterpret_cast< intptr_t >( address.pointer() );
      h.offset_ = 0; // initial value
      h.fp_ = fp;
      h.count_ = 1; // increment if previous call was the same
      h.size_ = sizeof( H ) + sizeof( P ) * count;
      
      // Enqueue byte with header flag set.
      Grappa::impl::nt_enqueue( buffer, &h, sizeof(h), true );
      Grappa::impl::nt_enqueue( buffer, &handler, sizeof(handler) );
      Grappa::impl::nt_enqueue( buffer, payload, count * sizeof(P) );
    }
  }

  static char * deserialize_and_call( char * buf ) {
    auto header_p = reinterpret_cast< NTHeader * >( buf );
    const auto invocation_size = header_p->size_;
    const auto offset          = header_p->offset_;
    const auto count           = header_p->count_;

    auto data_p = buf + sizeof(NTHeader);
    auto address = reinterpret_cast< T * >( header_p->addr_ );
      
    for( int i = 0; i < count; ++i ) {
      auto capture_p = reinterpret_cast< H * >( data_p );
      auto payload_p = reinterpret_cast< P * >( data_p + sizeof(H) );
      const auto payload_size = invocation_size - sizeof(H);
      (*capture_p)(address, payload_p, payload_size);
      data_p  += invocation_size;
      address += offset;
    }
        
    return buf + header_p->next_offset();
  }
};

// Specializer for message with address, payload, and capture, lambda takes reference
template< typename H, typename T, typename P >
struct NTPayloadAddressMessageSpecializer<H, T, P, false, false, true> {
  static void send_ntmessage( GlobalAddress<T> address, P * payload, size_t count, H handler, NTBuffer * buffer ) {
    LOG(INFO) << "Payload with GlobalAddress; handler takes reference and has non-empty capture: " << __PRETTY_FUNCTION__;
  }
};

} // namespace impl

//
// NTMessage sending functions exposed to user
//

// These are placeholders; really these functions should be doing the following:
// * get pointer to aggregation buffer to store message
// * form header for message with correct deserialization pointer (or combine with previous)
// * copy header and lambda and/or payload into buffer as appropriate
// * update most-recently-used bits to note that the buffer has a message in it
// * if buffer has reeached capactiy, send it now.
// The API of the specializers will need to be updated for this.

// placeholder for buffer in aggregator
static Grappa::impl::NTBuffer placeholder;

/// Send message with no address and no payload. 
template< typename H >
void send_new_ntmessage( Core destination, H handler ) {
  // placeholder API; should be updated to serialize into aggregation buffer
  static_assert( std::is_empty<H>::value ||
                 ! std::is_convertible<H,void(*)(void)>::value,
                 "Function pointers not supported here; please wrap in a lambda." );
  Grappa::impl::NTMessageSpecializer<H>::send_ntmessage( destination, handler, &placeholder );
}

/// Send message with address and no payload. 
template< typename H, typename T >
void send_new_ntmessage( GlobalAddress<T> address, H handler ) {
  // placeholder API; should be updated to serialize into aggregation buffer
  static_assert( std::is_empty<H>::value ||
                 ! ( std::is_convertible<H,void(*)(T&)>::value ||
                     std::is_convertible<H,void(*)(T*)>::value ),
                 "Function pointers not supported here; please wrap in a lambda." );
  Grappa::impl::NTAddressMessageSpecializer<H,T>::send_ntmessage( address, handler, &placeholder );
}

/// Send message with payload. Payload is copied, so payload buffer can be immediately reused.
template< typename H, typename P >
void send_new_ntmessage( Core destination, P * payload, size_t count, H handler ) {
  // placeholder API; should be updated to serialize into aggregation buffer
  static_assert( std::is_empty<H>::value ||
                 ! std::is_convertible<H,void(*)(P*,size_t)>::value,
                 "Function pointers not supported here; please wrap in a lambda." );
  Grappa::impl::NTPayloadMessageSpecializer<H,P>::send_ntmessage( destination, payload, count, handler, &placeholder );
}

/// Send message with address and payload. Payload is copied, so payload buffer can be immediately reused.
template< typename H, typename T, typename P >
void send_new_ntmessage( GlobalAddress<T> address, P * payload, size_t count, H handler ) {
  // placeholder API; should be updated to serialize into aggregation buffer
  static_assert( std::is_empty<H>::value ||
                 ! ( std::is_convertible<H,void(*)(T&,P*,size_t)>::value ||
                     std::is_convertible<H,void(*)(T*,P*,size_t)>::value ),
                 "Function pointers not supported here; please wrap in a lambda." );
  Grappa::impl::NTPayloadAddressMessageSpecializer<H,T,P>::send_ntmessage( address, payload, count, handler, &placeholder );
}

} // namespace Grappa
