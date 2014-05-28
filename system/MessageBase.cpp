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

#include "Aggregator.hpp"

#include "MessageBase.hpp"
#include "MessageBaseImpl.hpp"

#include "ConditionVariable.hpp"

namespace Grappa {

  /// Internal messaging functions
  namespace impl {

    /// @addtogroup Communication
    /// @{

    /// Block until message can be deallocated.
    void Grappa::impl::MessageBase::block_until_sent() {
      DVLOG(5) << this << " on " << Grappa::impl::global_scheduler.get_current_thread()
               << " maybe blocking until sent";
      // if enqueued, block until message is sent
      while( is_enqueued_ && !is_sent_ ) {
        DVLOG(5) << this << " on " << Grappa::impl::global_scheduler.get_current_thread()
                 << " actually blocking until sent";
        Grappa::wait( &cv_ );
        DVLOG(5) << this << " on " << Grappa::impl::global_scheduler.get_current_thread()
                 << " woken after blocking until sent";
      }
      DVLOG(5) << this << " on " << Grappa::impl::global_scheduler.get_current_thread()
               << " continuing after blocking until sent";
    }

    /// @}

  }
}

