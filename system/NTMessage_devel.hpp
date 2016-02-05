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
#include <Addressing.hpp>

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
//    * message size: including size of stored capture
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

constexpr int NTMESSAGE_WORD_BITS    = 64;
constexpr int NTMESSAGE_ADDRESS_BITS = 44;

struct NTHeader {
  union {
    struct { // TODO: reorder for efficiency?
      uint32_t dest_  : (NTMESSAGE_WORD_BITS - NTMESSAGE_ADDRESS_BITS); //< destination core
      int64_t addr_   : (NTMESSAGE_ADDRESS_BITS); //< first argument address

      // 33 bits remaining
      
      uint32_t fp_    : 31; //< deserializer function pointer

      //
      // TODO: rebalance these after collecting data
      //
      
      uint16_t size_  : 13; // overall message size (capture + payload). May be zero for no-capture, no-payload messages

      // TODO: how many messages can we practically combine?
      uint16_t count_ : 10; // message count: should be

      // TODO: do we need more than 0 or 1 for this? or do we need something more complicated?
      int16_t offset_ : 10; // for messages with arguments of type T, increment pointer by this much each time
    };
    uint64_t raw_[2]; // for size/alignment
  };
  // NTHeader(Core dest, uint16_t size, uint32_t fp): dest_(dest), size_(size), fp_(fp) { }
  // NTHeader(): dest_(-1), size_(0), fp_(0) { }
  // NTHeader( const NTMessageBase& m ): dest_(m.dest_), size_(m.size_), fp_(m.fp_) { }
  // NTHeader( NTMessageBase&& m ): dest_(m.dest_), size_(m.size_), fp_(m.fp_) { }

  // static char * deserialize_and_call( char * t ) {
  //   // TODO: what goes here? Specialize for various cases above
  //   NTMessage<T> * tt = reinterpret_cast< NTMessage<T> * >( t );
  //   tt->storage_();
  //   return t + sizeof( NTMessage<T> );
  // }
  
}  __attribute__((aligned(8))); // TODO: verify alignment?


//
// Messages without payload or address
//

template< typename H,
          bool cast_to_voidstarvoid = (std::is_convertible<H,void(*)(void)>::value),
          bool fits_in_address      = (sizeof(H) * 8 <= NTMESSAGE_ADDRESS_BITS) >
struct NTMessageSpecializer : public NTHeader {
  static void send_ntmessage( Core destination, H handler );
  static char * deserialize_and_call( char * t );
};

// Specializer for no-payload no-capture no-address message
template< typename H, bool dontcare >
struct NTMessageSpecializer<H, true, dontcare> {
  static void send_ntmessage( Core destination, H handler ) {
    LOG(INFO) << "No address; handler has empty capture: " << __PRETTY_FUNCTION__;
    handler();
  }
};

// Specializer for no-payload no-address message with capture that fits in address bits
template< typename H >
struct NTMessageSpecializer<H, false, true > {
  static void send_ntmessage( Core destination, H handler ) {
    LOG(INFO) << "No address; handler of size " << sizeof(H) << " fits in address field: " << __PRETTY_FUNCTION__;
    handler();
  }
};

// Specializer for no-payload no-address message with capture that doesn't fit in address bits
template< typename H >
struct NTMessageSpecializer<H, false, false > {
  static void send_ntmessage( Core destination, H handler ) {
    LOG(INFO) << "No address; handler of size " << sizeof(H) << " too big for address field: " << __PRETTY_FUNCTION__;
    handler();
  }
};

//
// Messages with address but without payload
//

template< typename T,
          typename H,
          bool cast_to_voidstarvoid = (std::is_convertible<H,void(*)(void)>::value) >
struct NTAddressMessageSpecializer : public NTHeader {
  static void send_ntmessage( GlobalAddress<T> address, H handler );
  static char * deserialize_and_call( char * t );
};

// Specializer for no-payload no-capture message with address
template< typename T, typename H >
struct NTAddressMessageSpecializer<T, H, true> {
  static void send_ntmessage( GlobalAddress<T> address, H handler ) {
    LOG(INFO) << "GlobalAddress; handler has empty capture: " << __PRETTY_FUNCTION__;
  }
};

// Specializer for no-payload message with address and capture
template< typename T, typename H >
struct NTAddressMessageSpecializer<T, H, false> {
  static void send_ntmessage( GlobalAddress<T> address, H handler ) {
    LOG(INFO) << "GlobalAddress; handler has non-empty capture: " << __PRETTY_FUNCTION__;
  }
};


//
// Messages with payload but without address
//

template< typename H,
          typename P,
          bool cast_to_voidstarvoid = (std::is_convertible<H,void(*)(void)>::value),
          bool fits_in_address      = (sizeof(H) * 8 <= NTMESSAGE_ADDRESS_BITS) >
struct NTPayloadMessageSpecializer : public NTHeader {
  static void send_ntmessage( Core destination, P * p, size_t size, H handler );
  static char * deserialize_and_call( char * t );
};

// Specializer for no-capture no-address message with payload
template< typename H, typename P, bool dontcare >
struct NTPayloadMessageSpecializer<H, P, true, dontcare> {
  static void send_ntmessage( Core destination, P * p, size_t size, H handler ) {
    LOG(INFO) << "Payload with no address; handler has empty capture: " << __PRETTY_FUNCTION__;
  }
};

// Specializer for no-address message with payload and capture that fits in address bits
template< typename H, typename P >
struct NTPayloadMessageSpecializer<H, P, false, true > {
  static void send_ntmessage( Core destination, P * p, size_t size, H handler ) {
    LOG(INFO) << "Payload with no address; handler of size " << sizeof(H) << " fits in address field: " << __PRETTY_FUNCTION__;
  }
};

// Specializer for no-address message with payload and capture that does not fit in address bits
template< typename H, typename P >
struct NTPayloadMessageSpecializer<H, P, false, false > {
  static void send_ntmessage( Core destination, P * p, size_t size, H handler ) {
    LOG(INFO) << "Payload with no address; handler of size " << sizeof(H) << " too big for address field: " << __PRETTY_FUNCTION__;
  }
};

//
// Messages with address and payload
//

template< typename T,
          typename H,
          typename P,
          bool cast_to_voidstarvoid = (std::is_convertible<H,void(*)(void)>::value) >
struct NTPayloadAddressMessageSpecializer : public NTHeader {
  static void send_ntmessage( GlobalAddress<T> address, P * payload, size_t count, H handler );
  static char * deserialize_and_call( char * t );
};

// Specializer for no-capture message with address and payload
template< typename T, typename H, typename P >
struct NTPayloadAddressMessageSpecializer<T, H, P, true> {
  static void send_ntmessage( GlobalAddress<T> address, P * payload, size_t count, H handler ) {
    LOG(INFO) << "Payload with GlobalAddress; handler has empty capture: " << __PRETTY_FUNCTION__;
  }
};

// Specializer for message with address, payload, and capture
template< typename T, typename H, typename P >
struct NTPayloadAddressMessageSpecializer<T, H, P, false> {
  static void send_ntmessage( GlobalAddress<T> address, P * payload, size_t count, H handler ) {
    LOG(INFO) << "Payload with GlobalAddress; handler has non-empty capture: " << __PRETTY_FUNCTION__;
  }
};

} // namespace impl

//
// NTMessage sending functions exposed to user
//

/// Send message with no payload. 
template< typename H >
void send_new_ntmessage( Core destination, H handler ) {
  Grappa::impl::NTMessageSpecializer<H>::send_ntmessage( destination, handler );
}

/// Send message with payload. Payload is copied, so payload buffer can be immediately reused.
template< typename H, typename P >
void send_new_ntmessage( Core destination, P * payload, size_t count, H handler ) {
  Grappa::impl::NTPayloadMessageSpecializer<H,P>::send_ntmessage( destination, payload, count, handler );
}

} // namespace Grappa
