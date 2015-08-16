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

#ifndef __CONDITION_VARIABLE_HPP__
#define __CONDITION_VARIABLE_HPP__

#include "Message.hpp"
#include "Communicator.hpp"
#include "Addressing.hpp"
#include "ConditionVariableLocal.hpp"

namespace Grappa {

  /// @addtogroup Synchronization
  /// @{
  
  /// Proxy for remote ConditionVariable manipulation
  /// @todo: implement
  inline void wait( GlobalAddress<ConditionVariable> m ) {
    // if local, just wait
    // if remote, spawn a task on the home node to wait
  }

  template<typename ConditionVariable>
  inline void signal( const GlobalAddress<ConditionVariable> m ) {
    if (m.core() == Grappa::mycore()) {
      // if local, just signal
      Grappa::signal(m.pointer());
    } else {
      // if remote, signal via active message
      send_heap_message(m.core(), [m]{
        Grappa::signal(m.pointer());
      });
    }
  }

  /// TODO: implement
  inline void signal_all( GlobalAddress<ConditionVariable> m ) {
    // if local, just signal
    // if remote, signal via active message
  }

  /// @}

}

#endif
