#ifdef NDEBUG
#undef NDEBUG
#endif

#define BOOST_TEST_MODULE basic
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

#include <Grappa.hpp>
#include <Primitive.hpp>


BOOST_AUTO_TEST_SUITE( basic );

using namespace Grappa;

retcode_t do_work(void *i, void *o) {
  auto gaa = reinterpret_cast<long global**>(i);
  auto po = reinterpret_cast<long*>(o);
  
  LOG(INFO) << "ga: " << gaddr(*gaa);
  LOG(INFO) << "po: " << po;
  
  auto p = pointer(*gaa);
  
  LOG(INFO) << "p: " << p;
  
  *po = *p;
  *p += 1;
  return 0;
}

struct Foo {
  long x, y;
  
  void bar() {
    y = x+1;
  }
};

Foo foo;

void test_foo() {
  on_all_cores([]{ foo = { 7, 0 }; });
  
  auto a = as_ptr(make_global(&foo, 1));
  a->bar();
  
  assert(a->x == 7);
  assert(a->y == 8);
  printf("%ld : %ld\n", a->x, a->y);
}

double var;

BOOST_AUTO_TEST_CASE( test1 ) {
  init( GRAPPA_TEST_ARGS );
  run([]{
    
    long alpha = 7;
    long global* g_alpha = make_global(&alpha);
    
    on_all_cores([=]{
      
      LOG(INFO) << gaddr(g_alpha);

      if (mycore() == 1) {
        long global* ga = g_alpha;
        long r = -1;
        grappa_on(0, do_work, &ga, sizeof(ga), &r, sizeof(r));
        
        assert(r == 7);
      }
    });
    assert(alpha == 8);
    
    fprintf(stderr, "-----------------\n");
    
    test_foo();
    
    long x = 1, y1 = 7;
    long global* xa = make_global(&x);
    long global* ya = make_global(&y1);
    
    long global* array = global_alloc<long>(10);
    
    on_all_cores([=]{
      var = 0;
      
      fprintf(stderr, "xa = %d : %p (%p)\n", core(xa), pointer(xa), reinterpret_cast<void*>(xa));
      fprintf(stderr, "ya = %d : %p (%p)\n", core(ya), pointer(ya), reinterpret_cast<void*>(ya));
      
      long y = *xa;
      fprintf(stderr, "*xa = %ld\n", y);
      
      long z = *ya;
      long w = *xa;
//      fprintf(stderr, "w = %ld\n", w);
    
      assert(z == 7);
      assert(y == w);
      
      long i = (*xa)++;
      assert(i >= 1);
//      fprintf(stderr, "i = %ld\n", i);
      
      auto dxa = delegate::read(gaddr(xa));
      fprintf(stderr, "dxa = %ld\n", dxa);
      assert(dxa >= 2 || dxa <= 4);
      assert(i < 4);
      fprintf(stderr, "i = %ld, *xa = %ld\n", i, *xa);
      
      long j = ++(*xa);
      fprintf(stderr, "j = %ld\n", j);
      assert(j <= 6);
      
      if (mycore() == 0) {
        for (long i=0; i<10; i++) {
          assert(array+i == gptr(gaddr(array)+i));
          array[i] = i;
        }
        barrier();
      } else {
        barrier();
        fprintf(stderr, "after barrier\n");
        long total = 0;
        for (long i=0; i<10; i++) {
          total += array[i];
        }
        assert(total == 45);
        fprintf(stderr, "total: %ld\n", total);
      }
      
      
      Core c = (mycore() + 1) % cores();
      auto gvar = as_ptr(make_global(&var, c));
      
      *gvar = 7.0;
      barrier();
      auto val = *gvar;
      fprintf(stderr, "val = %g\n", val);
      assert(val == 7.0);
      
    });
    Metrics::merge_and_dump_to_file();
  });
  finalize();
}

BOOST_AUTO_TEST_SUITE_END();

