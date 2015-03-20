//
// Some examples of symmetric allocation in Grappa
//

#include <Grappa.hpp>

using namespace Grappa;


//
// the dreaded global scope
//
int global_foo = 0;

void global_fn() {
  on_all_cores( [] { // no capture necessary
      global_foo++;
    } );
  
  int foo_sum = Grappa::reduce<int,collective_add>(&global_foo);
  CHECK_EQ( foo_sum, cores() );
}


//
// another variant of global scope, but with namespaces to avoid collisions
//
namespace mylibrary {
  int namespace_foo = 0; // only visible in mylibrary namespace
}
namespace { // anonymous namespace
  int namespace_foo = 0; // only visible in this translation unit
}
void namespace_fn() {
  on_all_cores( [=] {
      mylibrary::namespace_foo++;
      namespace_foo = 0;
    } );

  int foo_sum = Grappa::reduce<int,collective_add>(&mylibrary::namespace_foo);
  CHECK_EQ( foo_sum, cores() );

  on_all_cores( [=] {
      mylibrary::namespace_foo = 0;
      namespace_foo++;
    } );

  foo_sum = Grappa::reduce<int,collective_add>(&namespace_foo);
  CHECK_EQ( foo_sum, cores() );
}


//
// Dynamic symmetric allocation
//
// In the current version, this is messier under the hood than it should be.
// A recent commit is required to support allocating non-64-byte-multiple objects.
int symmetric_fn() {
  GlobalAddress< int > foo_p = symmetric_global_alloc< int >();

  on_all_cores( [=] { // must capture global pointer
      int * foo = foo_p.localize();
      *foo = 1;
    } );
  
  int foo_sum = Grappa::reduce<int,collective_add>(foo_p);
  CHECK_EQ( foo_sum, cores() );
}


//
// Symmetric allocation with a static local
//
// For symmetric variables that can be zero-initialized and are
// allocated in non-reentrant code, this is the simplest mechanism.
//
// Zero-initialization is required to be done automatically for static
// locals when the program starts; it doesn't need to be specified by
// the programmer.
//
// Constant initialization is a little fuzzy; most compilers will do
// it when the program starts, but they are allowed to do it any time
// up to the first time the code is executed. If multiple cores
// execute the code, an interleaving might occur that would cause data
// loss. Best to avoid this to be sure.
//
// Types that require running a constructor for initialization will
// only have their constructor called the first time the code is
// executed on each core. In this case, that happens only on core 0;
// other cores would have to call the construtor explicitly.
//
int static_fn() {
  static int foo; // scope is limited to this block
  
  on_all_cores( [] { // no capture necessary
      foo++;
    } );

  int foo_sum = Grappa::reduce<int,collective_add>(&foo);
  CHECK_EQ( foo_sum, cores() );
}

//
// Another example of symmetric allocation with a static local 
//
void blocking_full_read() {
  static FullEmpty< int > d; // allocate one per core; a zero-initialized FullEmpty<> starts empty

  on_all_cores( [] { // no capture necessary
      d.writeXF(1);
    } );
  
  int sum = 0;
  for( int i = 0; i < cores(); ++i ) {
    sum += delegate::call( i, [] { return d.readFE(); } );
  }
  CHECK_EQ( sum, cores() );
}



int main(int argc, char * argv[]) {
  init( &argc, &argv );

  run( [] {
      global_fn();
      namespace_fn();
      static_fn();
      symmetric_fn();
      blocking_full_read();
    } );

  finalize();
  return 0;
}

