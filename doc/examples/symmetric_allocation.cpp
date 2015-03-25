//
// Some examples of symmetric allocation in Grappa
//

#include <Grappa.hpp>
#include <FullEmpty.hpp>


//
// the dreaded global scope
//
int global_foo = 0;

void global_fn() {
  Grappa::on_all_cores( [] { // no capture necessary
      global_foo++;
    } );
  
  int foo_sum = Grappa::reduce<int,collective_add>(&global_foo);
  CHECK_EQ( foo_sum, Grappa::cores() );
}


//
// another variant of global scope, but with a namespace to avoid collisions
//
namespace mylibrary {
  int namespace_foo = 0; // only visible in mylibrary namespace
}

void namespace_fn() {
  Grappa::on_all_cores( [=] {
      mylibrary::namespace_foo++;
    } );

  int foo_sum = Grappa::reduce<int,collective_add>(&mylibrary::namespace_foo);
  CHECK_EQ( foo_sum, Grappa::cores() );
}

//
// another variant of global scope, but with a namespace to avoid collisions
//
namespace { // anonymous namespace
  int namespace_foo = 0; // only visible in this translation unit
}

void anonymous_namespace_fn() {
  Grappa::on_all_cores( [=] {
      namespace_foo++;
    } );

  int foo_sum = Grappa::reduce<int,collective_add>(&namespace_foo);
  CHECK_EQ( foo_sum, Grappa::cores() );
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
  
  Grappa::on_all_cores( [] { // no capture necessary
      foo++;
    } );

  int foo_sum = Grappa::reduce<int,collective_add>(&foo);
  CHECK_EQ( foo_sum, Grappa::cores() );
}

//
// Another example of symmetric allocation with a static local 
//
void blocking_full_read() {
  static Grappa::FullEmpty< int > d; // allocate one per core; a zero-initialized FullEmpty<> starts empty

  Grappa::on_all_cores( [] { // no capture necessary
      d.writeXF(1);
    } );
  
  int sum = 0;
  for( int i = 0; i < Grappa::cores(); ++i ) {
    sum += Grappa::readFF( make_global( &d, i ) );
  }
  CHECK_EQ( sum, Grappa::cores() );
}


//
// Dynamic symmetric allocation
//
// In the current version, this is messier under the hood than it should be.
// A recent commit is required to support allocating non-64-byte-multiple objects.
int symmetric_fn() {
  GlobalAddress< int > foo_p = Grappa::symmetric_global_alloc< int >();

  Grappa::on_all_cores( [=] { // must capture global pointer
      int * foo = foo_p.localize();
      *foo = 1;
    } );
  
  int foo_sum = Grappa::reduce<int,collective_add>(foo_p);
  CHECK_EQ( foo_sum, Grappa::cores() );
}



int main(int argc, char * argv[]) {
  Grappa::init( &argc, &argv );

  Grappa::run( [] {
      global_fn();
      namespace_fn();
      anonymous_namespace_fn();
      static_fn();
      blocking_full_read();
      symmetric_fn();
    } );

  Grappa::finalize();
  return 0;
}

