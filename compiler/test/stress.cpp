#include <Grappa.hpp>
#include <Primitive.hpp>
#include <Collective.hpp>
#include <GlobalAllocator.hpp>

using namespace Grappa;

DEFINE_int64(N, 1024, "Iterations");

void do_fetch_add(void *i, void *o) {
  auto gaa = reinterpret_cast<long global**>(i);
  auto po = reinterpret_cast<long*>(o);
  
  auto p = pointer(*gaa);
    
  *po = *p;
  *p += 1;
}

int main(int argc, char* argv[]) {
  init(&argc, &argv);
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
  return 0;
}
