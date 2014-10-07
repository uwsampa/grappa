/*
Query A(yr) :- sp2bench_1m(journal, '<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>', '<http://localhost/vocabulary/bench/Journal>'),
                 sp2bench_1m(journal, '<http://purl.org/dc/elements/1.1/title>', '\"Journal 1 (1940)\"^^<http://www.w3.org/2001/XMLSchema#string>'),
                 sp2bench_1m(journal, '<http://purl.org/dc/terms/issued>', yr)
*/
// grappa
#include <Grappa.hpp>
#include <Collective.hpp>

using namespace Grappa;

// stl
#include <vector>
#include <unordered_map>

// query library
#include "relation_io.hpp"
#include "DoubleDHT.hpp"
#include "utils.h"
#include "stats.h"
#include "strings.h"

// graph gen
#include "generator/make_graph.h"
#include "generator/utils.h"
#include "grappa/timer.h"
#include "grappa/common.h"
#include "grappa/graph.hpp"
#include "prng.h"

template < typename T >                                                                                                
struct Relation {
    GlobalAddress<T> data;
    size_t numtuples;
};  

DEFINE_uint64( nt, 30, "hack: number of tuples"); 



class MaterializedTupleRef_V2_0_1_2 {
    private:
    int64_t _fields[3];
    
    
    public:
    int64_t get(int field) const {
      return _fields[field];
    }
    
    void set(int field, int64_t val) {
      _fields[field] = val;
    }
    
    int numFields() const {
      return 3;
    }
    
    MaterializedTupleRef_V2_0_1_2 () {
      // no-op
    }

    MaterializedTupleRef_V2_0_1_2 (std::vector<int64_t> vals) {
      for (int i=0; i<vals.size(); i++) _fields[i] = vals[i];
    }
    
    std::ostream& dump(std::ostream& o) const {
      o << "Materialized(";
      for (int i=0; i<numFields(); i++) {
        o << _fields[i] << ",";
      }
      o << ")";
      return o;
    }
    
    
  } GRAPPA_BLOCK_ALIGNED;
  std::ostream& operator<< (std::ostream& o, const MaterializedTupleRef_V2_0_1_2& t) {
    return t.dump(o);
  }
  
class MaterializedTupleRef_V1_0_1_2 {
    private:
    int64_t _fields[3];
    
    
    public:
    int64_t get(int field) const {
      return _fields[field];
    }
    
    void set(int field, int64_t val) {
      _fields[field] = val;
    }
    
    int numFields() const {
      return 3;
    }
    
    MaterializedTupleRef_V1_0_1_2 () {
      // no-op
    }

    MaterializedTupleRef_V1_0_1_2 (std::vector<int64_t> vals) {
      for (int i=0; i<vals.size(); i++) _fields[i] = vals[i];
    }
    
    std::ostream& dump(std::ostream& o) const {
      o << "Materialized(";
      for (int i=0; i<numFields(); i++) {
        o << _fields[i] << ",";
      }
      o << ")";
      return o;
    }
    
    
  } GRAPPA_BLOCK_ALIGNED;
  std::ostream& operator<< (std::ostream& o, const MaterializedTupleRef_V1_0_1_2& t) {
    return t.dump(o);
  }

typedef DoubleDHT<int64_t, MaterializedTupleRef_V1_0_1_2, MaterializedTupleRef_V2_0_1_2, std_hash> DHT;
DHT hash0;

GlobalCompletionEvent loop1;
GlobalCompletionEvent loop2;

Relation<MaterializedTupleRef_V1_0_1_2> V1;
Relation<MaterializedTupleRef_V2_0_1_2> V2;

void query() {

  hash0.init_global_DHT( &hash0, cores()*16*1024 );


  {
    V1.data = readTuples<MaterializedTupleRef_V1_0_1_2>( "test1", FLAGS_nt);
    V1.numtuples = FLAGS_nt;
    auto l_V1 = V1;
    on_all_cores([=]{ V1 = l_V1; });
  }

  {
    V2.data = readTuples<MaterializedTupleRef_V2_0_1_2>( "test2", FLAGS_nt);
    V2.numtuples = FLAGS_nt;
    auto l_V2 = V2;
    on_all_cores([=]{ V2 = l_V2; });
  }

  CompletionEvent ce1;
  spawn(&ce1, [=] {
    forall<&loop1>( V1.data, V1.numtuples, [=](int64_t i, MaterializedTupleRef_V1_0_1_2& t_000) {
      hash0.insert_lookup_iter_left<&loop1>(t_000.get(0), t_000, [=](MaterializedTupleRef_V2_0_1_2& t_001) {
        LOG(INFO) << "V1(" << i <<") : (" << t_000 << ") -> " << t_001;
        });
      }); // end  scan over V1
  });

  CompletionEvent ce2;
  spawn(&ce2, [=] {
    forall<&loop2>( V2.data, V2.numtuples, [=](int64_t i, MaterializedTupleRef_V2_0_1_2& t_000) {
      hash0.insert_lookup_iter_right<&loop2>(t_000.get(0), t_000, [=](MaterializedTupleRef_V1_0_1_2& t_001) {
        LOG(INFO) << "V2(" << i << ") : (" << t_000 << ") -> " << t_001;
        });
      }); // end  scan over V2
  });

  ce1.wait();
  ce2.wait();

}


int main(int argc, char** argv) {
    init(&argc, &argv);

    run([] {

double start = Grappa::walltime();
    	query();
      double end = Grappa::walltime();
      query_runtime = end - start;
      //Metrics::merge_and_print();
    });

    finalize();
    return 0;
}
