#define BOOST_TEST_MODULE branch
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <Grappa.hpp>
#include <Primitive.hpp>

BOOST_AUTO_TEST_SUITE( BOOST_TEST_MODULE );

using namespace Grappa;

void foo(long& x) {
  long global* xa = make_global(&x);
  long y = 0;
  long global* ya = make_global(&y);
  
  if (*xa < 9) {
    *ya = *xa + 6;
  } else {
    (*ya)++;
    while (*ya <= 1) {
      (*ya)++;
      if (*xa) {
        *xa = 1;
      }
    }
  }
  
  CHECK_EQ(x, 1);
  CHECK_EQ(y, 2);
}

BOOST_AUTO_TEST_CASE( test1 ) {
  init( GRAPPA_TEST_ARGS );
  run([]{
    
    long x = 0;
    long global* xa = make_global(&x);
    
    on_all_cores([=]{
      
      if (xa != nullptr) {
        long xx = *xa;
        
        if (*xa == 0) {
          *xa = 7;
        }
        
        (*xa)++;
      }
      
    });
    
    VLOG(0) << "x => " << x;
    CHECK_EQ(x, 9);
    
    foo(x);
    
    Metrics::merge_and_dump_to_file();
  });
  finalize();
}

BOOST_AUTO_TEST_SUITE_END();
