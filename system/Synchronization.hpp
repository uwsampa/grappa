
#ifndef __SYNCHRONIZATION_HPP__
#define __SYNCHRONIZATION_HPP__

#include <TaskingScheduler.hpp>

typedef Thread Worker;

namespace Grappa {
  
  /// @TODO: remove me in the future.
  typedef Thread Worker;

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



