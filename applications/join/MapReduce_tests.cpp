#include "MapReduce.hpp"

struct WordCount {
  int64_t word;
  int64_t count;
  WordCount(int64_t w, int64_t c) : word(w), count(c) {}
};

void NumCountMap( const MapperContext<int64_t,int64_t,WordCount>& ctx, int64_t word ) {
  ctx.emitIntermediate( word, 1 );
}

//template < class IntIterable >
//void NumCountReduce( const Reducer<int64_t,int64_t,WordCount>& ctx, int64_t word, IntIterable counts ) {
void NumCountReduce( Reducer<int64_t,int64_t,WordCount>& ctx, int64_t word, std::vector<int64_t> counts ) {
    int64_t sum = 0; 
    for ( auto local_it = counts.begin(); local_it!= counts.end(); ++local_it ) {
      sum += *local_it; 
    }
    emit( ctx, WordCount(word, sum) );
}

struct aligned_int64_t {
  int64_t _x;
} GRAPPA_BLOCK_ALIGNED;
int64_t getX(GlobalAddress<aligned_int64_t> a) {
  return a->_x;
}

int main(int argc, char** argv) {
  Grappa::init(&argc, &argv);
  Grappa::run([=] {
    size_t numw = 1000;
    size_t dictionary_size = 33;
    size_t numred = 2*Grappa::cores();
    GlobalAddress<int64_t> words = Grappa::global_alloc<int64_t>(numw);
    Grappa::forall(words, numw, [=](int64_t i, int64_t& w) {
      w = (i*541) % dictionary_size;
    });

    GlobalAddress<Reducer<int64_t,int64_t,WordCount>> reds = MapReduceJobExecute<int64_t, int64_t, int64_t, WordCount, decltype(NumCountMap), decltype(NumCountReduce)>(words, numw, numred, &NumCountMap, &NumCountReduce); 

    auto counter = Grappa::symmetric_global_alloc<aligned_int64_t>();
    Grappa::forall(reds, numred, [=](int64_t i, Reducer<int64_t, int64_t, WordCount>& r) {
      LOG(INFO) << "Reducer " << i << " has " << r.result->end() - r.result->begin() << " keys";
      for ( auto local_it = r.result->begin(); local_it!= r.result->end(); ++local_it ) {
        if (local_it->count > 0) {
          LOG(INFO) << "Reducer " << i << " (" << local_it->word << ", " << local_it->count << ")";
        }
        counter->_x += local_it->count;
      }
      r.result->clear();
    });

    int64_t total = Grappa::reduce<int64_t, aligned_int64_t, &collective_add, &getX>(counter);
    LOG(INFO) << "total = " << total;
    CHECK( total == numw );
  });
  Grappa::finalize();
}
