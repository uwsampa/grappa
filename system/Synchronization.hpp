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



