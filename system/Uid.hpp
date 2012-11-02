// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#ifndef UID_HPP
#define UID_HPP

#include "Grappa.hpp"
#include <queue>

/// Distributed allocator of a range of contiguous IDs.
class UIDManager {
  private:
    uint64_t start;
    uint64_t end;

    int obj_id;
    Node * neighbors;

    int nextVictim;

    std::queue<Thread*> * waiters;
    bool stealLock;

    // managing global UIDManagers
    static int next_obj_id;
    static UIDManager * objects[];

    bool askLocal( uint64_t * result );
    bool stealRequest( uint64_t * r_start, uint64_t * r_end );
    void randomStealId( );
    
    struct RemoteIdSteal {
      uint64_t r_start;
      uint64_t r_end;
      int object_id;
      bool success;
      void operator()() {
        success = objects[object_id]->stealRequest( &r_start, &r_end );
      }
    };
  public:
    UIDManager();

    void init( uint64_t start, uint64_t end, Node * neighbors );
    uint64_t getUID();
};

#endif // UID_HPP
