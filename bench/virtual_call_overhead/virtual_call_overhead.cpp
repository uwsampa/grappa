
#include <cassert>
#include <time.h>
#include <unistd.h>
#include <cstdio>

#include <boost/scoped_ptr.hpp>

// option 1: dynamic dispatch

class Interface {
public:
  virtual int64_t foo() = 0;
  virtual void reset() = 0;
};

class Instance : public Interface {
private:
  int64_t val;
  int inc;
public:
  Instance(int inc) : Interface(), val(0), inc(inc) { }
  inline int64_t foo() __attribute__ ((always_inline));
  void reset() { val = 0; }
};

inline int64_t Instance::foo() {
  return val += inc;
}

// option 2: inlineable
class Inlineable {
private:
  int64_t val;
  int inc;
public:
  Inlineable( int inc ) : val(0), inc(inc) { }
  inline int64_t foo() __attribute__ ((always_inline));
  void reset() { val = 0; }
};

inline int64_t Inlineable::foo() {
  return val += inc;
}


// option 3: no inline

class NotInlineable {
private:
  int64_t val;
  int inc;
public:
  NotInlineable( int inc ) : val(0), inc(inc) { }
  int64_t foo() __attribute__ ((noinline));
  void reset() { val = 0; }
};

int64_t NotInlineable::foo() {
  return val += inc;
}

// option 4: PIMPL

template< typename T >
class Pimpl {
private:
  T * inst;
public:
  Pimpl( T * inst) : inst( inst ) { }
  inline int64_t foo() __attribute__ ((always_inline));
  void reset() { inst->reset(); }
};

template< typename T >
inline int64_t Pimpl<T>::foo() {
  return inst->foo();
}



void print( int64_t max, int64_t sum, timespec * start, timespec * end, const char * str ) {
  double runtime = ( ((double) end->tv_sec + end->tv_nsec / 1000000000.0) - 
		     ((double) start->tv_sec + start->tv_nsec / 1000000000.0) );
  char hostname[ 1024 ] = { 0 };
  gethostname( hostname, 64 );
  printf("header, hostname, call method, iterations, runtime, ns per iteration\n");
  printf("data, %s, %s, %ld, %f, %f\n",
	 hostname, str, max, runtime, 1000000000.0 * runtime / max);
  printf("%10s: %30s: %ld iterations in %2.5f s == %2.5f ns/it\n", 
	 hostname, str, max, runtime, 1000000000.0 * runtime / max);
  fflush(stdout);
}


int main( int argc, char * argv[] ) {
  
  int max = 2000000000;
  if( argc >= 2 ) {
    max = atoi( argv[1] );
  }


  int inc = 1;
  if( argc >= 3 ) {
    inc = atoi( argv[2] );
  }

  struct timespec start;
  struct timespec end;
  int64_t sum = 0;
  
  // option 1
  Instance inst = Instance( inc );
  Interface * virt = &inst;

  // option 1a
  Instance * instp = &inst;

  // option 2
  Inlineable inl = Inlineable( inc );

  // option 3
  NotInlineable ninl = NotInlineable( inc );

  // option 4
  Pimpl< Inlineable > pimpl = Pimpl< Inlineable >( new Inlineable( inc ) );

  // option 5
  Pimpl< NotInlineable > pimpln = Pimpl< NotInlineable >( new NotInlineable( inc ) );







  for( int i = 0; i < 3; ++i ) {
    // instance
    clock_gettime(CLOCK_MONOTONIC, &start);
    instp->reset();
    for( sum = 0; sum < max; ) {
      sum = instp->foo();
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    print( max, sum, &start, &end, "instance" );

    // inline
    clock_gettime(CLOCK_MONOTONIC, &start);
    inl.reset();
    for( sum = 0; sum < max; ) {
      sum = inl.foo();
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    print( max, sum, &start, &end, "inlineable direct" );

    // not inline
    clock_gettime(CLOCK_MONOTONIC, &start);
    ninl.reset();
    for( sum = 0; sum < max; ) {
      sum = ninl.foo();
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    print( max, sum, &start, &end, "not inlineable direct" );

    // pimpl not inline
    clock_gettime(CLOCK_MONOTONIC, &start);
    pimpln.reset();
    for( sum = 0; sum < max; ) {
      sum = pimpln.foo();
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    print( max, sum, &start, &end, "pimpl not inlineable" );

    // pimpl inline
    clock_gettime(CLOCK_MONOTONIC, &start);
    pimpl.reset();
    for( sum = 0; sum < max; ) {
      sum = pimpl.foo();
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    print( max, sum, &start, &end, "pimpl inlineable" );


    // virtual
    clock_gettime(CLOCK_MONOTONIC, &start);
    virt->reset();
    for( sum = 0; sum < max; ) {
      sum = virt->foo();
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    print( max, sum, &start, &end, "virtual" );

  }
  return 0;
}
