
#ifndef __MEMORYDESCRIPTOR_HPP__
#define __MEMORYDESCRIPTOR_HPP__


class MemoryDescriptor {
public:
  enum Type { Read = 0, Write, AtomicIncrement };
  typedef uint64_t Data;
  typedef uint64_t Address;

public:
  Type type;
  Data data;
  Address address;
  Address source_node;

  char padding1[64];
  bool full;
  char padding2[64];

public:
  MemoryDescriptor( Type type = Read, Address address = 0, Data data = 0 ) 
    : type(type)
    , data(data)
    , address(address)
    , source_node(0)
    , full(false)
  { }

  void setType( Type type ) { this->type = type; }
  void setData( Data data ) { this->data = data; }
  void setAddress( Address address ) { this->address = address; }
  void setFull( bool full ) { this->full = full; }
  void setSourceNode( Address node ) { this->source_node = node; }
  

  Type getType() { return this->type; }
  Data getData() { return this->data; }
  Address getAddress() { return this->address; }
  bool getFull() { return this->full; }
  Address getSourceNode() { return this->source_node; }
};

#endif
