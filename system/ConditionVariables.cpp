#include "ConditionVariables.hpp"
#include "SoftXMT.hpp"
  
Signaler::Signaler()
  : waiter ( NULL )
    , done ( false )
{}

void Signaler::wait() {
  if ( !done ) {
    waiter = CURRENT_THREAD;
    SoftXMT_suspend();
    CHECK( done );
  }
}

void Signaler::signal() {
  done = true;
  if ( waiter != NULL ) {
    SoftXMT_wake(waiter);
    waiter = NULL;
  }
}


ConditionVariable::ConditionVariable() 
: waiter ( NULL )
{}

void ConditionVariable::wait() {
  waiter = CURRENT_THREAD;
  SoftXMT_suspend();
}

void ConditionVariable::notify() {
  if ( waiter != NULL ) {
    SoftXMT_wake(waiter);
    waiter = NULL;
  }
}
