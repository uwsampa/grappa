#ifndef _UID_HPP_
#define _UID_HPP_

#include "SoftXMT.hpp"
#include <queue>

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

#endif // _UID_HPP_
