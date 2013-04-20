#include "LegacySignaler.hpp"
#include "Grappa.hpp"
  
Signaler::Signaler()
  : waiter ( NULL )
    , done ( false )
{}

void Signaler::wait() {
  if ( !done ) {
    waiter = CURRENT_THREAD;
    Grappa_suspend();
    CHECK( done );
  }
}

void Signaler::signal() {
  done = true;
  if ( waiter != NULL ) {
    Grappa_wake(waiter);
    waiter = NULL;
  }
}


ConditionVariable::ConditionVariable() 
: waiter ( NULL )
{}

void ConditionVariable::wait() {
  waiter = CURRENT_THREAD;
  Grappa_suspend();
}

void ConditionVariable::notify() {
  if ( waiter != NULL ) {
    Grappa_wake(waiter);
    waiter = NULL;
  }
}
