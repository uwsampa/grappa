
#ifndef __CONDITION_VARIABLE_HPP__
#define __CONDITION_VARIABLE_HPP__

#include <TaskingScheduler.hpp>

namespace Grappa {
  
  struct Worker;

  /// @addtogroup Synchronization
  /// @{

  class ConditionVariable {
  private:
    Worker * waiters_;
  public:
    ConditionVariable()
      : waiters_( NULL )
    { }

    inline void wait() {
      CURRENT_THREAD->next = waiters;
      waiters_ = CURRENT_THREAD;
      global_scheduler.thread_suspend();
    }

    inline void signal() {
      if( waiters_ ) {
	Worker * to_wake = waiters_;
	waiters_ = waiters_->next;
	global_scheduler.thread_wake( to_wake );
      }
    }

    inline void signal_all() {
      while( waiters_ ) {
	Worker * to_wake = waiters_;
	waiters_ = waiters_->next;
	global_scheduler.thread_wake( to_wake );
      }
    }
  };

  /// @}

}

#endif
