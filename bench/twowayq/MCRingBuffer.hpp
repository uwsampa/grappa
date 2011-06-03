#ifndef __MCRINGBUFFER_HPP__
#define __MCRINGBUFFER_HPP__

#include <stdint.h>

template< typename T = uint64_t, 
	  int buffer_size_log = 7, 
	  int batch_size = 64,
	  int cache_line_size = 64 >
class MCRingBuffer {
private:

  char pad0[cache_line_size];

  // producer
  int localRead;
  int nextWrite;
  int wBatch;
  char pad1[cache_line_size - 3 * sizeof(int)];

  // control
  volatile int read;
  volatile int write;
  char pad2[cache_line_size - 2 * sizeof(int)];

  // consumer
  int localWrite;
  int nextRead;
  int rBatch;
  char pad3[cache_line_size - 3 * sizeof(int)];

  // constants
  const int max;
  char pad4[cache_line_size - 1 * sizeof(int)];

  uint64_t buffer[1 << buffer_size_log];

  char pad5[cache_line_size];

public:
  MCRingBuffer() 
    : localRead(0)
    , nextWrite(0)
    , wBatch(0)
    , read(0)
    , write(0)
    , localWrite(0)
    , nextRead(0)
    , rBatch(0)
    , max(1 << buffer_size_log)
  { }

  inline int next(int x) {
    return (x + 1) & (max - 1);
  }
   
  inline bool produce(uint64_t element) {
    int afterNextWrite = next(nextWrite);
    if (afterNextWrite == localRead) {
      if (afterNextWrite == read) {
	return false;
      }
      localRead = read;
    }
    buffer[nextWrite] = element;
    nextWrite = afterNextWrite;
    wBatch++;
    if (wBatch >= batch_size) {
      write = nextWrite;
      wBatch = 0;
    }
    return true;
  }
  
  inline void flush() {
    write = nextWrite;
    wBatch = 0;
  }

  inline bool consume(uint64_t* element) {
    if (nextRead == localWrite) {
      if (nextRead == write) {
	return false;
      }
      localWrite = write;
    }
    *element = buffer[nextRead];
    nextRead = next(nextRead);
    rBatch++;
    if (rBatch >= batch_size) {
      read = nextRead;
      rBatch = 0;
    }
    return true;
  }

  inline bool can_consume() {
      if (nextRead==localWrite) {
          if (nextRead==write) {
              return false;
          }
      }
      return true;
  }
 
    
};

#endif
