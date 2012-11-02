// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include "Uid.hpp"
#include "Delegate.hpp"

// just determines space overhead equal to sizeof(ptr)*MAX_OBJECTS
#define MAX_OBJECTS 2

int UIDManager::next_obj_id = 0;
UIDManager * UIDManager::objects[MAX_OBJECTS];

UIDManager::UIDManager()
      : start( 0 )
      , end( 0 )
      , nextVictim( 0 ) 
      , neighbors( NULL ) 
      , waiters( new std::queue<Thread*>() ) 
      , stealLock( true ) {
      obj_id = next_obj_id++;
      CHECK( obj_id < MAX_OBJECTS );
      objects[obj_id] = this;
    }

void UIDManager::init( uint64_t start_, uint64_t end_, Node * neighbors_ ) {
  this->start = start_;
  this->end = end_;
  this->neighbors = neighbors_;
}

/// Blocks until return a new unique id
uint64_t UIDManager::getUID() {
  uint64_t result;
  while ( !askLocal( &result ) ) {
    if ( stealLock ) {
      stealLock = false;
      randomStealId();
      stealLock = true;
      while ( !waiters->empty() ) {
        Grappa_wake( waiters->front() ); 
        waiters->pop();
      }
    } else {
      waiters->push( CURRENT_THREAD );
      Grappa_suspend();
    }
  }
  return result;
}

bool UIDManager::askLocal( uint64_t * result ) {
  if ( start < end ) {
    *result = start++;
    VLOG(5) << "askLocal got "<<*result<<" now ("<<start<<","<<end<<")";
    return true;
  } else {
    VLOG(5) << "askLocal failed because ("<<start<<","<<end<<")";
    return false;
  }
}

#define max_int(a,b) ((a)>(b)) ? (a) : (b)
bool UIDManager::stealRequest( uint64_t * r_start, uint64_t * r_end ) {
  if ( start < end ) {
    uint64_t amount = end - start;
    
    // amount must be >=1 since need to provide id to someone if we have any at all to avoid application deadlock
    uint64_t steal_amount = max_int( amount / 2, 1 ); 
    
    *r_end = end;
    end -= steal_amount;
    *r_start = end;
    CHECK( (*r_end - *r_start) + (end - start) == amount );
    VLOG(5) << "success steal amount="<<steal_amount<<" keep=("<<start<<","<<end<<") give=("<<*r_start<<","<<*r_end<<")";
    return true;
  } else {
    VLOG(5) << "failed steal end="<<end << " start="<<start;
    return false;
  }
}

void UIDManager::randomStealId( ) {
  CHECK( neighbors != NULL );
  while (true) {
    int vicId = neighbors[nextVictim];
    nextVictim = (nextVictim+1) % Grappa_nodes();

    if (vicId==Grappa_mynode()) continue; // don't steal from myself

    RemoteIdSteal func;
    func.object_id = obj_id;
    Grappa_delegate_func( &func, vicId );
    if ( func.success ) {
      start = func.r_start;
      end = func.r_end;
      break;
    }
  }
}
