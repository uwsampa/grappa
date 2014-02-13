#include <Grappa.hpp>
#include <Primitive.hpp>
#include <Collective.hpp>
#include <GlobalAllocator.hpp>

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

int main(int argc, char* argv[]) {
  init(&argc, &argv);
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
    
    long x = 1, y = 7;
    long global* xa = make_global(&x);
    long global* ya = make_global(&y);
    
    long global* array = global_alloc<long>(10);
    
    on_all_cores([=]{
      fprintf(stderr, "xa = %d : %p\n", core(xa), pointer(xa));
      fprintf(stderr, "ya = %d : %p\n", core(ya), pointer(ya));
      
      long y = *xa;
      fprintf(stderr, "*xa = %ld\n", y);
      
      long z = *ya;
      long w = *xa;
      
      assert(z == 7);
      assert(y == w);
      
      long i = (*xa)++;
      assert(i >= 1);
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
        fprintf(stderr, "total: %ld\n", total);
      }
      
    });
    Metrics::merge_and_dump_to_file();
  });
  finalize();
  return 0;
}
