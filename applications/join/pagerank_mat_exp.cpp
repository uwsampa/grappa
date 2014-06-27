/*
Query ranks(src, 1.0) :- edges(src, dst)

sizes(src, count(dst)) :- edges(src, dst)

newranks(vid, sum(rank/len)) :- edges(src, vid), sizes(src, len), ranks(src, rank)

*/
// grappa
#include <Grappa.hpp>
#include <Collective.hpp>
#include <GlobalCompletionEvent.hpp>

using namespace Grappa;

// stl
#include <vector>
#include <unordered_map>

// query library
#include "relation_io.hpp"
#include "MatchesDHT.hpp"
#include "DoubleDHT.hpp"
#include "DHT_symmetric.hpp"
#include "Aggregates.hpp"
#include "utils.h"
#include "stats.h"
#include "strings.h"
#include "relation.hpp"

// graph gen
#include "generator/make_graph.h"
#include "generator/utils.h"
#include "grappa/timer.h"
#include "grappa/common.h"
#include "grappa/graph.hpp"
#include "prng.h"


DEFINE_uint64( nt, 30, "hack: number of tuples"); 

GlobalCompletionEvent jm_sync;

          // can be just the necessary schema
  class MaterializedTupleRef_V1_0_1 {

    public:
    int64_t _fields[2];
    

    int64_t get(int field) const {
      return _fields[field];
    }
    
    void set(int field, int64_t val) {
      _fields[field] = val;
    }
    
    int numFields() const {
      return 2;
    }
    
    MaterializedTupleRef_V1_0_1 () {
      // no-op
    }

    MaterializedTupleRef_V1_0_1 (std::vector<int64_t> vals) {
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
  std::ostream& operator<< (std::ostream& o, const MaterializedTupleRef_V1_0_1& t) {
    return t.dump(o);
  }

  

          // can be just the necessary schema
  class MaterializedTupleRef_V2_0_1_2_3_4_5_6 {

    public:
    int64_t _fields[7];
    

    int64_t get(int field) const {
      return _fields[field];
    }
    
    void set(int field, int64_t val) {
      _fields[field] = val;
    }
    
    int numFields() const {
      return 7;
    }
    
    MaterializedTupleRef_V2_0_1_2_3_4_5_6 () {
      // no-op
    }

    MaterializedTupleRef_V2_0_1_2_3_4_5_6 (std::vector<int64_t> vals) {
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
  std::ostream& operator<< (std::ostream& o, const MaterializedTupleRef_V2_0_1_2_3_4_5_6& t) {
    return t.dump(o);
  }

  

          // can be just the necessary schema
  class MaterializedTupleRef_V4_0_1 {

    public:
    int64_t _fields[2];
    

    int64_t get(int field) const {
      return _fields[field];
    }
    
    void set(int field, int64_t val) {
      _fields[field] = val;
    }
    
    int numFields() const {
      return 2;
    }
    
    MaterializedTupleRef_V4_0_1 () {
      // no-op
    }

    MaterializedTupleRef_V4_0_1 (std::vector<int64_t> vals) {
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
  std::ostream& operator<< (std::ostream& o, const MaterializedTupleRef_V4_0_1& t) {
    return t.dump(o);
  }

  

          // can be just the necessary schema
  class MaterializedTupleRef_V5_0_1 {

    public:
    int64_t _fields[2];
    

    int64_t get(int field) const {
      return _fields[field];
    }
    
    void set(int field, int64_t val) {
      _fields[field] = val;
    }
    
    int numFields() const {
      return 2;
    }
    
    MaterializedTupleRef_V5_0_1 () {
      // no-op
    }

    MaterializedTupleRef_V5_0_1 (std::vector<int64_t> vals) {
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
  std::ostream& operator<< (std::ostream& o, const MaterializedTupleRef_V5_0_1& t) {
    return t.dump(o);
  }

  

          // can be just the necessary schema
  class MaterializedTupleRef_V6_0_1 {

    public:
    int64_t _fields[2];
    

    int64_t get(int field) const {
      return _fields[field];
    }
    
    void set(int field, int64_t val) {
      _fields[field] = val;
    }
    
    int numFields() const {
      return 2;
    }
    
    MaterializedTupleRef_V6_0_1 () {
      // no-op
    }

    MaterializedTupleRef_V6_0_1 (std::vector<int64_t> vals) {
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
  std::ostream& operator<< (std::ostream& o, const MaterializedTupleRef_V6_0_1& t) {
    return t.dump(o);
  }

  

          // can be just the necessary schema
  class MaterializedTupleRef_V7_0_1 {

    public:
    int64_t _fields[2];
    

    int64_t get(int field) const {
      return _fields[field];
    }
    
    void set(int field, int64_t val) {
      _fields[field] = val;
    }
    
    int numFields() const {
      return 2;
    }
    
    MaterializedTupleRef_V7_0_1 () {
      // no-op
    }

    MaterializedTupleRef_V7_0_1 (std::vector<int64_t> vals) {
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
  std::ostream& operator<< (std::ostream& o, const MaterializedTupleRef_V7_0_1& t) {
    return t.dump(o);
  }

  
Relation<MaterializedTupleRef_V7_0_1> V7;

 GlobalCompletionEvent V8;
 

          // can be just the necessary schema
  class MaterializedTupleRef_V10_0_1 {

    public:
    int64_t _fields[2];
    

    int64_t get(int field) const {
      return _fields[field];
    }
    
    void set(int field, int64_t val) {
      _fields[field] = val;
    }
    
    int numFields() const {
      return 2;
    }
    
    MaterializedTupleRef_V10_0_1 () {
      // no-op
    }

    MaterializedTupleRef_V10_0_1 (std::vector<int64_t> vals) {
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
  std::ostream& operator<< (std::ostream& o, const MaterializedTupleRef_V10_0_1& t) {
    return t.dump(o);
  }

  

          // can be just the necessary schema
  class MaterializedTupleRef_V11_0_1 {

    public:
    int64_t _fields[2];
    

    int64_t get(int field) const {
      return _fields[field];
    }
    
    void set(int field, int64_t val) {
      _fields[field] = val;
    }
    
    int numFields() const {
      return 2;
    }
    
    MaterializedTupleRef_V11_0_1 () {
      // no-op
    }

    MaterializedTupleRef_V11_0_1 (std::vector<int64_t> vals) {
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
  std::ostream& operator<< (std::ostream& o, const MaterializedTupleRef_V11_0_1& t) {
    return t.dump(o);
  }

  

 GlobalCompletionEvent V12;
 

 GlobalCompletionEvent V14;
 

          // can be just the necessary schema
  class MaterializedTupleRef_V15_0_1 {

    public:
    int64_t _fields[2];
    

    int64_t get(int field) const {
      return _fields[field];
    }
    
    void set(int field, int64_t val) {
      _fields[field] = val;
    }
    
    int numFields() const {
      return 2;
    }
    
    MaterializedTupleRef_V15_0_1 () {
      // no-op
    }

    MaterializedTupleRef_V15_0_1 (std::vector<int64_t> vals) {
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
  std::ostream& operator<< (std::ostream& o, const MaterializedTupleRef_V15_0_1& t) {
    return t.dump(o);
  }

  

 GlobalCompletionEvent V16;
 

          // can be just the necessary schema
  class MaterializedTupleRef_V18_0_1_2_3 {

    public:
    int64_t _fields[4];
    

    int64_t get(int field) const {
      return _fields[field];
    }
    
    void set(int field, int64_t val) {
      _fields[field] = val;
    }
    
    int numFields() const {
      return 4;
    }
    
    MaterializedTupleRef_V18_0_1_2_3 () {
      // no-op
    }

    MaterializedTupleRef_V18_0_1_2_3 (std::vector<int64_t> vals) {
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
  std::ostream& operator<< (std::ostream& o, const MaterializedTupleRef_V18_0_1_2_3& t) {
    return t.dump(o);
  }

  

          // can be just the necessary schema
  class MaterializedTupleRef_V20_0_1_2_3_4_5 {

    public:
    int64_t _fields[6];
    

    int64_t get(int field) const {
      return _fields[field];
    }
    
    void set(int field, int64_t val) {
      _fields[field] = val;
    }
    
    int numFields() const {
      return 6;
    }
    
    MaterializedTupleRef_V20_0_1_2_3_4_5 () {
      // no-op
    }

    MaterializedTupleRef_V20_0_1_2_3_4_5 (std::vector<int64_t> vals) {
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
  std::ostream& operator<< (std::ostream& o, const MaterializedTupleRef_V20_0_1_2_3_4_5& t) {
    return t.dump(o);
  }

  

 GlobalCompletionEvent V22;
 

          // can be just the necessary schema
  class MaterializedTupleRef_V23_0_1 {

    public:
    int64_t _fields[2];
    

    int64_t get(int field) const {
      return _fields[field];
    }
    
    void set(int field, int64_t val) {
      _fields[field] = val;
    }
    
    int numFields() const {
      return 2;
    }
    
    MaterializedTupleRef_V23_0_1 () {
      // no-op
    }

    MaterializedTupleRef_V23_0_1 (std::vector<int64_t> vals) {
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
  std::ostream& operator<< (std::ostream& o, const MaterializedTupleRef_V23_0_1& t) {
    return t.dump(o);
  }

  
std::vector<MaterializedTupleRef_V1_0_1> result;
typedef DHT_symmetric<int64_t, int64_t, std_hash> DHT_int64;
 
typedef MatchesDHT<int64_t, MaterializedTupleRef_V4_0_1, std_hash> DHT_MaterializedTupleRef_V4_0_1;
 DHT_MaterializedTupleRef_V4_0_1 hash_000;
 
typedef MatchesDHT<int64_t, MaterializedTupleRef_V10_0_1, std_hash> DHT_MaterializedTupleRef_V10_0_1;
 DHT_MaterializedTupleRef_V10_0_1 hash_001;
 

StringIndex string_index;
void init( ) {
}

template <typename T>
struct RelationObject {
  std::vector<T> * data;
  
  RelationObject() : data(new std::vector<T>()) { }

  static GlobalAddress<RelationObject<T>> create() {
    auto r = symmetric_global_alloc<RelationObject<T>>();
    on_all_cores([=] {
        *(r.localize()) = RelationObject<T>();
        });

    return r;
  }
} GRAPPA_BLOCK_ALIGNED;

uint64_t sizes_count = 0;
uint64_t ranks_count = 0;
uint64_t contribs_count = 0;
uint64_t num_vertices_from_sizes = 0;

void query() {

    double start, end;
    double saved_scan_runtime = 0, saved_init_runtime = 0;
    start = walltime();

     auto group_hash_000 = DHT_int64::create_DHT_symmetric( );
hash_000.init_global_DHT( &hash_000,
 cores()*16*1024 );
hash_001.init_global_DHT( &hash_001,
 cores()*16*1024 );
auto group_hash_001 = DHT_int64::create_DHT_symmetric( );

    end = walltime();
    init_runtime += (end-start);
    saved_init_runtime += (end-start);

    
 Grappa::Metrics::reset();
 auto start_V24 = walltime();
 auto start_0 = walltime();
 
    {
    if (FLAGS_bin) {
    V7 = readTuplesUnordered<MaterializedTupleRef_V7_0_1>( "edges.bin" );
    } else {
    V7.data = readTuples<MaterializedTupleRef_V7_0_1>( "edges", FLAGS_nt);
    V7.numtuples = FLAGS_nt;
    auto l_V7 = V7;
    on_all_cores([=]{ V7 = l_V7; });
    }
    }
    
 auto end_0 = walltime();
 auto runtime_0 = end_0 - start_0;
 VLOG(1) << "pipeline 0: " << runtime_0 << " s";
 
 auto end_V24 = walltime();
 auto runtime_V24 = end_V24 - start_V24;
 saved_scan_runtime += runtime_V24;
 VLOG(1) << "pipeline group V24: " << runtime_V24 << " s";
 
 Grappa::Metrics::reset();
 auto start_V25 = walltime();
 // Compiled subplan for GrappaApply(vid=$0,_COLUMN1_=$1)[GrappaGroupBy($1; SUM($6))[GrappaApply(src=$0,vid=$1,src1=$2,len=$3,src11=$4,rank=$5,_COLUMN6_=($5 / $3))[GrappaSelect(($2 = $4))[GrappaHashJoin(($0 = $4))[GrappaHashJoin(($0 = $2))[MemoryScan[GrappaFileScan(public:adhoc:edges)], GrappaApply(src=$0,len=$1)[GrappaApply(src=$0,_COLUMN1_=$1)[GrappaGroupBy($0; COUNT($1))[MemoryScan[GrappaFileScan(public:adhoc:edges)]]]]], GrappaApply(src=$0,rank=$1)[GrappaProject($0,$1)[GrappaApply(src=$0,_COLUMN1_=1.0)[MemoryScan[GrappaFileScan(public:adhoc:edges)]]]]]]]]]

VLOG(1) << "#edges = " << V7.numtuples;

auto start_1 = walltime();
 
 forall<&V8>( V7.data, V7.numtuples, [=](int64_t i, MaterializedTupleRef_V7_0_1& t_005) {
 MaterializedTupleRef_V6_0_1 t_004;
    t_004.set(0, t_005.get(0));
    t_004.set(1, 1.0);
    MaterializedTupleRef_V5_0_1 t_003;
    t_003.set(0, t_004.get(0));
    t_003.set(1, t_004.get(1));
    MaterializedTupleRef_V4_0_1 t_002;
    t_002.set(0, t_003.get(0));
    t_002.set(1, t_003.get(1));
    
 ranks_count++;
 hash_000.insert_unique<&V8>(t_002.get(0), t_002);
 
 }); // end scan over V7
 
 auto end_1 = walltime();
 auto runtime_1 = end_1 - start_1;
 VLOG(1) << "pipeline 1: " << runtime_1 << " s";

 uint64_t total_ranks_count = reduce<uint64_t,COLL_ADD>( &ranks_count );
 VLOG(1) << "rank *inserts* = " << total_ranks_count;
 
auto start_2 = walltime();
 
 forall<&V12>( V7.data, V7.numtuples, [=](int64_t i, MaterializedTupleRef_V7_0_1& t_005) {
 group_hash_001->update <&V12, int64_t, &Aggregates::COUNT<int64_t,int64_t>,0>( t_005.get(0), t_005.get(1));
 
 }); // end scan over V7
 
 auto end_2 = walltime();
 auto runtime_2 = end_2 - start_2;
 VLOG(1) << "pipeline 2: " << runtime_2 << " s";

 group_hash_001-> forall_entries<&V14> ([=](std::pair<const int64_t,int64_t>& V13) {
    num_vertices_from_sizes++;
 });
 uint64_t total_nvfs_count = reduce<uint64_t,COLL_ADD>( &num_vertices_from_sizes );
 VLOG(1) << "#vert according to sizes hash = " << total_nvfs_count;
 
auto start_3 = walltime();
 group_hash_001-> forall_entries<&V14> ([=](std::pair<const int64_t,int64_t>& V13) {
 MaterializedTupleRef_V15_0_1 t_008( {V13.first, V13.second});
 MaterializedTupleRef_V11_0_1 t_007;
    t_007.set(0, t_008.get(0));
    t_007.set(1, t_008.get(1));
    MaterializedTupleRef_V10_0_1 t_006;
    t_006.set(0, t_007.get(0));
    t_006.set(1, t_007.get(1));
    
 sizes_count++;
 hash_001.insert<&V14>(t_006.get(0), t_006);
 
 });
 
 auto end_3 = walltime();
 auto runtime_3 = end_3 - start_3;
 VLOG(1) << "pipeline 3: " << runtime_3 << " s";
 
 uint64_t total_sizes_count = reduce<uint64_t,COLL_ADD>( &sizes_count );
 VLOG(1) << "#sizes = " << total_sizes_count;
 
 auto joinmat = RelationObject<MaterializedTupleRef_V2_0_1_2_3_4_5_6>::create();
auto start_4 = walltime();
 
 forall<&V16>( V7.data, V7.numtuples, [=](int64_t i, MaterializedTupleRef_V7_0_1& t_005) {
 
 hash_001.lookup_iter<&V16>( t_005.get(0), [=](MaterializedTupleRef_V10_0_1& V17) {
 join_coarse_result_count++;
 MaterializedTupleRef_V18_0_1_2_3 t_009 = combine<MaterializedTupleRef_V18_0_1_2_3, MaterializedTupleRef_V7_0_1, MaterializedTupleRef_V10_0_1> (t_005, V17);
 
 hash_000.lookup_iter<&V16>( t_009.get(0), [=](MaterializedTupleRef_V4_0_1& V19) {
 join_coarse_result_count++;
 MaterializedTupleRef_V20_0_1_2_3_4_5 t_010 = combine<MaterializedTupleRef_V20_0_1_2_3_4_5, MaterializedTupleRef_V18_0_1_2_3, MaterializedTupleRef_V4_0_1> (t_009, V19);
 if (( (t_010.get(2)) == (t_010.get(4)) )) {
      MaterializedTupleRef_V2_0_1_2_3_4_5_6 t_001;
    t_001.set(0, t_010.get(0));
    t_001.set(1, t_010.get(1));
    t_001.set(2, t_010.get(2));
    t_001.set(3, t_010.get(3));
    t_001.set(4, t_010.get(4));
    t_001.set(5, t_010.get(5));
    t_001.set(6, ( (t_010.get(5)) / (t_010.get(3)) ));
   
    joinmat->data->push_back(t_001);
    //VLOG_EVERY_N(1, 20000000) << "local size is " << joinmat->data->size(); 
    contribs_count++;
     
 
    }
    
 });
 
 });
 
 }); // end scan over V7
 
 auto end_4 = walltime();
 auto runtime_4 = end_4 - start_4;
 VLOG(1) << "pipeline 4: " << runtime_4 << " s";
 uint64_t total_contribs_count = reduce<uint64_t,COLL_ADD>( &contribs_count );
 VLOG(1) << "#contribs = " << total_contribs_count;

 auto start_99 = walltime();
 on_all_cores([=] {
     auto& vec = *(joinmat->data);
     VLOG(5) << "vec size = " << vec.size();
     forall_here<async,&jm_sync>(0, vec.size(), [=](int64_t i) {
        auto t_001 = vec[i];
        group_hash_000->update <&jm_sync, int64_t, &Aggregates::SUM<int64_t,int64_t>,0>( t_001.get(1), t_001.get(6));
     });
  });

  jm_sync.wait();
auto end_99 = walltime();
 auto runtime_99 = end_99 - start_99;
 VLOG(1) << "pipeline 99: " << runtime_99 << " s";
       
    

 
auto start_5 = walltime();
 group_hash_000-> forall_entries<&V22> ([=](std::pair<const int64_t,int64_t>& V21) {
 MaterializedTupleRef_V23_0_1 t_011( {V21.first, V21.second});
 MaterializedTupleRef_V1_0_1 t_000;
    t_000.set(0, t_011.get(0));
    t_000.set(1, t_011.get(1));
    result.push_back(t_000);
VLOG(2) << t_000;

 });
 
 auto end_5 = walltime();
 auto runtime_5 = end_5 - start_5;
 VLOG(1) << "pipeline 5: " << runtime_5 << " s";
 
LOG(INFO) << "Evaluating subplan GrappaApply(vid=$0,_COLUMN1_=$1)[GrappaGroupBy($1; SUM($6))[GrappaApply(src=$0,vid=$1,src1=$2,len=$3,src11=$4,rank=$5,_COLUMN6_=($5 / $3))[GrappaSelect(($2 = $4))[GrappaHashJoin(($0 = $4))[GrappaHashJoin(($0 = $2))[MemoryScan[GrappaFileScan(public:adhoc:edges)], GrappaApply(src=$0,len=$1)[GrappaApply(src=$0,_COLUMN1_=$1)[GrappaGroupBy($0; COUNT($1))[MemoryScan[GrappaFileScan(public:adhoc:edges)]]]]], GrappaApply(src=$0,rank=$1)[GrappaProject($0,$1)[GrappaApply(src=$0,_COLUMN1_=1.0)[MemoryScan[GrappaFileScan(public:adhoc:edges)]]]]]]]]]";

 auto end_V25 = walltime();
 auto runtime_V25 = end_V25 - start_V25;
 in_memory_runtime += runtime_V25;
 VLOG(1) << "pipeline group V25: " << runtime_V25 << " s";
 

    // since reset the stats after scan, need to set these again
    scan_runtime = saved_scan_runtime;
    init_runtime = saved_init_runtime;
}


int main(int argc, char** argv) {
    init(&argc, &argv);

    run([] {

    init();
double start = Grappa::walltime();
    	query();
      double end = Grappa::walltime();
      query_runtime = end - start;
      on_all_cores([] { emit_count = result.size(); });
      Metrics::merge_and_print();
    });

    finalize();
    return 0;
}
