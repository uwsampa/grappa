
#include <Grappa.hpp>
#include <Collective.hpp>
using namespace Grappa;

int main( int argc, char * argv[] ) {

  init( &argc, &argv );
  
  run( [] {
      on_all_cores( [] {
          LOG(INFO) << "Hello world from locale " << mylocale() << " core " << mycore();
        } );
    } );

  finalize();

  return 0;
}
