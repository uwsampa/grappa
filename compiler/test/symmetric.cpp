#define BOOST_TEST_MODULE symmetric_tests
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <Grappa.hpp>
#include <GlobalVector.hpp>
#include <Primitive.hpp>

BOOST_AUTO_TEST_SUITE( BOOST_TEST_MODULE );

using namespace Grappa;

struct Foo {
  long x, y;
  
  void bar(long z) {
    y = z;
    printf("x: %ld, y: %ld, z: %ld\n", x, y, z);
  }
  
} GRAPPA_BLOCK_ALIGNED;

long z;

BOOST_AUTO_TEST_CASE( test1 ) {
  init( GRAPPA_TEST_ARGS );
  run([]{
    auto sa = symmetric_global_alloc<Foo>();
    Foo symmetric* s = sa;
    
    on_all_cores([=]{
      
      s->x = mycore();
      s->y = 12345;
      
      s->bar(0);
      
      z = 7 * mycore();
      
    });
    
    for (Core c=0; c<cores(); c++) {
      CHECK_EQ( delegate::call(c, [=]{ return symm_addr(s)->x; }), c );
    }
    
    // test delegates with symmetric addresses
    long global* z1 = make_global(&z, 1);
    
    s->bar(*z1);
    
    call_on_all_cores([sa,z1]{
      CHECK_EQ(sa->x, mycore());
      if (mycore() == core(z1)) CHECK_EQ(sa->y, z);
    });
    
    ///////////////////////////
    // GlobalVector
    size_t N = 1024;
    auto va = GlobalVector<long>::create(N);
    auto v = as_ptr(va);
    
    CHECK_EQ(v->size(), 0);
    
    on_all_cores([=]{
      auto r = blockDist(0, 10);
      for (int i = r.start; i < r.end; i++) {
        v->push( i );
      }
    });
    
    LOG(INFO) << util::array_str("vector", v->begin(), v->size());
    
  });
  finalize();
}

BOOST_AUTO_TEST_SUITE_END();
