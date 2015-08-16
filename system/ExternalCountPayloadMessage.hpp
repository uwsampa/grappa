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

#pragma once

#include "Message.hpp"

namespace Grappa {

  /// 
  /// For Payload messages that need to set an existing flag
  /// when message created and unset when message sent
  ///
  /// To be used when you won't be able to reference the
  /// message just the flag
  template < typename T >
    class ExternalCountPayloadMessage : public Grappa::PayloadMessage<T> {
      private:
        uint64_t * count_;
      public:
        inline ExternalCountPayloadMessage( Core dest, T t, void * payload, size_t payload_size, uint64_t * count )
          : Grappa::PayloadMessage<T>( dest, t, payload, payload_size )
            , count_ ( count )
      { 

        DVLOG(5) << __func__ << ": " << this << " incrementing " << *count_ << " to " << *count_+1;
        *count_ += 1;
      }

      protected:
        virtual void mark_sent() {
          DVLOG(5) << __func__ << ": " << this << " Marking sent with"            << " is_delivered_=" << this->is_delivered_ 
            << " is_sent_=" << this->is_sent_;

          Core source_copy = this->source_;
          uint64_t * count_copy = this->count_;
          auto this_copy = this;

          Grappa::PayloadMessage<T>::mark_sent();

          /* `this` is now assumed deleted (may or may not be) */

          if ( Grappa::mycore() == source_copy ) {
            CHECK( *count_copy > 0 ) << "count_ = " << *count_copy;
            CHECK_EQ( Grappa::mycore(), source_copy );
            DVLOG(5) << __func__ << ": " << this_copy << " decrementing " << *count_copy << " to " << *count_copy-1;
            *count_copy -= 1;
          }
        }
  } __attribute__((aligned(64)));

  template< typename T >
    ExternalCountPayloadMessage<T> * send_heap_message( Core dest, T t, void * payload, size_t payload_size, uint64_t * count ) {
      void * p = Grappa::impl::locale_shared_memory.allocate_aligned( sizeof(ExternalCountPayloadMessage<T>), 8 );
      auto m = new (p) ExternalCountPayloadMessage<T>( dest, t, payload, payload_size, count );
      m->delete_after_send(); 
      m->enqueue();
      return m;
    }
}

