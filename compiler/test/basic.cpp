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
        
        CHECK_EQ(r, 7);
      }
    });
    CHECK_EQ(alpha, 8);
    
    long x = 1, y = 7;
    long global* xa = make_global(&x);
    long global* ya = make_global(&y);
    
    long global* array = global_alloc<long>(10);
    
    on_all_cores([=]{
      LOG(INFO) << gaddr(xa);
      
      long y = *xa;
      long z = *ya;
      long w = *xa;
      
      LOG(INFO) << "*xa = " << y;
      CHECK(z == 7);
      CHECK(y == w);
      
      long i = (*xa)++;
      CHECK(i >= 1);
      auto dxa = delegate::read(gaddr(xa));
      LOG(INFO) << "dxa = " << dxa;
      CHECK(dxa >= 2 || dxa <= 4);
      CHECK_LT(i, 4);
      LOG(INFO) << "i = " << i << ", *xa = " << *xa;
      
      long j = ++(*xa);
      LOG(INFO) << "j = " << j;
      CHECK_LE(j, 6);
      
      if (mycore() == 0) {
        for (long i=0; i<10; i++) {
          array[i] = i;
        }
        barrier();
      } else {
        barrier();
        long total = 0;
        for (long i=0; i<10; i++) {
          total += array[i];
        }
        LOG(INFO) << "total: " << total;
      }
      
    });
    Metrics::merge_and_dump_to_file();
  });
  finalize();
  return 0;
}
