////////////////////////////////////////////////////////////////////////
// This file is part of Grappa, a system for scaling irregular
// applications on commodity clusters. 

// Copyright (C) 2010-2014 University of Washington and Battelle
// Memorial Institute. University of Washington authorizes use of this
// Grappa software.

// Grappa is free software: you can redistribute it and/or modify it
// under the terms of the Affero General Public License as published
// by Affero, Inc., either version 1 of the License, or (at your
// option) any later version.

// Grappa is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// Affero General Public License for more details.

// You should have received a copy of the Affero General Public
// License along with this program. If not, you may obtain one from
// http://www.affero.org/oagpl.html.
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

