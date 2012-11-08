
#include <cxxabi.h>
#include <iostream>

#include <functional>
#include "Message.hpp"

// template< typename T >
// class FPFunctor {
// private:
//   T * fp_;
// public:
//   FPFunctor( T t )
//     : fp_( t )
//   { }
//   void operator()() {
//     fp_();
//   }
// };

// template< typename T >
// void makeFPFunctor( T t ) {
//   return FPFunctor<T>( t );
// }

void foo() {
  std::cout << "foo" << std::endl;
}

void splat( int x ) {
  std::cout << "splat x = " << x << std::endl;
}

// template< typename T >
// void argh( T t ) {
//   t();
// }


template< typename T >
class FPFunctor {
private:
  T fp_;
public:
  FPFunctor( T t )
    : fp_( t )
  { }
  void operator()() {
    fp_();
  }
};

template< typename T >
FPFunctor<T> wrapItUp( T t ) {
  return FPFunctor<T>(t);
}

template< typename T >
void exec( T t ) {
  char * name = abi::__cxa_demangle( typeid( t ).name(), NULL, NULL, NULL );
  std::cout << "Calling operator() on type " << name << " of size " << sizeof(t) << std::endl;
  t();
}

class Foo {
public:
  void operator()() {
    std::cout << "Class foo" << std::endl;
  }
};

#define GCM( n, d, f ) auto n ## _lambda = f; Grappa::Message< decltype( n ## _lambda ) > n( d, n ## _lambda )

int main() {

  int x = 0;
  int y = 0;

  auto noArg = wrapItUp( []{ std::cout << "bar" << std::endl; } );
  auto foop = wrapItUp( foo );
  auto fooc = wrapItUp( Foo() );
  auto oneArg = wrapItUp( [=]{ std::cout << "bar with x = " << x << std::endl; } );
  auto twoArg = wrapItUp( [=]{ std::cout << "bar with x = " << x << " and y = " << y << std::endl; } );
  x = 100;
  y = 200;

  std::cout << "x is " << x << ", y is " << y << std::endl;

  exec( noArg );
  exec( foop );
  exec( fooc );
  exec( oneArg );
  exec( twoArg );

  exec( std::bind( foo ) );
  exec( [] { foo(); } );

  auto f = [] ( int a, double b ) { std::cout << "a is " << a << " b is " << b << std::endl; };
  exec( std::bind( f, 1, 2.3 ) );

  auto g = [] { std::cout << "bind" << std::endl; };
  exec( std::bind( g ) );

  auto gx = [=] { std::cout << "bind with x = " << x << std::endl; };
  exec( std::bind( gx ) );

  auto gxy = [=] { std::cout << "bind with x = " << x << " and y = " << y << std::endl; };
  exec( std::bind( gxy ) );


  exec( std::bind( [] (int a, double b) {
	std::cout << "a is " << a << " b is " << b << std::endl;
      }, 4, 5.6 ) );



  // Grappa::Message< FPFunctor<void(void)> > m( wrapItUp( [] {
  // 	std::cout << "Hello!" << std::endl;
  //     } ) );
  Grappa::Message< std::function<void(void)> > m1( 0, std::function<void(void)>( [] {
	std::cout << "Hello!" << std::endl;
      } ) );
  Grappa::Message< std::function<void(void)> > m2( 0, std::function<void(void)>( [] {
  	std::cout << "Goodbye!" << std::endl;
      } ) );
  m1.stitch( &m2 );

  GCM( m3, 0, foop );
  m2.stitch( &m3 );

  GCM( m4, 0, fooc );
  m3.stitch( &m4 );
  
  //GCM( m5, []{ std::cout << "Foo" << std::endl; } );

  auto m5 = Grappa::scopedMessage( 0, []{ std::cout << "Foo" << std::endl; } );
  m4.stitch( &m5 );

  auto m6 = Grappa::scopedMessage( 0, [=]{ std::cout << "Haha x = " << x << std::endl; } );
  m5.stitch( &m6 );
  
  auto m7 = Grappa::scopedMessage( 0, fooc );
  m6.stitch( &m7 );
  
  auto m8 = Grappa::scopedMessage( 0, [] { foo(); } );
  m7.stitch( &m8 );
  
  exec( std::bind( splat, x ) );
  auto m9 = Grappa::scopedMessage( 0, std::bind( splat, x ) );
  m8.stitch( &m9 );
  
  exec( [=] { splat(x); } );
  auto m10 = Grappa::scopedMessage( 0, [=] { splat(x); } );
  m9.stitch( &m10 );
  
  exec( [=] { 
      [] (int xx) {
  	splat(xx); 
      }(x);
    } );
  auto m11 = Grappa::scopedMessage( 0, [=] { [] (int xx) { splat(xx); } (x); } );
  m10.stitch( &m11 );
  
  char buf[4096] = {0};
  
  std::cout << "Serializing" << std::endl;
  char * end = Grappa::impl::MessageBase::serialize_to_buffer( buf, &m1 );
  x++;  
  // for( intptr_t * i = (intptr_t*)buf; i < (intptr_t*)end; ++i ) {
  //   std::cout << i << ": " << (void*) *i << std::endl;
  // }

  std::cout << "Deserializing" << std::endl;
  Grappa::impl::MessageBase::deserialize_buffer( buf, end - buf );

  std::cout << "End x = " << x << std::endl;
}
