
#ifndef __MEMORYDESCRIPTOR_HPP__
#define __MEMORYDESCRIPTOR_HPP__

#include <iostream>

class MemoryDescriptor {
public:
  enum Type { Read = 0, Write, AtomicIncrement, Quit };
  enum Tag { RMA_Response = 1 << 12, RMA_Request }; // For MPI messages
  typedef uint64_t Data;
  typedef uint64_t Address;

public:
  Type type;
  Data data;
  Address address;
  Address index;
  Address node;

  char padding1[64];
  bool full;
  char padding2[64];

public:
  MemoryDescriptor( Type type = Read, Address address = 0, Data data = 0 ) 
    : type(type)
    , data(data)
    , address(address)
    , index(0)
    , node(0)
    , full(false)
  { }

};

std::ostream& operator<<( std::ostream& o, const MemoryDescriptor& md ) {
  o << "[MD" 
    << " type:" << md.type
    << " data:" << md.data
    << " address:" << md.address
    << " index:" << md.index
    << " node:" << md.node
    << " full:" << md.full
    << "]";
  return o;
}

#endif
