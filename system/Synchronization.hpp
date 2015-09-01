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

#ifndef __SYNCHRONIZATION_HPP__
#define __SYNCHRONIZATION_HPP__

#include <TaskingScheduler.hpp>

namespace Grappa {

  /// @addtogroup Synchronization
  /// @{
  
  namespace impl {

    /// compiler memory fence
    static inline void compiler_memory_fence() {
      __asm__ __volatile__ ("" ::: "memory");
    }


    /// Example Mutex/ConditionVariable/FullBit showing key bitfield
    /// elements. Similar objects are used, including whatever fields
    /// are needed for the synchronization object in question. 
    /// The goal is to keep these to 64 bits. 
    class ExampleMutexOrConditionVariableOrFullBit {
    private:
      union {
	struct {
	  // We can store  to 16 bits of flags.
	  bool lock_ : 1;
	  bool full_ : 1;
	  bool trap1_ : 1;
	  bool trap2_ : 1;

	  // Can't include pointers in bitfield, so use intptr_t
	  // instead. In order to store pointers from the current
	  // x86_64 ABI, this field must be at least 48 bits, and it
	  // must be signed.
	  intptr_t waiters_ : 60; 
	};
	intptr_t raw_; // probably unnecessary; just to ensure alignment
      };
    };

    /// Verify that ExampleMutexOrConditionVariableOrFullBit is only one 64-bit word
    static_assert( sizeof( ExampleMutexOrConditionVariableOrFullBit ) == 8, 
		   "ExampleMutexOrConditionVariableOrFullBit is not 64 bits for some reason.");


    /// accesor for wait list in synchronization object
    template< typename MutexOrConditionVariableOrFullBit >
    static inline Grappa::Worker * get_waiters( MutexOrConditionVariableOrFullBit * t ) { 
      return reinterpret_cast< Grappa::Worker * >( t->waiters_ );
    }

    /// accesor for wait list in synchronization object
    template< typename MutexOrConditionVariableOrFullBit >
    static inline void set_waiters( MutexOrConditionVariableOrFullBit * t, Grappa::Worker * w ) { 
      t->waiters_ = reinterpret_cast< intptr_t >( w ); 
    }

  }

  /// @}

}

#endif



