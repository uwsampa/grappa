#pragma once

// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include "TaskingScheduler.hpp"
#include "Synchronization.hpp"
#include "ConditionVariableLocal.hpp"
#include "CountingSemaphoreLocal.hpp"

namespace Grappa {

  /// @addtogroup Synchronization
  /// @{

  template< typename Semaphore >
  inline void increment( Semaphore * s, int64_t incr = 1 ) {
    s->increment( incr );
  }
  
  template< typename Semaphore >
  inline void decrement( Semaphore * s, int64_t decr = 1 ) {
    s->decrement( decr );
  }
  
  template< typename Semaphore >
  inline bool try_decrement( Semaphore * s, int64_t decr = 1 ) {
    return s->try_decrement( decr );
  }
  
  template< typename Semaphore >
  inline int64_t get_value( Semaphore * s ) {
    return s->get_value();
  }
  
  /// @}

}

