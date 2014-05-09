#include "HashJoin.hpp"
#include <GlobalAllocator.hpp>

using namespace Grappa;

struct Tuple1 {
  int64_t k;
  int64_t v;

  void set(int i, int64_t v) {
    if (i==0) this->k = v;
    else this->v = v;
  }
  int64_t get(int i) const {
    if (i==0) return this->k;
    else return this->v;
  }
  int numFields() const {
    return 2;
  }
    std::ostream& dump(std::ostream& o) const {
      o << "Materialized(";
        o << k << "," << v;
      o << ")";
      return o;
    }
}GRAPPA_BLOCK_ALIGNED;

struct Tuple2 {
  int64_t k;
  int64_t v;
  void set(int i, int64_t v) {
    if (i==0) this->k = v;
    else this->v = v;
  }
  int64_t get(int i) const {
    if (i==0) return this->k;
    else return this->v;
  }
  int numFields() const {
    return 2;
  }
    std::ostream& dump(std::ostream& o) const {
      o << "Materialized(";
        o << k << "," << v;
      o << ")";
      return o;
    }
}GRAPPA_BLOCK_ALIGNED;

struct Tuple3 {
  int64_t fields[4];
  void set(int i, int64_t v) {
    fields[i] = v;
  }
  int64_t get(int i) {
    return fields[i];
  }
  int numFields() const {
    return 4;
  }
    std::ostream& dump(std::ostream& o) const {
      o << "Materialized(";
      for (int i=0; i<numFields(); i++) {
        o << fields[i] << ",";
      }
      o << ")";
      return o;
    }
} GRAPPA_BLOCK_ALIGNED;

  
std::ostream& operator<< (std::ostream& o, const Tuple1& t) {
    return t.dump(o);
  }
std::ostream& operator<< (std::ostream& o, const Tuple2& t) {
    return t.dump(o);
  }
std::ostream& operator<< (std::ostream& o, const Tuple3& t) {
    return t.dump(o);
  }



void test_hash_join_array() {
  LOG(INFO) << "test_hash_join_array";

    auto leftnum = 10;
    auto rightnum = 7;
    auto leftTuples = Grappa::global_alloc<Tuple1>(leftnum);
    auto rightTuples = Grappa::global_alloc<Tuple2>(rightnum);
    auto numred = cores();

    auto reducers = allocateJoinReducers<int64_t,Tuple1,Tuple2,Tuple3>(numred); 
    auto ctx = HashJoinContext<int64_t,Tuple1,Tuple2,Tuple3>(reducers, numred);

    forall(leftTuples, leftnum, [=](int64_t i, Tuple1& t) {
        t.k = i;
        t.v = i*1000;
        ctx.emitIntermediateLeft( t.k, t );
    });
    
    forall(rightTuples, rightnum, [=](int64_t i, Tuple2& t) {
        t.k = 2*i;
        t.v = i*10;
        ctx.emitIntermediateRight( t.k, t );
    });

    ctx.reduceExecute();


    Grappa::forall(reducers, numred, [=](int64_t i, JoinReducer<int64_t,Tuple1,Tuple2,Tuple3>& r) {
      VLOG(1) << "Reducer " << i << " has " << r.result->end() - r.result->begin() << " keys";
      for ( auto local_it = r.result->begin(); local_it!= r.result->end(); ++local_it ) {
        VLOG(1) << *local_it;        
      }
      r.result->clear();
    });

    freeJoinReducers(reducers, numred);

}


int main(int argc, char** argv) {
  Grappa::init(&argc, &argv);
  Grappa::run([=] {
    test_hash_join_array();
  });
  Grappa::finalize();
}
