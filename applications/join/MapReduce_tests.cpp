#include "MapReduce.hpp"
#include <GlobalAllocator.hpp>

using namespace MapReduce;

//////////////////////////////////////
// Word Count
/////////////////////////////////////
struct WordCount {
  int64_t word;
  int64_t count;
  WordCount(int64_t w, int64_t c) : word(w), count(c) {}
};

void NumCountMap( const MapperContext<int64_t,int64_t,WordCount>& ctx, int64_t word ) {
  ctx.emitIntermediate( word, 1 );
}

void NumCountMapC( const CombiningMapperContext<int64_t,int64_t,WordCount>& ctx, int64_t word ) {
  ctx.emitIntermediate( word, 1 );
}

typedef int32_t TableId;
struct TupleA {
  int64_t fields[3];
};
struct TupleB {
  int64_t fields[2];
};


//template < class IntIterable >
//void NumCountReduce( const Reducer<int64_t,int64_t,WordCount>& ctx, int64_t word, IntIterable counts ) {
void NumCountReduce( Reducer<int64_t,int64_t,WordCount>& ctx, int64_t word, std::vector<int64_t> counts ) {
    int64_t sum = 0; 
    int64_t i = 0;
    for ( auto local_it = counts.begin(); local_it!= counts.end(); ++local_it ) {
      sum += *local_it; 
      ++i;
    }
    emit( ctx, WordCount(word, sum) );
    VLOG(1) << "reducer key " << word << " processed " << i << " values";
}

void NumCountCombiner( const CombiningMapperContext<int64_t,int64_t,WordCount>& ctx, int64_t word, std::vector<int64_t> counts ) {
  int64_t sum = 0; 
  for ( auto local_it = counts.begin(); local_it!= counts.end(); ++local_it ) {
    sum += *local_it; 
  }
  ctx.emitCombinedIntermediate( word, sum );
}
    
  
   

struct aligned_int64_t {
  int64_t _x;
} GRAPPA_BLOCK_ALIGNED;
int64_t getX(GlobalAddress<aligned_int64_t> a) {
  return a->_x;
}

void test_map_on_array() {
  LOG(INFO) << "test_map_on_array";
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
      VLOG(1) << "Reducer " << i << " has " << r.result->end() - r.result->begin() << " keys";
      for ( auto local_it = r.result->begin(); local_it!= r.result->end(); ++local_it ) {
        if (local_it->count > 0) {
          VLOG(2) << "Reducer " << i << " (" << local_it->word << ", " << local_it->count << ")";
        }
        counter->_x += local_it->count;
      }
      r.result->clear();
    });

    int64_t total = Grappa::reduce<int64_t, aligned_int64_t, &collective_add, &getX>(counter);
    LOG(INFO) << "total = " << total;
    CHECK( total == numw );
}

void test_map_on_array_combining() {
      LOG(INFO) << "test_map_on_array_combining";
    size_t numw = 1000;
    size_t dictionary_size = 33;
    size_t numred = 2*Grappa::cores();
    GlobalAddress<int64_t> words = Grappa::global_alloc<int64_t>(numw);
    Grappa::forall(words, numw, [=](int64_t i, int64_t& w) {
      w = (i*541) % dictionary_size;
    });

    auto reds = allocateReducers<int64_t,int64_t,WordCount>( numred );
    auto combiners = allocateCombiners<int64_t,int64_t>( );

    CombiningMapReduceJobExecute<int64_t, int64_t, int64_t, WordCount, decltype(NumCountMapC), decltype(NumCountCombiner), decltype(NumCountReduce)>(words, numw, reds, combiners, numred, &NumCountMapC, &NumCountCombiner, &NumCountReduce); 

    auto counter = Grappa::symmetric_global_alloc<aligned_int64_t>();
    Grappa::forall(reds, numred, [=](int64_t i, Reducer<int64_t, int64_t, WordCount>& r) {
      VLOG(1) << "Reducer " << i << " has " << r.result->end() - r.result->begin() << " keys";
      for ( auto local_it = r.result->begin(); local_it!= r.result->end(); ++local_it ) {
        if (local_it->count > 0) {
          VLOG(2) << "Reducer " << i << " (" << local_it->word << ", " << local_it->count << ")";
        }
        counter->_x += local_it->count;
      }
      r.result->clear();
    });

    int64_t total = Grappa::reduce<int64_t, aligned_int64_t, &collective_add, &getX>(counter);
    LOG(INFO) << "total = " << total;
    CHECK( total == numw );
}

template <typename T>
struct AlignedVec {
  std::vector<T> data;
} GRAPPA_BLOCK_ALIGNED;

void test_map_on_symmetric_randomAccess() {
      LOG(INFO) << "test_map_on_symmetric_randomAccess";
    size_t numw_per_core = 100;
    size_t numw = numw_per_core*Grappa::cores();
    size_t dictionary_size = 33;
    size_t numred = 2*Grappa::cores();
    auto words = Grappa::symmetric_global_alloc<AlignedVec<int64_t>>();
    // initialize
    Grappa::on_all_cores([=] {
        words->data = std::vector<int64_t>();
        for (int i=0; i<numw_per_core; i++) {
          words->data.push_back((Grappa::mycore()*numw_per_core + i*541) % dictionary_size);
        }
    });

    GlobalAddress<Reducer<int64_t,int64_t,WordCount>> reds = MapReduceJobExecute<int64_t, int64_t, int64_t, WordCount, decltype(NumCountMap), decltype(NumCountReduce)>(words, numred, &NumCountMap, &NumCountReduce); 

    auto counter = Grappa::symmetric_global_alloc<aligned_int64_t>();
    Grappa::forall(reds, numred, [=](int64_t i, Reducer<int64_t, int64_t, WordCount>& r) {
      VLOG(1) << "Reducer " << i << " has " << r.result->end() - r.result->begin() << " keys";
      for ( auto local_it = r.result->begin(); local_it!= r.result->end(); ++local_it ) {
        if (local_it->count > 0) {
          VLOG(2) << "Reducer " << i << " (" << local_it->word << ", " << local_it->count << ")";
        }
        counter->_x += local_it->count;
      }
      r.result->clear();
    });

    int64_t total = Grappa::reduce<int64_t, aligned_int64_t, &collective_add, &getX>(counter);
    LOG(INFO) << "total = " << total;
    CHECK( total == numw );
}


int main(int argc, char** argv) {
  Grappa::init(&argc, &argv);
  Grappa::run([=] {
    test_map_on_array();
    test_map_on_symmetric_randomAccess();
    test_map_on_array_combining();
  });
  Grappa::finalize();
}
