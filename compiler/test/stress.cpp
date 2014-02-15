#define BOOST_TEST_MODULE stress
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <Grappa.hpp>
#include <Primitive.hpp>

BOOST_AUTO_TEST_SUITE( BOOST_TEST_MODULE );

using namespace Grappa;

DEFINE_int64(N, 1024, "Iterations");

int16_t do_fetch_add(void *i, void *o) {
  auto gaa = reinterpret_cast<long global**>(i);
  auto po = reinterpret_cast<long*>(o);
  
  auto p = pointer(*gaa);
    
  *po = *p;
  *p += 1;
  
  return 0;
}

BOOST_AUTO_TEST_CASE( test1 ) {
  init( GRAPPA_TEST_ARGS );
  run([]{
    long c = 0;
    long global* gc = make_global(&c);
    
    on_all_cores([=]{
      auto _gc = gc;
      for (long i=0; i < FLAGS_N; i++) {
        long r;
        grappa_on(0, do_fetch_add, &_gc, sizeof(gc), &r, sizeof(r));
        CHECK_LT(r, FLAGS_N * cores());
      }
    });
    
    LOG(INFO) << "success!";
  });
  finalize();
}

BOOST_AUTO_TEST_SUITE_END();
