// grappa
#include <Grappa.hpp>
#include <Collective.hpp>
#include <GlobalCompletionEvent.hpp>
#include <Metrics.hpp>

using namespace Grappa;

// stl
#include <vector>
#include <iomanip>
#include <cstring>
#include <limits>

// query library
#include "relation_io.hpp"
#include "MatchesDHT.hpp"
#include "DoubleDHT.hpp"
#include "MapReduce.hpp"
//#include "HashJoin.hpp"
#include "DHT_symmetric.hpp"
#include "Aggregates.hpp"
#include "Iterators.hpp"
#include "radish_utils.h"
#include "stats.h"
#include "strings.h"
#include "dates.h"
#include "relation.hpp"

//FIXME: prefer to include this only for Iterator codes
#include "Operators.hpp"

DEFINE_uint64( nt, 30, "hack: number of tuples");
DEFINE_bool( jsonsplits, false, "interpret input file F as F/part-*,"
                             "and containing json records");

template <typename T>
struct counter {
  T count;
  static GlobalAddress<counter<T>> create(T init) {
    auto res = symmetric_global_alloc<counter<T>>();
    on_all_cores([res, init] {
        res->count = init;
        });           
    return res;
  }
} GRAPPA_BLOCK_ALIGNED;

template <typename T>
T get_count(GlobalAddress<counter<T>> p) {
  return p->count;                           
}

          // can be just the necessary schema
  class MaterializedTupleRef_V1_0 {
    // Invariant: data stored in _fields is always in the representation
    // specified by _scheme.

    public:
    
        double f0;
    

    static constexpr int numFields() {
      return 1;
    }

    // size of all fields in struct removing only end padding
    static size_t fieldsSize() {
        const MaterializedTupleRef_V1_0 _t;
        return

        
            sizeof(_t.f0);
         
    }

    // debugging function to get storage sizes
    static void print_representation() {
        const MaterializedTupleRef_V1_0 _t;

        
        std::cout << _t.fieldsSize() << std::endl;
        


    }

    MaterializedTupleRef_V1_0 () {
      // no-op
    }

    //template <typename OT>
    //MaterializedTupleRef_V1_0 (const OT& other) {
    //  std::memcpy(this, &other, sizeof(MaterializedTupleRef_V1_0));
    //}
    MaterializedTupleRef_V1_0 (
                               const double& a0
                               
                       
                       ) {
        
            f0 = a0;
        
    }

    
    


    MaterializedTupleRef_V1_0(const std::tuple<
        
        double
        
        
            >& o) {
        
            f0 = std::get<0>(o);
        
     }

     std::tuple<
        
        double
        
        
     > to_tuple() {

        std::tuple<
        
        double
        
        
        > r;
        
            std::get<0>(r) = f0;
        
        return r;
     }

    // shamelessly terrible disambiguation: one solution is named factory methods
    //MaterializedTupleRef_V1_0 (std::vector<int64_t> vals, bool ignore1, bool ignore2) {
    //    
    //        f0 = vals[0];
    //    
    //}

    // use the tuple schema to interpret the input stream
    static MaterializedTupleRef_V1_0 fromIStream(std::istream& ss, char delim=' ') {
        MaterializedTupleRef_V1_0 _ret;

        
            
               {
               // use operator>> to parse into proper numeric type
               ss >> _ret.f0;
               //throw away the next delimiter
               std::string _temp;
               std::getline(ss, _temp, delim);
               }
            
        

        return _ret;
    }

    void toOStream(std::ostream& os) const {
        os.write((char*)this, this->fieldsSize());
    }

    void toOStreamAscii(std::ostream& os) const {
        os
        
        << f0 << std::endl;
    }

    //template <typename Tuple, typename T>
    //MaterializedTupleRef_V1_0 (const Tuple& v0, const T& from) {
    //    constexpr size_t v0_size = std::tuple_size<Tuple>::value;
    //    constexpr int from_size = T::numFields();
    //    static_assert(MaterializedTupleRef_V1_0::numFields() == (v0_size + from_size), "constructor only works on same number of total fields");
    //    TupleUtils::assign<0, decltype(_scheme)>(_fields, v0);
    //    std::memcpy(((char*)&_fields)+v0_size*sizeof(int64_t), &(from._fields), from_size*sizeof(int64_t));
    //}

    //template <typename Tuple>
    //MaterializedTupleRef_V1_0 (const Tuple& v0) {
    //    static_assert(MaterializedTupleRef_V1_0::numFields() == (std::tuple_size<Tuple>::value), "constructor only works on same number of total fields");
    //    TupleUtils::assign<0, decltype(_scheme)>(_fields, v0);
    //}

    std::ostream& dump(std::ostream& o) const {
      o << "Materialized(";

      
        o << f0 << ",";
      

      o << ")";
      return o;
    }

    
  } GRAPPA_BLOCK_ALIGNED;

  std::ostream& operator<< (std::ostream& o, const MaterializedTupleRef_V1_0& t) {
    return t.dump(o);
  }

          // can be just the necessary schema
  class MaterializedTupleRef_V2_0 {
    // Invariant: data stored in _fields is always in the representation
    // specified by _scheme.

    public:
    
        double f0;
    

    static constexpr int numFields() {
      return 1;
    }

    // size of all fields in struct removing only end padding
    static size_t fieldsSize() {
        const MaterializedTupleRef_V2_0 _t;
        return

        
            sizeof(_t.f0);
         
    }

    // debugging function to get storage sizes
    static void print_representation() {
        const MaterializedTupleRef_V2_0 _t;

        
        std::cout << _t.fieldsSize() << std::endl;
        


    }

    MaterializedTupleRef_V2_0 () {
      // no-op
    }

    //template <typename OT>
    //MaterializedTupleRef_V2_0 (const OT& other) {
    //  std::memcpy(this, &other, sizeof(MaterializedTupleRef_V2_0));
    //}
    MaterializedTupleRef_V2_0 (
                               const double& a0
                               
                       
                       ) {
        
            f0 = a0;
        
    }

    
    


    MaterializedTupleRef_V2_0(const std::tuple<
        
        double
        
        
            >& o) {
        
            f0 = std::get<0>(o);
        
     }

     std::tuple<
        
        double
        
        
     > to_tuple() {

        std::tuple<
        
        double
        
        
        > r;
        
            std::get<0>(r) = f0;
        
        return r;
     }

    // shamelessly terrible disambiguation: one solution is named factory methods
    //MaterializedTupleRef_V2_0 (std::vector<int64_t> vals, bool ignore1, bool ignore2) {
    //    
    //        f0 = vals[0];
    //    
    //}

    // use the tuple schema to interpret the input stream
    static MaterializedTupleRef_V2_0 fromIStream(std::istream& ss, char delim=' ') {
        MaterializedTupleRef_V2_0 _ret;

        
            
               {
               // use operator>> to parse into proper numeric type
               ss >> _ret.f0;
               //throw away the next delimiter
               std::string _temp;
               std::getline(ss, _temp, delim);
               }
            
        

        return _ret;
    }

    void toOStream(std::ostream& os) const {
        os.write((char*)this, this->fieldsSize());
    }

    void toOStreamAscii(std::ostream& os) const {
        os
        
        << f0 << std::endl;
    }

    //template <typename Tuple, typename T>
    //MaterializedTupleRef_V2_0 (const Tuple& v0, const T& from) {
    //    constexpr size_t v0_size = std::tuple_size<Tuple>::value;
    //    constexpr int from_size = T::numFields();
    //    static_assert(MaterializedTupleRef_V2_0::numFields() == (v0_size + from_size), "constructor only works on same number of total fields");
    //    TupleUtils::assign<0, decltype(_scheme)>(_fields, v0);
    //    std::memcpy(((char*)&_fields)+v0_size*sizeof(int64_t), &(from._fields), from_size*sizeof(int64_t));
    //}

    //template <typename Tuple>
    //MaterializedTupleRef_V2_0 (const Tuple& v0) {
    //    static_assert(MaterializedTupleRef_V2_0::numFields() == (std::tuple_size<Tuple>::value), "constructor only works on same number of total fields");
    //    TupleUtils::assign<0, decltype(_scheme)>(_fields, v0);
    //}

    std::ostream& dump(std::ostream& o) const {
      o << "Materialized(";

      
        o << f0 << ",";
      

      o << ")";
      return o;
    }

    
  } GRAPPA_BLOCK_ALIGNED;

  std::ostream& operator<< (std::ostream& o, const MaterializedTupleRef_V2_0& t) {
    return t.dump(o);
  }

          // can be just the necessary schema
  class MaterializedTupleRef_V4_0 {
    // Invariant: data stored in _fields is always in the representation
    // specified by _scheme.

    public:
    
        double f0;
    

    static constexpr int numFields() {
      return 1;
    }

    // size of all fields in struct removing only end padding
    static size_t fieldsSize() {
        const MaterializedTupleRef_V4_0 _t;
        return

        
            sizeof(_t.f0);
         
    }

    // debugging function to get storage sizes
    static void print_representation() {
        const MaterializedTupleRef_V4_0 _t;

        
        std::cout << _t.fieldsSize() << std::endl;
        


    }

    MaterializedTupleRef_V4_0 () {
      // no-op
    }

    //template <typename OT>
    //MaterializedTupleRef_V4_0 (const OT& other) {
    //  std::memcpy(this, &other, sizeof(MaterializedTupleRef_V4_0));
    //}
    MaterializedTupleRef_V4_0 (
                               const double& a0
                               
                       
                       ) {
        
            f0 = a0;
        
    }

    
    


    MaterializedTupleRef_V4_0(const std::tuple<
        
        double
        
        
            >& o) {
        
            f0 = std::get<0>(o);
        
     }

     std::tuple<
        
        double
        
        
     > to_tuple() {

        std::tuple<
        
        double
        
        
        > r;
        
            std::get<0>(r) = f0;
        
        return r;
     }

    // shamelessly terrible disambiguation: one solution is named factory methods
    //MaterializedTupleRef_V4_0 (std::vector<int64_t> vals, bool ignore1, bool ignore2) {
    //    
    //        f0 = vals[0];
    //    
    //}

    // use the tuple schema to interpret the input stream
    static MaterializedTupleRef_V4_0 fromIStream(std::istream& ss, char delim=' ') {
        MaterializedTupleRef_V4_0 _ret;

        
            
               {
               // use operator>> to parse into proper numeric type
               ss >> _ret.f0;
               //throw away the next delimiter
               std::string _temp;
               std::getline(ss, _temp, delim);
               }
            
        

        return _ret;
    }

    void toOStream(std::ostream& os) const {
        os.write((char*)this, this->fieldsSize());
    }

    void toOStreamAscii(std::ostream& os) const {
        os
        
        << f0 << std::endl;
    }

    //template <typename Tuple, typename T>
    //MaterializedTupleRef_V4_0 (const Tuple& v0, const T& from) {
    //    constexpr size_t v0_size = std::tuple_size<Tuple>::value;
    //    constexpr int from_size = T::numFields();
    //    static_assert(MaterializedTupleRef_V4_0::numFields() == (v0_size + from_size), "constructor only works on same number of total fields");
    //    TupleUtils::assign<0, decltype(_scheme)>(_fields, v0);
    //    std::memcpy(((char*)&_fields)+v0_size*sizeof(int64_t), &(from._fields), from_size*sizeof(int64_t));
    //}

    //template <typename Tuple>
    //MaterializedTupleRef_V4_0 (const Tuple& v0) {
    //    static_assert(MaterializedTupleRef_V4_0::numFields() == (std::tuple_size<Tuple>::value), "constructor only works on same number of total fields");
    //    TupleUtils::assign<0, decltype(_scheme)>(_fields, v0);
    //}

    std::ostream& dump(std::ostream& o) const {
      o << "Materialized(";

      
        o << f0 << ",";
      

      o << ")";
      return o;
    }

    
  } GRAPPA_BLOCK_ALIGNED;

  std::ostream& operator<< (std::ostream& o, const MaterializedTupleRef_V4_0& t) {
    return t.dump(o);
  }

          // can be just the necessary schema
  class MaterializedTupleRef_V5_0 {
    // Invariant: data stored in _fields is always in the representation
    // specified by _scheme.

    public:
    
        double f0;
    

    static constexpr int numFields() {
      return 1;
    }

    // size of all fields in struct removing only end padding
    static size_t fieldsSize() {
        const MaterializedTupleRef_V5_0 _t;
        return

        
            sizeof(_t.f0);
         
    }

    // debugging function to get storage sizes
    static void print_representation() {
        const MaterializedTupleRef_V5_0 _t;

        
        std::cout << _t.fieldsSize() << std::endl;
        


    }

    MaterializedTupleRef_V5_0 () {
      // no-op
    }

    //template <typename OT>
    //MaterializedTupleRef_V5_0 (const OT& other) {
    //  std::memcpy(this, &other, sizeof(MaterializedTupleRef_V5_0));
    //}
    MaterializedTupleRef_V5_0 (
                               const double& a0
                               
                       
                       ) {
        
            f0 = a0;
        
    }

    
    


    MaterializedTupleRef_V5_0(const std::tuple<
        
        double
        
        
            >& o) {
        
            f0 = std::get<0>(o);
        
     }

     std::tuple<
        
        double
        
        
     > to_tuple() {

        std::tuple<
        
        double
        
        
        > r;
        
            std::get<0>(r) = f0;
        
        return r;
     }

    // shamelessly terrible disambiguation: one solution is named factory methods
    //MaterializedTupleRef_V5_0 (std::vector<int64_t> vals, bool ignore1, bool ignore2) {
    //    
    //        f0 = vals[0];
    //    
    //}

    // use the tuple schema to interpret the input stream
    static MaterializedTupleRef_V5_0 fromIStream(std::istream& ss, char delim=' ') {
        MaterializedTupleRef_V5_0 _ret;

        
            
               {
               // use operator>> to parse into proper numeric type
               ss >> _ret.f0;
               //throw away the next delimiter
               std::string _temp;
               std::getline(ss, _temp, delim);
               }
            
        

        return _ret;
    }

    void toOStream(std::ostream& os) const {
        os.write((char*)this, this->fieldsSize());
    }

    void toOStreamAscii(std::ostream& os) const {
        os
        
        << f0 << std::endl;
    }

    //template <typename Tuple, typename T>
    //MaterializedTupleRef_V5_0 (const Tuple& v0, const T& from) {
    //    constexpr size_t v0_size = std::tuple_size<Tuple>::value;
    //    constexpr int from_size = T::numFields();
    //    static_assert(MaterializedTupleRef_V5_0::numFields() == (v0_size + from_size), "constructor only works on same number of total fields");
    //    TupleUtils::assign<0, decltype(_scheme)>(_fields, v0);
    //    std::memcpy(((char*)&_fields)+v0_size*sizeof(int64_t), &(from._fields), from_size*sizeof(int64_t));
    //}

    //template <typename Tuple>
    //MaterializedTupleRef_V5_0 (const Tuple& v0) {
    //    static_assert(MaterializedTupleRef_V5_0::numFields() == (std::tuple_size<Tuple>::value), "constructor only works on same number of total fields");
    //    TupleUtils::assign<0, decltype(_scheme)>(_fields, v0);
    //}

    std::ostream& dump(std::ostream& o) const {
      o << "Materialized(";

      
        o << f0 << ",";
      

      o << ")";
      return o;
    }

    
  } GRAPPA_BLOCK_ALIGNED;

  std::ostream& operator<< (std::ostream& o, const MaterializedTupleRef_V5_0& t) {
    return t.dump(o);
  }

          // can be just the necessary schema
  class MaterializedTupleRef_V6_0_1 {
    // Invariant: data stored in _fields is always in the representation
    // specified by _scheme.

    public:
    
        int64_t f0;
    
        int64_t f1;
    

    static constexpr int numFields() {
      return 2;
    }

    // size of all fields in struct removing only end padding
    static size_t fieldsSize() {
        const MaterializedTupleRef_V6_0_1 _t;
        return

        
            ((char*)&_t.f1) + sizeof(_t.f1) - ((char*)&_t);
         
    }

    // debugging function to get storage sizes
    static void print_representation() {
        const MaterializedTupleRef_V6_0_1 _t;

        
        
        std::cout << (((char*)&_t.f1) - ((char*)&_t.f0)) << ",";
        
        std::cout << (_t.fieldsSize() - (((char*)&_t.f0) - ((char*)&_t)));
        std::cout << std::endl;
        


    }

    MaterializedTupleRef_V6_0_1 () {
      // no-op
    }

    //template <typename OT>
    //MaterializedTupleRef_V6_0_1 (const OT& other) {
    //  std::memcpy(this, &other, sizeof(MaterializedTupleRef_V6_0_1));
    //}
    MaterializedTupleRef_V6_0_1 (
                               const int64_t& a0
                               ,
                       
                               const int64_t& a1
                               
                       
                       ) {
        
            f0 = a0;
        
            f1 = a1;
        
    }

    
    


    MaterializedTupleRef_V6_0_1(const std::tuple<
        
        int64_t
        ,
        
        int64_t
        
        
            >& o) {
        
            f0 = std::get<0>(o);
        
            f1 = std::get<1>(o);
        
     }

     std::tuple<
        
        int64_t
        ,
        
        int64_t
        
        
     > to_tuple() {

        std::tuple<
        
        int64_t
        ,
        
        int64_t
        
        
        > r;
        
            std::get<0>(r) = f0;
        
            std::get<1>(r) = f1;
        
        return r;
     }

    // shamelessly terrible disambiguation: one solution is named factory methods
    //MaterializedTupleRef_V6_0_1 (std::vector<int64_t> vals, bool ignore1, bool ignore2) {
    //    
    //        f0 = vals[0];
    //    
    //        f1 = vals[1];
    //    
    //}

    // use the tuple schema to interpret the input stream
    static MaterializedTupleRef_V6_0_1 fromIStream(std::istream& ss, char delim=' ') {
        MaterializedTupleRef_V6_0_1 _ret;

        
            
               {
               // use operator>> to parse into proper numeric type
               ss >> _ret.f0;
               //throw away the next delimiter
               std::string _temp;
               std::getline(ss, _temp, delim);
               }
            
        
            
               {
               // use operator>> to parse into proper numeric type
               ss >> _ret.f1;
               //throw away the next delimiter
               std::string _temp;
               std::getline(ss, _temp, delim);
               }
            
        

        return _ret;
    }

    void toOStream(std::ostream& os) const {
        os.write((char*)this, this->fieldsSize());
    }

    void toOStreamAscii(std::ostream& os) const {
        os
        
        << f0 << " "
        
        << f1 << std::endl;
    }

    //template <typename Tuple, typename T>
    //MaterializedTupleRef_V6_0_1 (const Tuple& v0, const T& from) {
    //    constexpr size_t v0_size = std::tuple_size<Tuple>::value;
    //    constexpr int from_size = T::numFields();
    //    static_assert(MaterializedTupleRef_V6_0_1::numFields() == (v0_size + from_size), "constructor only works on same number of total fields");
    //    TupleUtils::assign<0, decltype(_scheme)>(_fields, v0);
    //    std::memcpy(((char*)&_fields)+v0_size*sizeof(int64_t), &(from._fields), from_size*sizeof(int64_t));
    //}

    //template <typename Tuple>
    //MaterializedTupleRef_V6_0_1 (const Tuple& v0) {
    //    static_assert(MaterializedTupleRef_V6_0_1::numFields() == (std::tuple_size<Tuple>::value), "constructor only works on same number of total fields");
    //    TupleUtils::assign<0, decltype(_scheme)>(_fields, v0);
    //}

    std::ostream& dump(std::ostream& o) const {
      o << "Materialized(";

      
        o << f0 << ",";
      
        o << f1 << ",";
      

      o << ")";
      return o;
    }

    
  } GRAPPA_BLOCK_ALIGNED;

  std::ostream& operator<< (std::ostream& o, const MaterializedTupleRef_V6_0_1& t) {
    return t.dump(o);
  }

          // can be just the necessary schema
  class MaterializedTupleRef_V8_0 {
    // Invariant: data stored in _fields is always in the representation
    // specified by _scheme.

    public:
    
        int64_t f0;
    

    static constexpr int numFields() {
      return 1;
    }

    // size of all fields in struct removing only end padding
    static size_t fieldsSize() {
        const MaterializedTupleRef_V8_0 _t;
        return

        
            sizeof(_t.f0);
         
    }

    // debugging function to get storage sizes
    static void print_representation() {
        const MaterializedTupleRef_V8_0 _t;

        
        std::cout << _t.fieldsSize() << std::endl;
        


    }

    MaterializedTupleRef_V8_0 () {
      // no-op
    }

    //template <typename OT>
    //MaterializedTupleRef_V8_0 (const OT& other) {
    //  std::memcpy(this, &other, sizeof(MaterializedTupleRef_V8_0));
    //}
    MaterializedTupleRef_V8_0 (
                               const int64_t& a0
                               
                       
                       ) {
        
            f0 = a0;
        
    }

    
    


    MaterializedTupleRef_V8_0(const std::tuple<
        
        int64_t
        
        
            >& o) {
        
            f0 = std::get<0>(o);
        
     }

     std::tuple<
        
        int64_t
        
        
     > to_tuple() {

        std::tuple<
        
        int64_t
        
        
        > r;
        
            std::get<0>(r) = f0;
        
        return r;
     }

    // shamelessly terrible disambiguation: one solution is named factory methods
    //MaterializedTupleRef_V8_0 (std::vector<int64_t> vals, bool ignore1, bool ignore2) {
    //    
    //        f0 = vals[0];
    //    
    //}

    // use the tuple schema to interpret the input stream
    static MaterializedTupleRef_V8_0 fromIStream(std::istream& ss, char delim=' ') {
        MaterializedTupleRef_V8_0 _ret;

        
            
               {
               // use operator>> to parse into proper numeric type
               ss >> _ret.f0;
               //throw away the next delimiter
               std::string _temp;
               std::getline(ss, _temp, delim);
               }
            
        

        return _ret;
    }

    void toOStream(std::ostream& os) const {
        os.write((char*)this, this->fieldsSize());
    }

    void toOStreamAscii(std::ostream& os) const {
        os
        
        << f0 << std::endl;
    }

    //template <typename Tuple, typename T>
    //MaterializedTupleRef_V8_0 (const Tuple& v0, const T& from) {
    //    constexpr size_t v0_size = std::tuple_size<Tuple>::value;
    //    constexpr int from_size = T::numFields();
    //    static_assert(MaterializedTupleRef_V8_0::numFields() == (v0_size + from_size), "constructor only works on same number of total fields");
    //    TupleUtils::assign<0, decltype(_scheme)>(_fields, v0);
    //    std::memcpy(((char*)&_fields)+v0_size*sizeof(int64_t), &(from._fields), from_size*sizeof(int64_t));
    //}

    //template <typename Tuple>
    //MaterializedTupleRef_V8_0 (const Tuple& v0) {
    //    static_assert(MaterializedTupleRef_V8_0::numFields() == (std::tuple_size<Tuple>::value), "constructor only works on same number of total fields");
    //    TupleUtils::assign<0, decltype(_scheme)>(_fields, v0);
    //}

    std::ostream& dump(std::ostream& o) const {
      o << "Materialized(";

      
        o << f0 << ",";
      

      o << ")";
      return o;
    }

    
  } GRAPPA_BLOCK_ALIGNED;

  std::ostream& operator<< (std::ostream& o, const MaterializedTupleRef_V8_0& t) {
    return t.dump(o);
  }

          // can be just the necessary schema
  class MaterializedTupleRef_V10_0 {
    // Invariant: data stored in _fields is always in the representation
    // specified by _scheme.

    public:
    
        int64_t f0;
    

    static constexpr int numFields() {
      return 1;
    }

    // size of all fields in struct removing only end padding
    static size_t fieldsSize() {
        const MaterializedTupleRef_V10_0 _t;
        return

        
            sizeof(_t.f0);
         
    }

    // debugging function to get storage sizes
    static void print_representation() {
        const MaterializedTupleRef_V10_0 _t;

        
        std::cout << _t.fieldsSize() << std::endl;
        


    }

    MaterializedTupleRef_V10_0 () {
      // no-op
    }

    //template <typename OT>
    //MaterializedTupleRef_V10_0 (const OT& other) {
    //  std::memcpy(this, &other, sizeof(MaterializedTupleRef_V10_0));
    //}
    MaterializedTupleRef_V10_0 (
                               const int64_t& a0
                               
                       
                       ) {
        
            f0 = a0;
        
    }

    
    


    MaterializedTupleRef_V10_0(const std::tuple<
        
        int64_t
        
        
            >& o) {
        
            f0 = std::get<0>(o);
        
     }

     std::tuple<
        
        int64_t
        
        
     > to_tuple() {

        std::tuple<
        
        int64_t
        
        
        > r;
        
            std::get<0>(r) = f0;
        
        return r;
     }

    // shamelessly terrible disambiguation: one solution is named factory methods
    //MaterializedTupleRef_V10_0 (std::vector<int64_t> vals, bool ignore1, bool ignore2) {
    //    
    //        f0 = vals[0];
    //    
    //}

    // use the tuple schema to interpret the input stream
    static MaterializedTupleRef_V10_0 fromIStream(std::istream& ss, char delim=' ') {
        MaterializedTupleRef_V10_0 _ret;

        
            
               {
               // use operator>> to parse into proper numeric type
               ss >> _ret.f0;
               //throw away the next delimiter
               std::string _temp;
               std::getline(ss, _temp, delim);
               }
            
        

        return _ret;
    }

    void toOStream(std::ostream& os) const {
        os.write((char*)this, this->fieldsSize());
    }

    void toOStreamAscii(std::ostream& os) const {
        os
        
        << f0 << std::endl;
    }

    //template <typename Tuple, typename T>
    //MaterializedTupleRef_V10_0 (const Tuple& v0, const T& from) {
    //    constexpr size_t v0_size = std::tuple_size<Tuple>::value;
    //    constexpr int from_size = T::numFields();
    //    static_assert(MaterializedTupleRef_V10_0::numFields() == (v0_size + from_size), "constructor only works on same number of total fields");
    //    TupleUtils::assign<0, decltype(_scheme)>(_fields, v0);
    //    std::memcpy(((char*)&_fields)+v0_size*sizeof(int64_t), &(from._fields), from_size*sizeof(int64_t));
    //}

    //template <typename Tuple>
    //MaterializedTupleRef_V10_0 (const Tuple& v0) {
    //    static_assert(MaterializedTupleRef_V10_0::numFields() == (std::tuple_size<Tuple>::value), "constructor only works on same number of total fields");
    //    TupleUtils::assign<0, decltype(_scheme)>(_fields, v0);
    //}

    std::ostream& dump(std::ostream& o) const {
      o << "Materialized(";

      
        o << f0 << ",";
      

      o << ")";
      return o;
    }

    
  } GRAPPA_BLOCK_ALIGNED;

  std::ostream& operator<< (std::ostream& o, const MaterializedTupleRef_V10_0& t) {
    return t.dump(o);
  }

          // can be just the necessary schema
  class MaterializedTupleRef_V11_0_1_2_3_4_5_6_7_8 {
    // Invariant: data stored in _fields is always in the representation
    // specified by _scheme.

    public:
    
        int64_t f0;
    
        std::array<char, MAX_STR_LEN> f1;
    
        std::array<char, MAX_STR_LEN> f2;
    
        std::array<char, MAX_STR_LEN> f3;
    
        std::array<char, MAX_STR_LEN> f4;
    
        int64_t f5;
    
        std::array<char, MAX_STR_LEN> f6;
    
        double f7;
    
        std::array<char, MAX_STR_LEN> f8;
    

    static constexpr int numFields() {
      return 9;
    }

    // size of all fields in struct removing only end padding
    static size_t fieldsSize() {
        const MaterializedTupleRef_V11_0_1_2_3_4_5_6_7_8 _t;
        return

        
            ((char*)&_t.f8) + sizeof(_t.f8) - ((char*)&_t);
         
    }

    // debugging function to get storage sizes
    static void print_representation() {
        const MaterializedTupleRef_V11_0_1_2_3_4_5_6_7_8 _t;

        
        
        std::cout << (((char*)&_t.f1) - ((char*)&_t.f0)) << ",";
        
        std::cout << (((char*)&_t.f2) - ((char*)&_t.f1)) << ",";
        
        std::cout << (((char*)&_t.f3) - ((char*)&_t.f2)) << ",";
        
        std::cout << (((char*)&_t.f4) - ((char*)&_t.f3)) << ",";
        
        std::cout << (((char*)&_t.f5) - ((char*)&_t.f4)) << ",";
        
        std::cout << (((char*)&_t.f6) - ((char*)&_t.f5)) << ",";
        
        std::cout << (((char*)&_t.f7) - ((char*)&_t.f6)) << ",";
        
        std::cout << (((char*)&_t.f8) - ((char*)&_t.f7)) << ",";
        
        std::cout << (_t.fieldsSize() - (((char*)&_t.f7) - ((char*)&_t)));
        std::cout << std::endl;
        


    }

    MaterializedTupleRef_V11_0_1_2_3_4_5_6_7_8 () {
      // no-op
    }

    //template <typename OT>
    //MaterializedTupleRef_V11_0_1_2_3_4_5_6_7_8 (const OT& other) {
    //  std::memcpy(this, &other, sizeof(MaterializedTupleRef_V11_0_1_2_3_4_5_6_7_8));
    //}
    MaterializedTupleRef_V11_0_1_2_3_4_5_6_7_8 (
                               const int64_t& a0
                               ,
                       
                               const std::array<char, MAX_STR_LEN>& a1
                               ,
                       
                               const std::array<char, MAX_STR_LEN>& a2
                               ,
                       
                               const std::array<char, MAX_STR_LEN>& a3
                               ,
                       
                               const std::array<char, MAX_STR_LEN>& a4
                               ,
                       
                               const int64_t& a5
                               ,
                       
                               const std::array<char, MAX_STR_LEN>& a6
                               ,
                       
                               const double& a7
                               ,
                       
                               const std::array<char, MAX_STR_LEN>& a8
                               
                       
                       ) {
        
            f0 = a0;
        
            f1 = a1;
        
            f2 = a2;
        
            f3 = a3;
        
            f4 = a4;
        
            f5 = a5;
        
            f6 = a6;
        
            f7 = a7;
        
            f8 = a8;
        
    }

    
    


    MaterializedTupleRef_V11_0_1_2_3_4_5_6_7_8(const std::tuple<
        
        int64_t
        ,
        
        std::array<char, MAX_STR_LEN>
        ,
        
        std::array<char, MAX_STR_LEN>
        ,
        
        std::array<char, MAX_STR_LEN>
        ,
        
        std::array<char, MAX_STR_LEN>
        ,
        
        int64_t
        ,
        
        std::array<char, MAX_STR_LEN>
        ,
        
        double
        ,
        
        std::array<char, MAX_STR_LEN>
        
        
            >& o) {
        
            f0 = std::get<0>(o);
        
            f1 = std::get<1>(o);
        
            f2 = std::get<2>(o);
        
            f3 = std::get<3>(o);
        
            f4 = std::get<4>(o);
        
            f5 = std::get<5>(o);
        
            f6 = std::get<6>(o);
        
            f7 = std::get<7>(o);
        
            f8 = std::get<8>(o);
        
     }

     std::tuple<
        
        int64_t
        ,
        
        std::array<char, MAX_STR_LEN>
        ,
        
        std::array<char, MAX_STR_LEN>
        ,
        
        std::array<char, MAX_STR_LEN>
        ,
        
        std::array<char, MAX_STR_LEN>
        ,
        
        int64_t
        ,
        
        std::array<char, MAX_STR_LEN>
        ,
        
        double
        ,
        
        std::array<char, MAX_STR_LEN>
        
        
     > to_tuple() {

        std::tuple<
        
        int64_t
        ,
        
        std::array<char, MAX_STR_LEN>
        ,
        
        std::array<char, MAX_STR_LEN>
        ,
        
        std::array<char, MAX_STR_LEN>
        ,
        
        std::array<char, MAX_STR_LEN>
        ,
        
        int64_t
        ,
        
        std::array<char, MAX_STR_LEN>
        ,
        
        double
        ,
        
        std::array<char, MAX_STR_LEN>
        
        
        > r;
        
            std::get<0>(r) = f0;
        
            std::get<1>(r) = f1;
        
            std::get<2>(r) = f2;
        
            std::get<3>(r) = f3;
        
            std::get<4>(r) = f4;
        
            std::get<5>(r) = f5;
        
            std::get<6>(r) = f6;
        
            std::get<7>(r) = f7;
        
            std::get<8>(r) = f8;
        
        return r;
     }

    // shamelessly terrible disambiguation: one solution is named factory methods
    //MaterializedTupleRef_V11_0_1_2_3_4_5_6_7_8 (std::vector<int64_t> vals, bool ignore1, bool ignore2) {
    //    
    //        f0 = vals[0];
    //    
    //        f1 = vals[1];
    //    
    //        f2 = vals[2];
    //    
    //        f3 = vals[3];
    //    
    //        f4 = vals[4];
    //    
    //        f5 = vals[5];
    //    
    //        f6 = vals[6];
    //    
    //        f7 = vals[7];
    //    
    //        f8 = vals[8];
    //    
    //}

    // use the tuple schema to interpret the input stream
    static MaterializedTupleRef_V11_0_1_2_3_4_5_6_7_8 fromIStream(std::istream& ss, char delim=' ') {
        MaterializedTupleRef_V11_0_1_2_3_4_5_6_7_8 _ret;

        
            
               {
               // use operator>> to parse into proper numeric type
               ss >> _ret.f0;
               //throw away the next delimiter
               std::string _temp;
               std::getline(ss, _temp, delim);
               }
            
        
            
               {
               std::string _temp;
               std::getline(ss, _temp, delim);
               // beware; truncate= true, so unckecked truncation for stringlen >= MAX_STR_LEN!
               _ret.f1 = to_array<MAX_STR_LEN, std::string, true>(_temp);
               }
            
        
            
               {
               std::string _temp;
               std::getline(ss, _temp, delim);
               // beware; truncate= true, so unckecked truncation for stringlen >= MAX_STR_LEN!
               _ret.f2 = to_array<MAX_STR_LEN, std::string, true>(_temp);
               }
            
        
            
               {
               std::string _temp;
               std::getline(ss, _temp, delim);
               // beware; truncate= true, so unckecked truncation for stringlen >= MAX_STR_LEN!
               _ret.f3 = to_array<MAX_STR_LEN, std::string, true>(_temp);
               }
            
        
            
               {
               std::string _temp;
               std::getline(ss, _temp, delim);
               // beware; truncate= true, so unckecked truncation for stringlen >= MAX_STR_LEN!
               _ret.f4 = to_array<MAX_STR_LEN, std::string, true>(_temp);
               }
            
        
            
               {
               // use operator>> to parse into proper numeric type
               ss >> _ret.f5;
               //throw away the next delimiter
               std::string _temp;
               std::getline(ss, _temp, delim);
               }
            
        
            
               {
               std::string _temp;
               std::getline(ss, _temp, delim);
               // beware; truncate= true, so unckecked truncation for stringlen >= MAX_STR_LEN!
               _ret.f6 = to_array<MAX_STR_LEN, std::string, true>(_temp);
               }
            
        
            
               {
               // use operator>> to parse into proper numeric type
               ss >> _ret.f7;
               //throw away the next delimiter
               std::string _temp;
               std::getline(ss, _temp, delim);
               }
            
        
            
               {
               std::string _temp;
               std::getline(ss, _temp, delim);
               // beware; truncate= true, so unckecked truncation for stringlen >= MAX_STR_LEN!
               _ret.f8 = to_array<MAX_STR_LEN, std::string, true>(_temp);
               }
            
        

        return _ret;
    }

    void toOStream(std::ostream& os) const {
        os.write((char*)this, this->fieldsSize());
    }

    void toOStreamAscii(std::ostream& os) const {
        os
        
        << f0 << " "
        
        << f1 << " "
        
        << f2 << " "
        
        << f3 << " "
        
        << f4 << " "
        
        << f5 << " "
        
        << f6 << " "
        
        << f7 << " "
        
        << f8 << std::endl;
    }

    //template <typename Tuple, typename T>
    //MaterializedTupleRef_V11_0_1_2_3_4_5_6_7_8 (const Tuple& v0, const T& from) {
    //    constexpr size_t v0_size = std::tuple_size<Tuple>::value;
    //    constexpr int from_size = T::numFields();
    //    static_assert(MaterializedTupleRef_V11_0_1_2_3_4_5_6_7_8::numFields() == (v0_size + from_size), "constructor only works on same number of total fields");
    //    TupleUtils::assign<0, decltype(_scheme)>(_fields, v0);
    //    std::memcpy(((char*)&_fields)+v0_size*sizeof(int64_t), &(from._fields), from_size*sizeof(int64_t));
    //}

    //template <typename Tuple>
    //MaterializedTupleRef_V11_0_1_2_3_4_5_6_7_8 (const Tuple& v0) {
    //    static_assert(MaterializedTupleRef_V11_0_1_2_3_4_5_6_7_8::numFields() == (std::tuple_size<Tuple>::value), "constructor only works on same number of total fields");
    //    TupleUtils::assign<0, decltype(_scheme)>(_fields, v0);
    //}

    std::ostream& dump(std::ostream& o) const {
      o << "Materialized(";

      
        o << f0 << ",";
      
        o << f1 << ",";
      
        o << f2 << ",";
      
        o << f3 << ",";
      
        o << f4 << ",";
      
        o << f5 << ",";
      
        o << f6 << ",";
      
        o << f7 << ",";
      
        o << f8 << ",";
      

      o << ")";
      return o;
    }

    
  } GRAPPA_BLOCK_ALIGNED;

  std::ostream& operator<< (std::ostream& o, const MaterializedTupleRef_V11_0_1_2_3_4_5_6_7_8& t) {
    return t.dump(o);
  }

DEFINE_string(input_file_part, "part", "Input file");

Relation<aligned_vector<MaterializedTupleRef_V11_0_1_2_3_4_5_6_7_8>> V11;

std::vector<std::string> schema_V11 = { "p_partkey","p_name","p_mfgr","p_brand","p_type","p_size","p_container","p_retailprice","p_comment", };
GlobalCompletionEvent V12(true);
GRAPPA_DEFINE_METRIC(CallbackMetric<int64_t>, app_1_gce_incomplete, [] {
  return V12.incomplete();
});
          // can be just the necessary schema
  class MaterializedTupleRef_V13_0_1 {
    // Invariant: data stored in _fields is always in the representation
    // specified by _scheme.

    public:
    
        int64_t f0;
    
        int64_t f1;
    

    static constexpr int numFields() {
      return 2;
    }

    // size of all fields in struct removing only end padding
    static size_t fieldsSize() {
        const MaterializedTupleRef_V13_0_1 _t;
        return

        
            ((char*)&_t.f1) + sizeof(_t.f1) - ((char*)&_t);
         
    }

    // debugging function to get storage sizes
    static void print_representation() {
        const MaterializedTupleRef_V13_0_1 _t;

        
        
        std::cout << (((char*)&_t.f1) - ((char*)&_t.f0)) << ",";
        
        std::cout << (_t.fieldsSize() - (((char*)&_t.f0) - ((char*)&_t)));
        std::cout << std::endl;
        


    }

    MaterializedTupleRef_V13_0_1 () {
      // no-op
    }

    //template <typename OT>
    //MaterializedTupleRef_V13_0_1 (const OT& other) {
    //  std::memcpy(this, &other, sizeof(MaterializedTupleRef_V13_0_1));
    //}
    MaterializedTupleRef_V13_0_1 (
                               const int64_t& a0
                               ,
                       
                               const int64_t& a1
                               
                       
                       ) {
        
            f0 = a0;
        
            f1 = a1;
        
    }

    
    


    MaterializedTupleRef_V13_0_1(const std::tuple<
        
        int64_t
        ,
        
        int64_t
        
        
            >& o) {
        
            f0 = std::get<0>(o);
        
            f1 = std::get<1>(o);
        
     }

     std::tuple<
        
        int64_t
        ,
        
        int64_t
        
        
     > to_tuple() {

        std::tuple<
        
        int64_t
        ,
        
        int64_t
        
        
        > r;
        
            std::get<0>(r) = f0;
        
            std::get<1>(r) = f1;
        
        return r;
     }

    // shamelessly terrible disambiguation: one solution is named factory methods
    //MaterializedTupleRef_V13_0_1 (std::vector<int64_t> vals, bool ignore1, bool ignore2) {
    //    
    //        f0 = vals[0];
    //    
    //        f1 = vals[1];
    //    
    //}

    // use the tuple schema to interpret the input stream
    static MaterializedTupleRef_V13_0_1 fromIStream(std::istream& ss, char delim=' ') {
        MaterializedTupleRef_V13_0_1 _ret;

        
            
               {
               // use operator>> to parse into proper numeric type
               ss >> _ret.f0;
               //throw away the next delimiter
               std::string _temp;
               std::getline(ss, _temp, delim);
               }
            
        
            
               {
               // use operator>> to parse into proper numeric type
               ss >> _ret.f1;
               //throw away the next delimiter
               std::string _temp;
               std::getline(ss, _temp, delim);
               }
            
        

        return _ret;
    }

    void toOStream(std::ostream& os) const {
        os.write((char*)this, this->fieldsSize());
    }

    void toOStreamAscii(std::ostream& os) const {
        os
        
        << f0 << " "
        
        << f1 << std::endl;
    }

    //template <typename Tuple, typename T>
    //MaterializedTupleRef_V13_0_1 (const Tuple& v0, const T& from) {
    //    constexpr size_t v0_size = std::tuple_size<Tuple>::value;
    //    constexpr int from_size = T::numFields();
    //    static_assert(MaterializedTupleRef_V13_0_1::numFields() == (v0_size + from_size), "constructor only works on same number of total fields");
    //    TupleUtils::assign<0, decltype(_scheme)>(_fields, v0);
    //    std::memcpy(((char*)&_fields)+v0_size*sizeof(int64_t), &(from._fields), from_size*sizeof(int64_t));
    //}

    //template <typename Tuple>
    //MaterializedTupleRef_V13_0_1 (const Tuple& v0) {
    //    static_assert(MaterializedTupleRef_V13_0_1::numFields() == (std::tuple_size<Tuple>::value), "constructor only works on same number of total fields");
    //    TupleUtils::assign<0, decltype(_scheme)>(_fields, v0);
    //}

    std::ostream& dump(std::ostream& o) const {
      o << "Materialized(";

      
        o << f0 << ",";
      
        o << f1 << ",";
      

      o << ")";
      return o;
    }

    
  } GRAPPA_BLOCK_ALIGNED;

  std::ostream& operator<< (std::ostream& o, const MaterializedTupleRef_V13_0_1& t) {
    return t.dump(o);
  }

          // can be just the necessary schema
  class MaterializedTupleRef_V14_0_1_2_3_4_5_6_7_8_9_10_11_12_13_14_15 {
    // Invariant: data stored in _fields is always in the representation
    // specified by _scheme.

    public:
    
        int64_t f0;
    
        int64_t f1;
    
        int64_t f2;
    
        int64_t f3;
    
        int64_t f4;
    
        double f5;
    
        double f6;
    
        double f7;
    
        std::array<char, MAX_STR_LEN> f8;
    
        std::array<char, MAX_STR_LEN> f9;
    
        std::array<char, MAX_STR_LEN> f10;
    
        std::array<char, MAX_STR_LEN> f11;
    
        std::array<char, MAX_STR_LEN> f12;
    
        std::array<char, MAX_STR_LEN> f13;
    
        std::array<char, MAX_STR_LEN> f14;
    
        std::array<char, MAX_STR_LEN> f15;
    

    static constexpr int numFields() {
      return 16;
    }

    // size of all fields in struct removing only end padding
    static size_t fieldsSize() {
        const MaterializedTupleRef_V14_0_1_2_3_4_5_6_7_8_9_10_11_12_13_14_15 _t;
        return

        
            ((char*)&_t.f15) + sizeof(_t.f15) - ((char*)&_t);
         
    }

    // debugging function to get storage sizes
    static void print_representation() {
        const MaterializedTupleRef_V14_0_1_2_3_4_5_6_7_8_9_10_11_12_13_14_15 _t;

        
        
        std::cout << (((char*)&_t.f1) - ((char*)&_t.f0)) << ",";
        
        std::cout << (((char*)&_t.f2) - ((char*)&_t.f1)) << ",";
        
        std::cout << (((char*)&_t.f3) - ((char*)&_t.f2)) << ",";
        
        std::cout << (((char*)&_t.f4) - ((char*)&_t.f3)) << ",";
        
        std::cout << (((char*)&_t.f5) - ((char*)&_t.f4)) << ",";
        
        std::cout << (((char*)&_t.f6) - ((char*)&_t.f5)) << ",";
        
        std::cout << (((char*)&_t.f7) - ((char*)&_t.f6)) << ",";
        
        std::cout << (((char*)&_t.f8) - ((char*)&_t.f7)) << ",";
        
        std::cout << (((char*)&_t.f9) - ((char*)&_t.f8)) << ",";
        
        std::cout << (((char*)&_t.f10) - ((char*)&_t.f9)) << ",";
        
        std::cout << (((char*)&_t.f11) - ((char*)&_t.f10)) << ",";
        
        std::cout << (((char*)&_t.f12) - ((char*)&_t.f11)) << ",";
        
        std::cout << (((char*)&_t.f13) - ((char*)&_t.f12)) << ",";
        
        std::cout << (((char*)&_t.f14) - ((char*)&_t.f13)) << ",";
        
        std::cout << (((char*)&_t.f15) - ((char*)&_t.f14)) << ",";
        
        std::cout << (_t.fieldsSize() - (((char*)&_t.f14) - ((char*)&_t)));
        std::cout << std::endl;
        


    }

    MaterializedTupleRef_V14_0_1_2_3_4_5_6_7_8_9_10_11_12_13_14_15 () {
      // no-op
    }

    //template <typename OT>
    //MaterializedTupleRef_V14_0_1_2_3_4_5_6_7_8_9_10_11_12_13_14_15 (const OT& other) {
    //  std::memcpy(this, &other, sizeof(MaterializedTupleRef_V14_0_1_2_3_4_5_6_7_8_9_10_11_12_13_14_15));
    //}
    MaterializedTupleRef_V14_0_1_2_3_4_5_6_7_8_9_10_11_12_13_14_15 (
                               const int64_t& a0
                               ,
                       
                               const int64_t& a1
                               ,
                       
                               const int64_t& a2
                               ,
                       
                               const int64_t& a3
                               ,
                       
                               const int64_t& a4
                               ,
                       
                               const double& a5
                               ,
                       
                               const double& a6
                               ,
                       
                               const double& a7
                               ,
                       
                               const std::array<char, MAX_STR_LEN>& a8
                               ,
                       
                               const std::array<char, MAX_STR_LEN>& a9
                               ,
                       
                               const std::array<char, MAX_STR_LEN>& a10
                               ,
                       
                               const std::array<char, MAX_STR_LEN>& a11
                               ,
                       
                               const std::array<char, MAX_STR_LEN>& a12
                               ,
                       
                               const std::array<char, MAX_STR_LEN>& a13
                               ,
                       
                               const std::array<char, MAX_STR_LEN>& a14
                               ,
                       
                               const std::array<char, MAX_STR_LEN>& a15
                               
                       
                       ) {
        
            f0 = a0;
        
            f1 = a1;
        
            f2 = a2;
        
            f3 = a3;
        
            f4 = a4;
        
            f5 = a5;
        
            f6 = a6;
        
            f7 = a7;
        
            f8 = a8;
        
            f9 = a9;
        
            f10 = a10;
        
            f11 = a11;
        
            f12 = a12;
        
            f13 = a13;
        
            f14 = a14;
        
            f15 = a15;
        
    }

    
    


    MaterializedTupleRef_V14_0_1_2_3_4_5_6_7_8_9_10_11_12_13_14_15(const std::tuple<
        
        int64_t
        ,
        
        int64_t
        ,
        
        int64_t
        ,
        
        int64_t
        ,
        
        int64_t
        ,
        
        double
        ,
        
        double
        ,
        
        double
        ,
        
        std::array<char, MAX_STR_LEN>
        ,
        
        std::array<char, MAX_STR_LEN>
        ,
        
        std::array<char, MAX_STR_LEN>
        ,
        
        std::array<char, MAX_STR_LEN>
        ,
        
        std::array<char, MAX_STR_LEN>
        ,
        
        std::array<char, MAX_STR_LEN>
        ,
        
        std::array<char, MAX_STR_LEN>
        ,
        
        std::array<char, MAX_STR_LEN>
        
        
            >& o) {
        
            f0 = std::get<0>(o);
        
            f1 = std::get<1>(o);
        
            f2 = std::get<2>(o);
        
            f3 = std::get<3>(o);
        
            f4 = std::get<4>(o);
        
            f5 = std::get<5>(o);
        
            f6 = std::get<6>(o);
        
            f7 = std::get<7>(o);
        
            f8 = std::get<8>(o);
        
            f9 = std::get<9>(o);
        
            f10 = std::get<10>(o);
        
            f11 = std::get<11>(o);
        
            f12 = std::get<12>(o);
        
            f13 = std::get<13>(o);
        
            f14 = std::get<14>(o);
        
            f15 = std::get<15>(o);
        
     }

     std::tuple<
        
        int64_t
        ,
        
        int64_t
        ,
        
        int64_t
        ,
        
        int64_t
        ,
        
        int64_t
        ,
        
        double
        ,
        
        double
        ,
        
        double
        ,
        
        std::array<char, MAX_STR_LEN>
        ,
        
        std::array<char, MAX_STR_LEN>
        ,
        
        std::array<char, MAX_STR_LEN>
        ,
        
        std::array<char, MAX_STR_LEN>
        ,
        
        std::array<char, MAX_STR_LEN>
        ,
        
        std::array<char, MAX_STR_LEN>
        ,
        
        std::array<char, MAX_STR_LEN>
        ,
        
        std::array<char, MAX_STR_LEN>
        
        
     > to_tuple() {

        std::tuple<
        
        int64_t
        ,
        
        int64_t
        ,
        
        int64_t
        ,
        
        int64_t
        ,
        
        int64_t
        ,
        
        double
        ,
        
        double
        ,
        
        double
        ,
        
        std::array<char, MAX_STR_LEN>
        ,
        
        std::array<char, MAX_STR_LEN>
        ,
        
        std::array<char, MAX_STR_LEN>
        ,
        
        std::array<char, MAX_STR_LEN>
        ,
        
        std::array<char, MAX_STR_LEN>
        ,
        
        std::array<char, MAX_STR_LEN>
        ,
        
        std::array<char, MAX_STR_LEN>
        ,
        
        std::array<char, MAX_STR_LEN>
        
        
        > r;
        
            std::get<0>(r) = f0;
        
            std::get<1>(r) = f1;
        
            std::get<2>(r) = f2;
        
            std::get<3>(r) = f3;
        
            std::get<4>(r) = f4;
        
            std::get<5>(r) = f5;
        
            std::get<6>(r) = f6;
        
            std::get<7>(r) = f7;
        
            std::get<8>(r) = f8;
        
            std::get<9>(r) = f9;
        
            std::get<10>(r) = f10;
        
            std::get<11>(r) = f11;
        
            std::get<12>(r) = f12;
        
            std::get<13>(r) = f13;
        
            std::get<14>(r) = f14;
        
            std::get<15>(r) = f15;
        
        return r;
     }

    // shamelessly terrible disambiguation: one solution is named factory methods
    //MaterializedTupleRef_V14_0_1_2_3_4_5_6_7_8_9_10_11_12_13_14_15 (std::vector<int64_t> vals, bool ignore1, bool ignore2) {
    //    
    //        f0 = vals[0];
    //    
    //        f1 = vals[1];
    //    
    //        f2 = vals[2];
    //    
    //        f3 = vals[3];
    //    
    //        f4 = vals[4];
    //    
    //        f5 = vals[5];
    //    
    //        f6 = vals[6];
    //    
    //        f7 = vals[7];
    //    
    //        f8 = vals[8];
    //    
    //        f9 = vals[9];
    //    
    //        f10 = vals[10];
    //    
    //        f11 = vals[11];
    //    
    //        f12 = vals[12];
    //    
    //        f13 = vals[13];
    //    
    //        f14 = vals[14];
    //    
    //        f15 = vals[15];
    //    
    //}

    // use the tuple schema to interpret the input stream
    static MaterializedTupleRef_V14_0_1_2_3_4_5_6_7_8_9_10_11_12_13_14_15 fromIStream(std::istream& ss, char delim=' ') {
        MaterializedTupleRef_V14_0_1_2_3_4_5_6_7_8_9_10_11_12_13_14_15 _ret;

        
            
               {
               // use operator>> to parse into proper numeric type
               ss >> _ret.f0;
               //throw away the next delimiter
               std::string _temp;
               std::getline(ss, _temp, delim);
               }
            
        
            
               {
               // use operator>> to parse into proper numeric type
               ss >> _ret.f1;
               //throw away the next delimiter
               std::string _temp;
               std::getline(ss, _temp, delim);
               }
            
        
            
               {
               // use operator>> to parse into proper numeric type
               ss >> _ret.f2;
               //throw away the next delimiter
               std::string _temp;
               std::getline(ss, _temp, delim);
               }
            
        
            
               {
               // use operator>> to parse into proper numeric type
               ss >> _ret.f3;
               //throw away the next delimiter
               std::string _temp;
               std::getline(ss, _temp, delim);
               }
            
        
            
               {
               // use operator>> to parse into proper numeric type
               ss >> _ret.f4;
               //throw away the next delimiter
               std::string _temp;
               std::getline(ss, _temp, delim);
               }
            
        
            
               {
               // use operator>> to parse into proper numeric type
               ss >> _ret.f5;
               //throw away the next delimiter
               std::string _temp;
               std::getline(ss, _temp, delim);
               }
            
        
            
               {
               // use operator>> to parse into proper numeric type
               ss >> _ret.f6;
               //throw away the next delimiter
               std::string _temp;
               std::getline(ss, _temp, delim);
               }
            
        
            
               {
               // use operator>> to parse into proper numeric type
               ss >> _ret.f7;
               //throw away the next delimiter
               std::string _temp;
               std::getline(ss, _temp, delim);
               }
            
        
            
               {
               std::string _temp;
               std::getline(ss, _temp, delim);
               // beware; truncate= true, so unckecked truncation for stringlen >= MAX_STR_LEN!
               _ret.f8 = to_array<MAX_STR_LEN, std::string, true>(_temp);
               }
            
        
            
               {
               std::string _temp;
               std::getline(ss, _temp, delim);
               // beware; truncate= true, so unckecked truncation for stringlen >= MAX_STR_LEN!
               _ret.f9 = to_array<MAX_STR_LEN, std::string, true>(_temp);
               }
            
        
            
               {
               std::string _temp;
               std::getline(ss, _temp, delim);
               // beware; truncate= true, so unckecked truncation for stringlen >= MAX_STR_LEN!
               _ret.f10 = to_array<MAX_STR_LEN, std::string, true>(_temp);
               }
            
        
            
               {
               std::string _temp;
               std::getline(ss, _temp, delim);
               // beware; truncate= true, so unckecked truncation for stringlen >= MAX_STR_LEN!
               _ret.f11 = to_array<MAX_STR_LEN, std::string, true>(_temp);
               }
            
        
            
               {
               std::string _temp;
               std::getline(ss, _temp, delim);
               // beware; truncate= true, so unckecked truncation for stringlen >= MAX_STR_LEN!
               _ret.f12 = to_array<MAX_STR_LEN, std::string, true>(_temp);
               }
            
        
            
               {
               std::string _temp;
               std::getline(ss, _temp, delim);
               // beware; truncate= true, so unckecked truncation for stringlen >= MAX_STR_LEN!
               _ret.f13 = to_array<MAX_STR_LEN, std::string, true>(_temp);
               }
            
        
            
               {
               std::string _temp;
               std::getline(ss, _temp, delim);
               // beware; truncate= true, so unckecked truncation for stringlen >= MAX_STR_LEN!
               _ret.f14 = to_array<MAX_STR_LEN, std::string, true>(_temp);
               }
            
        
            
               {
               std::string _temp;
               std::getline(ss, _temp, delim);
               // beware; truncate= true, so unckecked truncation for stringlen >= MAX_STR_LEN!
               _ret.f15 = to_array<MAX_STR_LEN, std::string, true>(_temp);
               }
            
        

        return _ret;
    }

    void toOStream(std::ostream& os) const {
        os.write((char*)this, this->fieldsSize());
    }

    void toOStreamAscii(std::ostream& os) const {
        os
        
        << f0 << " "
        
        << f1 << " "
        
        << f2 << " "
        
        << f3 << " "
        
        << f4 << " "
        
        << f5 << " "
        
        << f6 << " "
        
        << f7 << " "
        
        << f8 << " "
        
        << f9 << " "
        
        << f10 << " "
        
        << f11 << " "
        
        << f12 << " "
        
        << f13 << " "
        
        << f14 << " "
        
        << f15 << std::endl;
    }

    //template <typename Tuple, typename T>
    //MaterializedTupleRef_V14_0_1_2_3_4_5_6_7_8_9_10_11_12_13_14_15 (const Tuple& v0, const T& from) {
    //    constexpr size_t v0_size = std::tuple_size<Tuple>::value;
    //    constexpr int from_size = T::numFields();
    //    static_assert(MaterializedTupleRef_V14_0_1_2_3_4_5_6_7_8_9_10_11_12_13_14_15::numFields() == (v0_size + from_size), "constructor only works on same number of total fields");
    //    TupleUtils::assign<0, decltype(_scheme)>(_fields, v0);
    //    std::memcpy(((char*)&_fields)+v0_size*sizeof(int64_t), &(from._fields), from_size*sizeof(int64_t));
    //}

    //template <typename Tuple>
    //MaterializedTupleRef_V14_0_1_2_3_4_5_6_7_8_9_10_11_12_13_14_15 (const Tuple& v0) {
    //    static_assert(MaterializedTupleRef_V14_0_1_2_3_4_5_6_7_8_9_10_11_12_13_14_15::numFields() == (std::tuple_size<Tuple>::value), "constructor only works on same number of total fields");
    //    TupleUtils::assign<0, decltype(_scheme)>(_fields, v0);
    //}

    std::ostream& dump(std::ostream& o) const {
      o << "Materialized(";

      
        o << f0 << ",";
      
        o << f1 << ",";
      
        o << f2 << ",";
      
        o << f3 << ",";
      
        o << f4 << ",";
      
        o << f5 << ",";
      
        o << f6 << ",";
      
        o << f7 << ",";
      
        o << f8 << ",";
      
        o << f9 << ",";
      
        o << f10 << ",";
      
        o << f11 << ",";
      
        o << f12 << ",";
      
        o << f13 << ",";
      
        o << f14 << ",";
      
        o << f15 << ",";
      

      o << ")";
      return o;
    }

    
  } GRAPPA_BLOCK_ALIGNED;

  std::ostream& operator<< (std::ostream& o, const MaterializedTupleRef_V14_0_1_2_3_4_5_6_7_8_9_10_11_12_13_14_15& t) {
    return t.dump(o);
  }

DEFINE_string(input_file_lineitem, "lineitem", "Input file");

Relation<aligned_vector<MaterializedTupleRef_V14_0_1_2_3_4_5_6_7_8_9_10_11_12_13_14_15>> V14;

std::vector<std::string> schema_V14 = { "l_orderkey","l_partkey","l_suppkey","l_linenumber","l_quantity","l_extendedprice","l_discount","l_tax","l_returnflag","l_linestatus","l_shipdate","l_commitdate","l_receiptdate","l_shipinstruct","l_shipmode","l_comment", };
GlobalCompletionEvent V15(true);
GRAPPA_DEFINE_METRIC(CallbackMetric<int64_t>, app_3_gce_incomplete, [] {
  return V15.incomplete();
});
          // can be just the necessary schema
  class MaterializedTupleRef_V17_0_1_2 {
    // Invariant: data stored in _fields is always in the representation
    // specified by _scheme.

    public:
    
        int64_t f0;
    
        int64_t f1;
    
        int64_t f2;
    

    static constexpr int numFields() {
      return 3;
    }

    // size of all fields in struct removing only end padding
    static size_t fieldsSize() {
        const MaterializedTupleRef_V17_0_1_2 _t;
        return

        
            ((char*)&_t.f2) + sizeof(_t.f2) - ((char*)&_t);
         
    }

    // debugging function to get storage sizes
    static void print_representation() {
        const MaterializedTupleRef_V17_0_1_2 _t;

        
        
        std::cout << (((char*)&_t.f1) - ((char*)&_t.f0)) << ",";
        
        std::cout << (((char*)&_t.f2) - ((char*)&_t.f1)) << ",";
        
        std::cout << (_t.fieldsSize() - (((char*)&_t.f1) - ((char*)&_t)));
        std::cout << std::endl;
        


    }

    MaterializedTupleRef_V17_0_1_2 () {
      // no-op
    }

    //template <typename OT>
    //MaterializedTupleRef_V17_0_1_2 (const OT& other) {
    //  std::memcpy(this, &other, sizeof(MaterializedTupleRef_V17_0_1_2));
    //}
    MaterializedTupleRef_V17_0_1_2 (
                               const int64_t& a0
                               ,
                       
                               const int64_t& a1
                               ,
                       
                               const int64_t& a2
                               
                       
                       ) {
        
            f0 = a0;
        
            f1 = a1;
        
            f2 = a2;
        
    }

    
    


    MaterializedTupleRef_V17_0_1_2(const std::tuple<
        
        int64_t
        ,
        
        int64_t
        ,
        
        int64_t
        
        
            >& o) {
        
            f0 = std::get<0>(o);
        
            f1 = std::get<1>(o);
        
            f2 = std::get<2>(o);
        
     }

     std::tuple<
        
        int64_t
        ,
        
        int64_t
        ,
        
        int64_t
        
        
     > to_tuple() {

        std::tuple<
        
        int64_t
        ,
        
        int64_t
        ,
        
        int64_t
        
        
        > r;
        
            std::get<0>(r) = f0;
        
            std::get<1>(r) = f1;
        
            std::get<2>(r) = f2;
        
        return r;
     }

    // shamelessly terrible disambiguation: one solution is named factory methods
    //MaterializedTupleRef_V17_0_1_2 (std::vector<int64_t> vals, bool ignore1, bool ignore2) {
    //    
    //        f0 = vals[0];
    //    
    //        f1 = vals[1];
    //    
    //        f2 = vals[2];
    //    
    //}

    // use the tuple schema to interpret the input stream
    static MaterializedTupleRef_V17_0_1_2 fromIStream(std::istream& ss, char delim=' ') {
        MaterializedTupleRef_V17_0_1_2 _ret;

        
            
               {
               // use operator>> to parse into proper numeric type
               ss >> _ret.f0;
               //throw away the next delimiter
               std::string _temp;
               std::getline(ss, _temp, delim);
               }
            
        
            
               {
               // use operator>> to parse into proper numeric type
               ss >> _ret.f1;
               //throw away the next delimiter
               std::string _temp;
               std::getline(ss, _temp, delim);
               }
            
        
            
               {
               // use operator>> to parse into proper numeric type
               ss >> _ret.f2;
               //throw away the next delimiter
               std::string _temp;
               std::getline(ss, _temp, delim);
               }
            
        

        return _ret;
    }

    void toOStream(std::ostream& os) const {
        os.write((char*)this, this->fieldsSize());
    }

    void toOStreamAscii(std::ostream& os) const {
        os
        
        << f0 << " "
        
        << f1 << " "
        
        << f2 << std::endl;
    }

    //template <typename Tuple, typename T>
    //MaterializedTupleRef_V17_0_1_2 (const Tuple& v0, const T& from) {
    //    constexpr size_t v0_size = std::tuple_size<Tuple>::value;
    //    constexpr int from_size = T::numFields();
    //    static_assert(MaterializedTupleRef_V17_0_1_2::numFields() == (v0_size + from_size), "constructor only works on same number of total fields");
    //    TupleUtils::assign<0, decltype(_scheme)>(_fields, v0);
    //    std::memcpy(((char*)&_fields)+v0_size*sizeof(int64_t), &(from._fields), from_size*sizeof(int64_t));
    //}

    //template <typename Tuple>
    //MaterializedTupleRef_V17_0_1_2 (const Tuple& v0) {
    //    static_assert(MaterializedTupleRef_V17_0_1_2::numFields() == (std::tuple_size<Tuple>::value), "constructor only works on same number of total fields");
    //    TupleUtils::assign<0, decltype(_scheme)>(_fields, v0);
    //}

    std::ostream& dump(std::ostream& o) const {
      o << "Materialized(";

      
        o << f0 << ",";
      
        o << f1 << ",";
      
        o << f2 << ",";
      

      o << ")";
      return o;
    }

    
  } GRAPPA_BLOCK_ALIGNED;

  std::ostream& operator<< (std::ostream& o, const MaterializedTupleRef_V17_0_1_2& t) {
    return t.dump(o);
  }

static MaterializedTupleRef_V17_0_1_2 create_V18(const MaterializedTupleRef_V13_0_1& t1, const MaterializedTupleRef_V10_0& t2) {
    MaterializedTupleRef_V17_0_1_2 t;
    
        t.f0 = t1.f0;
    
        t.f1 = t1.f1;
    

    
        t.f2 = t2.f0;
    

    return t;
}
MaterializedTupleRef_V6_0_1 __V6_update(const MaterializedTupleRef_V6_0_1& state, const MaterializedTupleRef_V8_0& t_005) {
    
    auto _v0 = Aggregates::SUM<int64_t, int64_t>(state.f0,t_005.f0);
    
    auto _v1 = Aggregates::COUNT<int64_t, int64_t>(state.f1,t_005.f0);
    
    return MaterializedTupleRef_V6_0_1(std::make_tuple(_v0,_v1));
}
MaterializedTupleRef_V6_0_1 __V6_init() {
    
    auto _v0 = Aggregates::Zero<int64_t>();
    
    auto _v1 = Aggregates::Zero<int64_t>();
    

    return MaterializedTupleRef_V6_0_1( std::make_tuple(_v0,_v1) );
}
MaterializedTupleRef_V6_0_1 __V6_combine(const MaterializedTupleRef_V6_0_1& state0, const MaterializedTupleRef_V6_0_1& state1) {
    
    auto _v0 = Aggregates::SUM<int64_t, int64_t>(state0.f0,state1.f0);
    
    auto _v1 = Aggregates::SUM<int64_t, int64_t>(state0.f1,state1.f1);
    
    return MaterializedTupleRef_V6_0_1(std::make_tuple(_v0,_v1));
}
GlobalCompletionEvent V19(true);
GRAPPA_DEFINE_METRIC(CallbackMetric<int64_t>, app_4_gce_incomplete, [] {
  return V19.incomplete();
});
          // can be just the necessary schema
  class MaterializedTupleRef_V20_0_1 {
    // Invariant: data stored in _fields is always in the representation
    // specified by _scheme.

    public:
    
        int64_t f0;
    
        int64_t f1;
    

    static constexpr int numFields() {
      return 2;
    }

    // size of all fields in struct removing only end padding
    static size_t fieldsSize() {
        const MaterializedTupleRef_V20_0_1 _t;
        return

        
            ((char*)&_t.f1) + sizeof(_t.f1) - ((char*)&_t);
         
    }

    // debugging function to get storage sizes
    static void print_representation() {
        const MaterializedTupleRef_V20_0_1 _t;

        
        
        std::cout << (((char*)&_t.f1) - ((char*)&_t.f0)) << ",";
        
        std::cout << (_t.fieldsSize() - (((char*)&_t.f0) - ((char*)&_t)));
        std::cout << std::endl;
        


    }

    MaterializedTupleRef_V20_0_1 () {
      // no-op
    }

    //template <typename OT>
    //MaterializedTupleRef_V20_0_1 (const OT& other) {
    //  std::memcpy(this, &other, sizeof(MaterializedTupleRef_V20_0_1));
    //}
    MaterializedTupleRef_V20_0_1 (
                               const int64_t& a0
                               ,
                       
                               const int64_t& a1
                               
                       
                       ) {
        
            f0 = a0;
        
            f1 = a1;
        
    }

    
    


    MaterializedTupleRef_V20_0_1(const std::tuple<
        
        int64_t
        ,
        
        int64_t
        
        
            >& o) {
        
            f0 = std::get<0>(o);
        
            f1 = std::get<1>(o);
        
     }

     std::tuple<
        
        int64_t
        ,
        
        int64_t
        
        
     > to_tuple() {

        std::tuple<
        
        int64_t
        ,
        
        int64_t
        
        
        > r;
        
            std::get<0>(r) = f0;
        
            std::get<1>(r) = f1;
        
        return r;
     }

    // shamelessly terrible disambiguation: one solution is named factory methods
    //MaterializedTupleRef_V20_0_1 (std::vector<int64_t> vals, bool ignore1, bool ignore2) {
    //    
    //        f0 = vals[0];
    //    
    //        f1 = vals[1];
    //    
    //}

    // use the tuple schema to interpret the input stream
    static MaterializedTupleRef_V20_0_1 fromIStream(std::istream& ss, char delim=' ') {
        MaterializedTupleRef_V20_0_1 _ret;

        
            
               {
               // use operator>> to parse into proper numeric type
               ss >> _ret.f0;
               //throw away the next delimiter
               std::string _temp;
               std::getline(ss, _temp, delim);
               }
            
        
            
               {
               // use operator>> to parse into proper numeric type
               ss >> _ret.f1;
               //throw away the next delimiter
               std::string _temp;
               std::getline(ss, _temp, delim);
               }
            
        

        return _ret;
    }

    void toOStream(std::ostream& os) const {
        os.write((char*)this, this->fieldsSize());
    }

    void toOStreamAscii(std::ostream& os) const {
        os
        
        << f0 << " "
        
        << f1 << std::endl;
    }

    //template <typename Tuple, typename T>
    //MaterializedTupleRef_V20_0_1 (const Tuple& v0, const T& from) {
    //    constexpr size_t v0_size = std::tuple_size<Tuple>::value;
    //    constexpr int from_size = T::numFields();
    //    static_assert(MaterializedTupleRef_V20_0_1::numFields() == (v0_size + from_size), "constructor only works on same number of total fields");
    //    TupleUtils::assign<0, decltype(_scheme)>(_fields, v0);
    //    std::memcpy(((char*)&_fields)+v0_size*sizeof(int64_t), &(from._fields), from_size*sizeof(int64_t));
    //}

    //template <typename Tuple>
    //MaterializedTupleRef_V20_0_1 (const Tuple& v0) {
    //    static_assert(MaterializedTupleRef_V20_0_1::numFields() == (std::tuple_size<Tuple>::value), "constructor only works on same number of total fields");
    //    TupleUtils::assign<0, decltype(_scheme)>(_fields, v0);
    //}

    std::ostream& dump(std::ostream& o) const {
      o << "Materialized(";

      
        o << f0 << ",";
      
        o << f1 << ",";
      

      o << ")";
      return o;
    }

    
  } GRAPPA_BLOCK_ALIGNED;

  std::ostream& operator<< (std::ostream& o, const MaterializedTupleRef_V20_0_1& t) {
    return t.dump(o);
  }

MaterializedTupleRef_V5_0 t_012;
          // can be just the necessary schema
  class MaterializedTupleRef_V21_0_1 {
    // Invariant: data stored in _fields is always in the representation
    // specified by _scheme.

    public:
    
        int64_t f0;
    
        double f1;
    

    static constexpr int numFields() {
      return 2;
    }

    // size of all fields in struct removing only end padding
    static size_t fieldsSize() {
        const MaterializedTupleRef_V21_0_1 _t;
        return

        
            ((char*)&_t.f1) + sizeof(_t.f1) - ((char*)&_t);
         
    }

    // debugging function to get storage sizes
    static void print_representation() {
        const MaterializedTupleRef_V21_0_1 _t;

        
        
        std::cout << (((char*)&_t.f1) - ((char*)&_t.f0)) << ",";
        
        std::cout << (_t.fieldsSize() - (((char*)&_t.f0) - ((char*)&_t)));
        std::cout << std::endl;
        


    }

    MaterializedTupleRef_V21_0_1 () {
      // no-op
    }

    //template <typename OT>
    //MaterializedTupleRef_V21_0_1 (const OT& other) {
    //  std::memcpy(this, &other, sizeof(MaterializedTupleRef_V21_0_1));
    //}
    MaterializedTupleRef_V21_0_1 (
                               const int64_t& a0
                               ,
                       
                               const double& a1
                               
                       
                       ) {
        
            f0 = a0;
        
            f1 = a1;
        
    }

    
    


    MaterializedTupleRef_V21_0_1(const std::tuple<
        
        int64_t
        ,
        
        double
        
        
            >& o) {
        
            f0 = std::get<0>(o);
        
            f1 = std::get<1>(o);
        
     }

     std::tuple<
        
        int64_t
        ,
        
        double
        
        
     > to_tuple() {

        std::tuple<
        
        int64_t
        ,
        
        double
        
        
        > r;
        
            std::get<0>(r) = f0;
        
            std::get<1>(r) = f1;
        
        return r;
     }

    // shamelessly terrible disambiguation: one solution is named factory methods
    //MaterializedTupleRef_V21_0_1 (std::vector<int64_t> vals, bool ignore1, bool ignore2) {
    //    
    //        f0 = vals[0];
    //    
    //        f1 = vals[1];
    //    
    //}

    // use the tuple schema to interpret the input stream
    static MaterializedTupleRef_V21_0_1 fromIStream(std::istream& ss, char delim=' ') {
        MaterializedTupleRef_V21_0_1 _ret;

        
            
               {
               // use operator>> to parse into proper numeric type
               ss >> _ret.f0;
               //throw away the next delimiter
               std::string _temp;
               std::getline(ss, _temp, delim);
               }
            
        
            
               {
               // use operator>> to parse into proper numeric type
               ss >> _ret.f1;
               //throw away the next delimiter
               std::string _temp;
               std::getline(ss, _temp, delim);
               }
            
        

        return _ret;
    }

    void toOStream(std::ostream& os) const {
        os.write((char*)this, this->fieldsSize());
    }

    void toOStreamAscii(std::ostream& os) const {
        os
        
        << f0 << " "
        
        << f1 << std::endl;
    }

    //template <typename Tuple, typename T>
    //MaterializedTupleRef_V21_0_1 (const Tuple& v0, const T& from) {
    //    constexpr size_t v0_size = std::tuple_size<Tuple>::value;
    //    constexpr int from_size = T::numFields();
    //    static_assert(MaterializedTupleRef_V21_0_1::numFields() == (v0_size + from_size), "constructor only works on same number of total fields");
    //    TupleUtils::assign<0, decltype(_scheme)>(_fields, v0);
    //    std::memcpy(((char*)&_fields)+v0_size*sizeof(int64_t), &(from._fields), from_size*sizeof(int64_t));
    //}

    //template <typename Tuple>
    //MaterializedTupleRef_V21_0_1 (const Tuple& v0) {
    //    static_assert(MaterializedTupleRef_V21_0_1::numFields() == (std::tuple_size<Tuple>::value), "constructor only works on same number of total fields");
    //    TupleUtils::assign<0, decltype(_scheme)>(_fields, v0);
    //}

    std::ostream& dump(std::ostream& o) const {
      o << "Materialized(";

      
        o << f0 << ",";
      
        o << f1 << ",";
      

      o << ")";
      return o;
    }

    
  } GRAPPA_BLOCK_ALIGNED;

  std::ostream& operator<< (std::ostream& o, const MaterializedTupleRef_V21_0_1& t) {
    return t.dump(o);
  }

          // can be just the necessary schema
  class MaterializedTupleRef_V22_0_1_2 {
    // Invariant: data stored in _fields is always in the representation
    // specified by _scheme.

    public:
    
        int64_t f0;
    
        int64_t f1;
    
        double f2;
    

    static constexpr int numFields() {
      return 3;
    }

    // size of all fields in struct removing only end padding
    static size_t fieldsSize() {
        const MaterializedTupleRef_V22_0_1_2 _t;
        return

        
            ((char*)&_t.f2) + sizeof(_t.f2) - ((char*)&_t);
         
    }

    // debugging function to get storage sizes
    static void print_representation() {
        const MaterializedTupleRef_V22_0_1_2 _t;

        
        
        std::cout << (((char*)&_t.f1) - ((char*)&_t.f0)) << ",";
        
        std::cout << (((char*)&_t.f2) - ((char*)&_t.f1)) << ",";
        
        std::cout << (_t.fieldsSize() - (((char*)&_t.f1) - ((char*)&_t)));
        std::cout << std::endl;
        


    }

    MaterializedTupleRef_V22_0_1_2 () {
      // no-op
    }

    //template <typename OT>
    //MaterializedTupleRef_V22_0_1_2 (const OT& other) {
    //  std::memcpy(this, &other, sizeof(MaterializedTupleRef_V22_0_1_2));
    //}
    MaterializedTupleRef_V22_0_1_2 (
                               const int64_t& a0
                               ,
                       
                               const int64_t& a1
                               ,
                       
                               const double& a2
                               
                       
                       ) {
        
            f0 = a0;
        
            f1 = a1;
        
            f2 = a2;
        
    }

    
    


    MaterializedTupleRef_V22_0_1_2(const std::tuple<
        
        int64_t
        ,
        
        int64_t
        ,
        
        double
        
        
            >& o) {
        
            f0 = std::get<0>(o);
        
            f1 = std::get<1>(o);
        
            f2 = std::get<2>(o);
        
     }

     std::tuple<
        
        int64_t
        ,
        
        int64_t
        ,
        
        double
        
        
     > to_tuple() {

        std::tuple<
        
        int64_t
        ,
        
        int64_t
        ,
        
        double
        
        
        > r;
        
            std::get<0>(r) = f0;
        
            std::get<1>(r) = f1;
        
            std::get<2>(r) = f2;
        
        return r;
     }

    // shamelessly terrible disambiguation: one solution is named factory methods
    //MaterializedTupleRef_V22_0_1_2 (std::vector<int64_t> vals, bool ignore1, bool ignore2) {
    //    
    //        f0 = vals[0];
    //    
    //        f1 = vals[1];
    //    
    //        f2 = vals[2];
    //    
    //}

    // use the tuple schema to interpret the input stream
    static MaterializedTupleRef_V22_0_1_2 fromIStream(std::istream& ss, char delim=' ') {
        MaterializedTupleRef_V22_0_1_2 _ret;

        
            
               {
               // use operator>> to parse into proper numeric type
               ss >> _ret.f0;
               //throw away the next delimiter
               std::string _temp;
               std::getline(ss, _temp, delim);
               }
            
        
            
               {
               // use operator>> to parse into proper numeric type
               ss >> _ret.f1;
               //throw away the next delimiter
               std::string _temp;
               std::getline(ss, _temp, delim);
               }
            
        
            
               {
               // use operator>> to parse into proper numeric type
               ss >> _ret.f2;
               //throw away the next delimiter
               std::string _temp;
               std::getline(ss, _temp, delim);
               }
            
        

        return _ret;
    }

    void toOStream(std::ostream& os) const {
        os.write((char*)this, this->fieldsSize());
    }

    void toOStreamAscii(std::ostream& os) const {
        os
        
        << f0 << " "
        
        << f1 << " "
        
        << f2 << std::endl;
    }

    //template <typename Tuple, typename T>
    //MaterializedTupleRef_V22_0_1_2 (const Tuple& v0, const T& from) {
    //    constexpr size_t v0_size = std::tuple_size<Tuple>::value;
    //    constexpr int from_size = T::numFields();
    //    static_assert(MaterializedTupleRef_V22_0_1_2::numFields() == (v0_size + from_size), "constructor only works on same number of total fields");
    //    TupleUtils::assign<0, decltype(_scheme)>(_fields, v0);
    //    std::memcpy(((char*)&_fields)+v0_size*sizeof(int64_t), &(from._fields), from_size*sizeof(int64_t));
    //}

    //template <typename Tuple>
    //MaterializedTupleRef_V22_0_1_2 (const Tuple& v0) {
    //    static_assert(MaterializedTupleRef_V22_0_1_2::numFields() == (std::tuple_size<Tuple>::value), "constructor only works on same number of total fields");
    //    TupleUtils::assign<0, decltype(_scheme)>(_fields, v0);
    //}

    std::ostream& dump(std::ostream& o) const {
      o << "Materialized(";

      
        o << f0 << ",";
      
        o << f1 << ",";
      
        o << f2 << ",";
      

      o << ")";
      return o;
    }

    
  } GRAPPA_BLOCK_ALIGNED;

  std::ostream& operator<< (std::ostream& o, const MaterializedTupleRef_V22_0_1_2& t) {
    return t.dump(o);
  }

GlobalCompletionEvent V23(true);
GRAPPA_DEFINE_METRIC(CallbackMetric<int64_t>, app_5_gce_incomplete, [] {
  return V23.incomplete();
});
          // can be just the necessary schema
  class MaterializedTupleRef_V25_0_1_2_3 {
    // Invariant: data stored in _fields is always in the representation
    // specified by _scheme.

    public:
    
        int64_t f0;
    
        int64_t f1;
    
        double f2;
    
        int64_t f3;
    

    static constexpr int numFields() {
      return 4;
    }

    // size of all fields in struct removing only end padding
    static size_t fieldsSize() {
        const MaterializedTupleRef_V25_0_1_2_3 _t;
        return

        
            ((char*)&_t.f3) + sizeof(_t.f3) - ((char*)&_t);
         
    }

    // debugging function to get storage sizes
    static void print_representation() {
        const MaterializedTupleRef_V25_0_1_2_3 _t;

        
        
        std::cout << (((char*)&_t.f1) - ((char*)&_t.f0)) << ",";
        
        std::cout << (((char*)&_t.f2) - ((char*)&_t.f1)) << ",";
        
        std::cout << (((char*)&_t.f3) - ((char*)&_t.f2)) << ",";
        
        std::cout << (_t.fieldsSize() - (((char*)&_t.f2) - ((char*)&_t)));
        std::cout << std::endl;
        


    }

    MaterializedTupleRef_V25_0_1_2_3 () {
      // no-op
    }

    //template <typename OT>
    //MaterializedTupleRef_V25_0_1_2_3 (const OT& other) {
    //  std::memcpy(this, &other, sizeof(MaterializedTupleRef_V25_0_1_2_3));
    //}
    MaterializedTupleRef_V25_0_1_2_3 (
                               const int64_t& a0
                               ,
                       
                               const int64_t& a1
                               ,
                       
                               const double& a2
                               ,
                       
                               const int64_t& a3
                               
                       
                       ) {
        
            f0 = a0;
        
            f1 = a1;
        
            f2 = a2;
        
            f3 = a3;
        
    }

    
    


    MaterializedTupleRef_V25_0_1_2_3(const std::tuple<
        
        int64_t
        ,
        
        int64_t
        ,
        
        double
        ,
        
        int64_t
        
        
            >& o) {
        
            f0 = std::get<0>(o);
        
            f1 = std::get<1>(o);
        
            f2 = std::get<2>(o);
        
            f3 = std::get<3>(o);
        
     }

     std::tuple<
        
        int64_t
        ,
        
        int64_t
        ,
        
        double
        ,
        
        int64_t
        
        
     > to_tuple() {

        std::tuple<
        
        int64_t
        ,
        
        int64_t
        ,
        
        double
        ,
        
        int64_t
        
        
        > r;
        
            std::get<0>(r) = f0;
        
            std::get<1>(r) = f1;
        
            std::get<2>(r) = f2;
        
            std::get<3>(r) = f3;
        
        return r;
     }

    // shamelessly terrible disambiguation: one solution is named factory methods
    //MaterializedTupleRef_V25_0_1_2_3 (std::vector<int64_t> vals, bool ignore1, bool ignore2) {
    //    
    //        f0 = vals[0];
    //    
    //        f1 = vals[1];
    //    
    //        f2 = vals[2];
    //    
    //        f3 = vals[3];
    //    
    //}

    // use the tuple schema to interpret the input stream
    static MaterializedTupleRef_V25_0_1_2_3 fromIStream(std::istream& ss, char delim=' ') {
        MaterializedTupleRef_V25_0_1_2_3 _ret;

        
            
               {
               // use operator>> to parse into proper numeric type
               ss >> _ret.f0;
               //throw away the next delimiter
               std::string _temp;
               std::getline(ss, _temp, delim);
               }
            
        
            
               {
               // use operator>> to parse into proper numeric type
               ss >> _ret.f1;
               //throw away the next delimiter
               std::string _temp;
               std::getline(ss, _temp, delim);
               }
            
        
            
               {
               // use operator>> to parse into proper numeric type
               ss >> _ret.f2;
               //throw away the next delimiter
               std::string _temp;
               std::getline(ss, _temp, delim);
               }
            
        
            
               {
               // use operator>> to parse into proper numeric type
               ss >> _ret.f3;
               //throw away the next delimiter
               std::string _temp;
               std::getline(ss, _temp, delim);
               }
            
        

        return _ret;
    }

    void toOStream(std::ostream& os) const {
        os.write((char*)this, this->fieldsSize());
    }

    void toOStreamAscii(std::ostream& os) const {
        os
        
        << f0 << " "
        
        << f1 << " "
        
        << f2 << " "
        
        << f3 << std::endl;
    }

    //template <typename Tuple, typename T>
    //MaterializedTupleRef_V25_0_1_2_3 (const Tuple& v0, const T& from) {
    //    constexpr size_t v0_size = std::tuple_size<Tuple>::value;
    //    constexpr int from_size = T::numFields();
    //    static_assert(MaterializedTupleRef_V25_0_1_2_3::numFields() == (v0_size + from_size), "constructor only works on same number of total fields");
    //    TupleUtils::assign<0, decltype(_scheme)>(_fields, v0);
    //    std::memcpy(((char*)&_fields)+v0_size*sizeof(int64_t), &(from._fields), from_size*sizeof(int64_t));
    //}

    //template <typename Tuple>
    //MaterializedTupleRef_V25_0_1_2_3 (const Tuple& v0) {
    //    static_assert(MaterializedTupleRef_V25_0_1_2_3::numFields() == (std::tuple_size<Tuple>::value), "constructor only works on same number of total fields");
    //    TupleUtils::assign<0, decltype(_scheme)>(_fields, v0);
    //}

    std::ostream& dump(std::ostream& o) const {
      o << "Materialized(";

      
        o << f0 << ",";
      
        o << f1 << ",";
      
        o << f2 << ",";
      
        o << f3 << ",";
      

      o << ")";
      return o;
    }

    
  } GRAPPA_BLOCK_ALIGNED;

  std::ostream& operator<< (std::ostream& o, const MaterializedTupleRef_V25_0_1_2_3& t) {
    return t.dump(o);
  }

static MaterializedTupleRef_V25_0_1_2_3 create_V26(const MaterializedTupleRef_V22_0_1_2& t1, const MaterializedTupleRef_V10_0& t2) {
    MaterializedTupleRef_V25_0_1_2_3 t;
    
        t.f0 = t1.f0;
    
        t.f1 = t1.f1;
    
        t.f2 = t1.f2;
    

    
        t.f3 = t2.f0;
    

    return t;
}
          // can be just the necessary schema
  class MaterializedTupleRef_V27_0_1_2 {
    // Invariant: data stored in _fields is always in the representation
    // specified by _scheme.

    public:
    
        int64_t f0;
    
        double f1;
    
        double f2;
    

    static constexpr int numFields() {
      return 3;
    }

    // size of all fields in struct removing only end padding
    static size_t fieldsSize() {
        const MaterializedTupleRef_V27_0_1_2 _t;
        return

        
            ((char*)&_t.f2) + sizeof(_t.f2) - ((char*)&_t);
         
    }

    // debugging function to get storage sizes
    static void print_representation() {
        const MaterializedTupleRef_V27_0_1_2 _t;

        
        
        std::cout << (((char*)&_t.f1) - ((char*)&_t.f0)) << ",";
        
        std::cout << (((char*)&_t.f2) - ((char*)&_t.f1)) << ",";
        
        std::cout << (_t.fieldsSize() - (((char*)&_t.f1) - ((char*)&_t)));
        std::cout << std::endl;
        


    }

    MaterializedTupleRef_V27_0_1_2 () {
      // no-op
    }

    //template <typename OT>
    //MaterializedTupleRef_V27_0_1_2 (const OT& other) {
    //  std::memcpy(this, &other, sizeof(MaterializedTupleRef_V27_0_1_2));
    //}
    MaterializedTupleRef_V27_0_1_2 (
                               const int64_t& a0
                               ,
                       
                               const double& a1
                               ,
                       
                               const double& a2
                               
                       
                       ) {
        
            f0 = a0;
        
            f1 = a1;
        
            f2 = a2;
        
    }

    
    


    MaterializedTupleRef_V27_0_1_2(const std::tuple<
        
        int64_t
        ,
        
        double
        ,
        
        double
        
        
            >& o) {
        
            f0 = std::get<0>(o);
        
            f1 = std::get<1>(o);
        
            f2 = std::get<2>(o);
        
     }

     std::tuple<
        
        int64_t
        ,
        
        double
        ,
        
        double
        
        
     > to_tuple() {

        std::tuple<
        
        int64_t
        ,
        
        double
        ,
        
        double
        
        
        > r;
        
            std::get<0>(r) = f0;
        
            std::get<1>(r) = f1;
        
            std::get<2>(r) = f2;
        
        return r;
     }

    // shamelessly terrible disambiguation: one solution is named factory methods
    //MaterializedTupleRef_V27_0_1_2 (std::vector<int64_t> vals, bool ignore1, bool ignore2) {
    //    
    //        f0 = vals[0];
    //    
    //        f1 = vals[1];
    //    
    //        f2 = vals[2];
    //    
    //}

    // use the tuple schema to interpret the input stream
    static MaterializedTupleRef_V27_0_1_2 fromIStream(std::istream& ss, char delim=' ') {
        MaterializedTupleRef_V27_0_1_2 _ret;

        
            
               {
               // use operator>> to parse into proper numeric type
               ss >> _ret.f0;
               //throw away the next delimiter
               std::string _temp;
               std::getline(ss, _temp, delim);
               }
            
        
            
               {
               // use operator>> to parse into proper numeric type
               ss >> _ret.f1;
               //throw away the next delimiter
               std::string _temp;
               std::getline(ss, _temp, delim);
               }
            
        
            
               {
               // use operator>> to parse into proper numeric type
               ss >> _ret.f2;
               //throw away the next delimiter
               std::string _temp;
               std::getline(ss, _temp, delim);
               }
            
        

        return _ret;
    }

    void toOStream(std::ostream& os) const {
        os.write((char*)this, this->fieldsSize());
    }

    void toOStreamAscii(std::ostream& os) const {
        os
        
        << f0 << " "
        
        << f1 << " "
        
        << f2 << std::endl;
    }

    //template <typename Tuple, typename T>
    //MaterializedTupleRef_V27_0_1_2 (const Tuple& v0, const T& from) {
    //    constexpr size_t v0_size = std::tuple_size<Tuple>::value;
    //    constexpr int from_size = T::numFields();
    //    static_assert(MaterializedTupleRef_V27_0_1_2::numFields() == (v0_size + from_size), "constructor only works on same number of total fields");
    //    TupleUtils::assign<0, decltype(_scheme)>(_fields, v0);
    //    std::memcpy(((char*)&_fields)+v0_size*sizeof(int64_t), &(from._fields), from_size*sizeof(int64_t));
    //}

    //template <typename Tuple>
    //MaterializedTupleRef_V27_0_1_2 (const Tuple& v0) {
    //    static_assert(MaterializedTupleRef_V27_0_1_2::numFields() == (std::tuple_size<Tuple>::value), "constructor only works on same number of total fields");
    //    TupleUtils::assign<0, decltype(_scheme)>(_fields, v0);
    //}

    std::ostream& dump(std::ostream& o) const {
      o << "Materialized(";

      
        o << f0 << ",";
      
        o << f1 << ",";
      
        o << f2 << ",";
      

      o << ")";
      return o;
    }

    
  } GRAPPA_BLOCK_ALIGNED;

  std::ostream& operator<< (std::ostream& o, const MaterializedTupleRef_V27_0_1_2& t) {
    return t.dump(o);
  }

static MaterializedTupleRef_V27_0_1_2 create_V28(const MaterializedTupleRef_V21_0_1& t1, const MaterializedTupleRef_V5_0& t2) {
    MaterializedTupleRef_V27_0_1_2 t;
    
        t.f0 = t1.f0;
    
        t.f1 = t1.f1;
    

    
        t.f2 = t2.f0;
    

    return t;
}
MaterializedTupleRef_V2_0 __V2_update(const MaterializedTupleRef_V2_0& state, const MaterializedTupleRef_V4_0& t_002) {
    
    auto _v0 = Aggregates::SUM<double, double>(state.f0,t_002.f0);
    
    return MaterializedTupleRef_V2_0(std::make_tuple(_v0));
}
MaterializedTupleRef_V2_0 __V2_init() {
    
    auto _v0 = Aggregates::Zero<double>();
    

    return MaterializedTupleRef_V2_0( std::make_tuple(_v0) );
}
MaterializedTupleRef_V2_0 __V2_combine(const MaterializedTupleRef_V2_0& state0, const MaterializedTupleRef_V2_0& state1) {
    
    auto _v0 = Aggregates::SUM<double, double>(state0.f0,state1.f0);
    
    return MaterializedTupleRef_V2_0(std::make_tuple(_v0));
}
GlobalCompletionEvent V29(true);
GRAPPA_DEFINE_METRIC(CallbackMetric<int64_t>, app_6_gce_incomplete, [] {
  return V29.incomplete();
});
          // can be just the necessary schema
  class MaterializedTupleRef_V30_0 {
    // Invariant: data stored in _fields is always in the representation
    // specified by _scheme.

    public:
    
        double f0;
    

    static constexpr int numFields() {
      return 1;
    }

    // size of all fields in struct removing only end padding
    static size_t fieldsSize() {
        const MaterializedTupleRef_V30_0 _t;
        return

        
            sizeof(_t.f0);
         
    }

    // debugging function to get storage sizes
    static void print_representation() {
        const MaterializedTupleRef_V30_0 _t;

        
        std::cout << _t.fieldsSize() << std::endl;
        


    }

    MaterializedTupleRef_V30_0 () {
      // no-op
    }

    //template <typename OT>
    //MaterializedTupleRef_V30_0 (const OT& other) {
    //  std::memcpy(this, &other, sizeof(MaterializedTupleRef_V30_0));
    //}
    MaterializedTupleRef_V30_0 (
                               const double& a0
                               
                       
                       ) {
        
            f0 = a0;
        
    }

    
    


    MaterializedTupleRef_V30_0(const std::tuple<
        
        double
        
        
            >& o) {
        
            f0 = std::get<0>(o);
        
     }

     std::tuple<
        
        double
        
        
     > to_tuple() {

        std::tuple<
        
        double
        
        
        > r;
        
            std::get<0>(r) = f0;
        
        return r;
     }

    // shamelessly terrible disambiguation: one solution is named factory methods
    //MaterializedTupleRef_V30_0 (std::vector<int64_t> vals, bool ignore1, bool ignore2) {
    //    
    //        f0 = vals[0];
    //    
    //}

    // use the tuple schema to interpret the input stream
    static MaterializedTupleRef_V30_0 fromIStream(std::istream& ss, char delim=' ') {
        MaterializedTupleRef_V30_0 _ret;

        
            
               {
               // use operator>> to parse into proper numeric type
               ss >> _ret.f0;
               //throw away the next delimiter
               std::string _temp;
               std::getline(ss, _temp, delim);
               }
            
        

        return _ret;
    }

    void toOStream(std::ostream& os) const {
        os.write((char*)this, this->fieldsSize());
    }

    void toOStreamAscii(std::ostream& os) const {
        os
        
        << f0 << std::endl;
    }

    //template <typename Tuple, typename T>
    //MaterializedTupleRef_V30_0 (const Tuple& v0, const T& from) {
    //    constexpr size_t v0_size = std::tuple_size<Tuple>::value;
    //    constexpr int from_size = T::numFields();
    //    static_assert(MaterializedTupleRef_V30_0::numFields() == (v0_size + from_size), "constructor only works on same number of total fields");
    //    TupleUtils::assign<0, decltype(_scheme)>(_fields, v0);
    //    std::memcpy(((char*)&_fields)+v0_size*sizeof(int64_t), &(from._fields), from_size*sizeof(int64_t));
    //}

    //template <typename Tuple>
    //MaterializedTupleRef_V30_0 (const Tuple& v0) {
    //    static_assert(MaterializedTupleRef_V30_0::numFields() == (std::tuple_size<Tuple>::value), "constructor only works on same number of total fields");
    //    TupleUtils::assign<0, decltype(_scheme)>(_fields, v0);
    //}

    std::ostream& dump(std::ostream& o) const {
      o << "Materialized(";

      
        o << f0 << ",";
      

      o << ")";
      return o;
    }

    
  } GRAPPA_BLOCK_ALIGNED;

  std::ostream& operator<< (std::ostream& o, const MaterializedTupleRef_V30_0& t) {
    return t.dump(o);
  }

std::vector<MaterializedTupleRef_V1_0> result;
typedef MatchesDHT<std::tuple<int64_t>, MaterializedTupleRef_V10_0, hash_tuple::hash<std::tuple<int64_t>>> DHT_MaterializedTupleRef_V10_0;
DHT_MaterializedTupleRef_V10_0 hash_000;
decltype(hash_000.get_base()) hash_000_g;

StringIndex string_index;
void init( ) {
}

void query() {
    double start, end;
    double saved_scan_runtime = 0, saved_init_runtime = 0;
    start = walltime();

     auto GrappaGroupBy_hash_000 = symmetric_global_alloc<MaterializedTupleRef_V2_0>();
auto GrappaGroupBy_hash_001 = symmetric_global_alloc<MaterializedTupleRef_V6_0_1>();
hash_000.init_global_DHT( &hash_000, cores()*16*1024 );
Grappa::on_all_cores([=] {
  hash_000_g = hash_000.get_base();
});

    end = walltime();
    init_runtime += (end-start);
    saved_init_runtime += (end-start);

    Grappa::Metrics::reset();
Grappa::Metrics::reset();

auto start_V31 = walltime();

        
        auto start_0 = walltime();
VLOG(1)  << "timestamp 0 start " << std::setprecision(15) << start_0;

if (FLAGS_bin) {
BinaryRelationFileReader<MaterializedTupleRef_V11_0_1_2_3_4_5_6_7_8,
                           aligned_vector<MaterializedTupleRef_V11_0_1_2_3_4_5_6_7_8>,
                           SymmetricArrayRepresentation<MaterializedTupleRef_V11_0_1_2_3_4_5_6_7_8>> reader;
                           // just always broadcast the name to all cores
                           // although for some queries it is unnecessary
                           auto l_V11 = reader.read( FLAGS_input_file_part + ".bin" );
                           on_all_cores([=] {
                                V11 = l_V11;
                           });

                           } else {

                           CHECK(false) << "only --bin=true supported for symmetric array repr";

                           }
auto end_0 = walltime();
auto runtime_0 = end_0 - start_0;
VLOG(1)  << "pipeline 0: " << runtime_0 << " s";

VLOG(1)  << "timestamp 0 end " << std::setprecision(15) << end_0;

        

        
        auto start_2 = walltime();
VLOG(1)  << "timestamp 2 start " << std::setprecision(15) << start_2;

if (FLAGS_bin) {
BinaryRelationFileReader<MaterializedTupleRef_V14_0_1_2_3_4_5_6_7_8_9_10_11_12_13_14_15,
                           aligned_vector<MaterializedTupleRef_V14_0_1_2_3_4_5_6_7_8_9_10_11_12_13_14_15>,
                           SymmetricArrayRepresentation<MaterializedTupleRef_V14_0_1_2_3_4_5_6_7_8_9_10_11_12_13_14_15>> reader;
                           // just always broadcast the name to all cores
                           // although for some queries it is unnecessary
                           auto l_V14 = reader.read( FLAGS_input_file_lineitem + ".bin" );
                           on_all_cores([=] {
                                V14 = l_V14;
                           });

                           } else {

                           CHECK(false) << "only --bin=true supported for symmetric array repr";

                           }
auto end_2 = walltime();
auto runtime_2 = end_2 - start_2;
VLOG(1)  << "pipeline 2: " << runtime_2 << " s";

VLOG(1)  << "timestamp 2 end " << std::setprecision(15) << end_2;

        
auto end_V31 = walltime();

auto runtime_V31 = end_V31 - start_V31;

saved_scan_runtime += runtime_V31;
VLOG(1) << "pipeline group V31: " << runtime_V31 << " s";
Grappa::Metrics::reset();
Grappa::Metrics::reset();
Grappa::Metrics::start_tracing();
auto start_V32 = walltime();
// Compiled subplan for GrappaStore(public:adhoc:q17)[GrappaApply(avg_yearly=($0 / 7.0))[GrappaGroupBy(; SUM($0))[GrappaApply(l_extendedprice=$1)[GrappaSelect(($0 < $2))[GrappaBroadcastCrossProduct[GrappaApply(l_quantity=$1,l_extendedprice=$2)[GrappaHashJoin(($0 = $3))[GrappaApply(l_partkey=$1,l_quantity=$4,l_extendedprice=$5)[GrappaMemoryScan[GrappaFileScan(public:adhoc:lineitem)]],GrappaApply(p_partkey=$0)[GrappaSelect((($6 = "MED BOX") and ($3 = "Brand#23")))[GrappaMemoryScan[GrappaFileScan(public:adhoc:part)]]]]],GrappaApply(frac_avg_lq=(0.2 * ($0 / $1)))[GrappaGroupBy(; SUM($0),COUNT($0))[GrappaApply(l_quantity=$1)[GrappaHashJoin(($0 = $2))[GrappaApply(l_partkey=$1,l_quantity=$4)[GrappaMemoryScan[GrappaFileScan(public:adhoc:lineitem)]],GrappaApply(p_partkey=$0)[GrappaSelect((($6 = "MED BOX") and ($3 = "Brand#23")))[GrappaMemoryScan[GrappaFileScan(public:adhoc:part)]]]]]]]]]]]]]

CompletionEvent p_task_1;
spawn(&p_task_1, [=] {

        
        auto start_1 = walltime();
VLOG(1)  << "timestamp 1 start " << std::setprecision(15) << start_1;

forall<&V12>( V11.data, [=](MaterializedTupleRef_V11_0_1_2_3_4_5_6_7_8& t_007) {
if (( (( (t_007.f6) == (("MED BOX")) )) and (( (t_007.f3) == (("Brand#23")) )) )) {
  // GrappaApply(p_partkey=$0)
MaterializedTupleRef_V10_0 t_006;t_006.f0 = t_007.f0;
// right side of GrappaHashJoin(($0 = $2))[GrappaApply(l_partkey=$1,l_quantity=$4)[GrappaMemoryScan[GrappaFileScan(public:adhoc:lineitem)]],GrappaApply(p_partkey=$0)[GrappaSelect((($6 = "MED BOX") and ($3 = "Brand#23")))[GrappaMemoryScan[GrappaFileScan(public:adhoc:part)]]]]

hash_000.insert_async<&hash_000_g,&V12 >(std::make_tuple(t_006.f0), t_006);
}

});
auto end_1 = walltime();
auto runtime_1 = end_1 - start_1;
VLOG(1)  << "pipeline 1: " << runtime_1 << " s";

VLOG(1)  << "timestamp 1 end " << std::setprecision(15) << end_1;

        
});

CompletionEvent p_task_3;
spawn(&p_task_3, [=,&p_task_1] {

        p_task_1.wait();
        auto start_3 = walltime();
VLOG(1)  << "timestamp 3 start " << std::setprecision(15) << start_3;

forall<&V15>( V14.data, [=](MaterializedTupleRef_V14_0_1_2_3_4_5_6_7_8_9_10_11_12_13_14_15& t_009) {
// GrappaApply(l_partkey=$1,l_quantity=$4)
MaterializedTupleRef_V13_0_1 t_008;t_008.f0 = t_009.f1;
t_008.f1 = t_009.f4;
// left side of GrappaHashJoin(($0 = $2))[GrappaApply(l_partkey=$1,l_quantity=$4)[GrappaMemoryScan[GrappaFileScan(public:adhoc:lineitem)]],GrappaApply(p_partkey=$0)[GrappaSelect((($6 = "MED BOX") and ($3 = "Brand#23")))[GrappaMemoryScan[GrappaFileScan(public:adhoc:part)]]]]

hash_000.lookup_iter<&V15, &hash_000_g>( std::make_tuple(t_008.f0), [=](MaterializedTupleRef_V10_0& V16) {
  join_coarse_result_count++;
  MaterializedTupleRef_V17_0_1_2 t_010 = create_V18(t_008, V16);
  // GrappaApply(l_quantity=$1)
MaterializedTupleRef_V8_0 t_005;t_005.f0 = t_010.f1;
auto GrappaGroupBy_hash_001_local_ptr = GrappaGroupBy_hash_001.localize();
*GrappaGroupBy_hash_001_local_ptr = __V6_update(*GrappaGroupBy_hash_001_local_ptr, t_005);
});
});
auto end_3 = walltime();
auto runtime_3 = end_3 - start_3;
VLOG(1)  << "pipeline 3: " << runtime_3 << " s";

VLOG(1)  << "timestamp 3 end " << std::setprecision(15) << end_3;

        
});

CompletionEvent p_task_4;
spawn(&p_task_4, [=,&p_task_3] {

        p_task_3.wait();
        auto start_4 = walltime();
VLOG(1)  << "timestamp 4 start " << std::setprecision(15) << start_4;

// scan of GrappaGroupBy(; SUM($0),COUNT($0))[GrappaApply(l_quantity=$1)[GrappaHashJoin(($0 = $2))[GrappaApply(l_partkey=$1,l_quantity=$4)[GrappaMemoryScan[GrappaFileScan(public:adhoc:lineitem)]],GrappaApply(p_partkey=$0)[GrappaSelect((($6 = "MED BOX") and ($3 = "Brand#23")))[GrappaMemoryScan[GrappaFileScan(public:adhoc:part)]]]]]]

auto t_011_tmp = reduce<
MaterializedTupleRef_V6_0_1, &__V6_combine
>(GrappaGroupBy_hash_001);


MaterializedTupleRef_V20_0_1 t_011;
t_011.f0 = t_011_tmp.f0;
t_011.f1 = t_011_tmp.f1;



// GrappaApply(frac_avg_lq=(0.2 * ($0 / $1)))
MaterializedTupleRef_V5_0 t_003;t_003.f0 = ( (0.2) * (( (t_011.f0) / (t_011.f1) )) );
// GrappaBroadcastCrossProduct RIGHT
on_all_cores([=] {
                  t_012 = t_003;
                   });
                   

// putting a wait here satisfies the invariant that inner code depends
// on global synchronization by the pipeline source
V19.wait();

auto end_4 = walltime();
auto runtime_4 = end_4 - start_4;
VLOG(1)  << "pipeline 4: " << runtime_4 << " s";

VLOG(1)  << "timestamp 4 end " << std::setprecision(15) << end_4;

        
});

CompletionEvent p_task_5;
spawn(&p_task_5, [=,&p_task_1
,&p_task_4] {

        p_task_1.wait();
p_task_4.wait();
        auto start_5 = walltime();
VLOG(1)  << "timestamp 5 start " << std::setprecision(15) << start_5;

forall<&V23>( V14.data, [=](MaterializedTupleRef_V14_0_1_2_3_4_5_6_7_8_9_10_11_12_13_14_15& t_009) {
// GrappaApply(l_partkey=$1,l_quantity=$4,l_extendedprice=$5)
MaterializedTupleRef_V22_0_1_2 t_014;t_014.f0 = t_009.f1;
t_014.f1 = t_009.f4;
t_014.f2 = t_009.f5;
// left side of GrappaHashJoin(($0 = $3))[GrappaApply(l_partkey=$1,l_quantity=$4,l_extendedprice=$5)[GrappaMemoryScan[GrappaFileScan(public:adhoc:lineitem)]],GrappaApply(p_partkey=$0)[GrappaSelect((($6 = "MED BOX") and ($3 = "Brand#23")))[GrappaMemoryScan[GrappaFileScan(public:adhoc:part)]]]]

hash_000.lookup_iter<&V23, &hash_000_g>( std::make_tuple(t_014.f0), [=](MaterializedTupleRef_V10_0& V24) {
  join_coarse_result_count++;
  MaterializedTupleRef_V25_0_1_2_3 t_015 = create_V26(t_014, V24);
  // GrappaApply(l_quantity=$1,l_extendedprice=$2)
MaterializedTupleRef_V21_0_1 t_013;t_013.f0 = t_015.f1;
t_013.f1 = t_015.f2;
// GrappaBroadcastCrossProduct LEFT

            MaterializedTupleRef_V27_0_1_2 t_016 =
              create_V28(t_013, t_012);
              if (( (t_016.f0) < (t_016.f2) )) {
  // GrappaApply(l_extendedprice=$1)
MaterializedTupleRef_V4_0 t_002;t_002.f0 = t_016.f1;
auto GrappaGroupBy_hash_000_local_ptr = GrappaGroupBy_hash_000.localize();
*GrappaGroupBy_hash_000_local_ptr = __V2_update(*GrappaGroupBy_hash_000_local_ptr, t_002);
}

});
});
auto end_5 = walltime();
auto runtime_5 = end_5 - start_5;
VLOG(1)  << "pipeline 5: " << runtime_5 << " s";

VLOG(1)  << "timestamp 5 end " << std::setprecision(15) << end_5;

        
});

CompletionEvent p_task_6;
spawn(&p_task_6, [=,&p_task_5] {

        p_task_5.wait();
        auto start_6 = walltime();
VLOG(1)  << "timestamp 6 start " << std::setprecision(15) << start_6;

// scan of GrappaGroupBy(; SUM($0))[GrappaApply(l_extendedprice=$1)[GrappaSelect(($0 < $2))[GrappaBroadcastCrossProduct[GrappaApply(l_quantity=$1,l_extendedprice=$2)[GrappaHashJoin(($0 = $3))[GrappaApply(l_partkey=$1,l_quantity=$4,l_extendedprice=$5)[GrappaMemoryScan[GrappaFileScan(public:adhoc:lineitem)]],GrappaApply(p_partkey=$0)[GrappaSelect((($6 = "MED BOX") and ($3 = "Brand#23")))[GrappaMemoryScan[GrappaFileScan(public:adhoc:part)]]]]],GrappaApply(frac_avg_lq=(0.2 * ($0 / $1)))[GrappaGroupBy(; SUM($0),COUNT($0))[GrappaApply(l_quantity=$1)[GrappaHashJoin(($0 = $2))[GrappaApply(l_partkey=$1,l_quantity=$4)[GrappaMemoryScan[GrappaFileScan(public:adhoc:lineitem)]],GrappaApply(p_partkey=$0)[GrappaSelect((($6 = "MED BOX") and ($3 = "Brand#23")))[GrappaMemoryScan[GrappaFileScan(public:adhoc:part)]]]]]]]]]]]

auto t_017_tmp = reduce<
MaterializedTupleRef_V2_0, &__V2_combine
>(GrappaGroupBy_hash_000);


MaterializedTupleRef_V30_0 t_017;
t_017.f0 = t_017_tmp.f0;



// GrappaApply(avg_yearly=($0 / 7.0))
MaterializedTupleRef_V1_0 t_000;t_000.f0 = ( (t_017.f0) / (7.0) );
result.push_back(t_000);
VLOG(2) << t_000;


// putting a wait here satisfies the invariant that inner code depends
// on global synchronization by the pipeline source
V29.wait();

auto end_6 = walltime();
auto runtime_6 = end_6 - start_6;
VLOG(1)  << "pipeline 6: " << runtime_6 << " s";

VLOG(1)  << "timestamp 6 end " << std::setprecision(15) << end_6;

        
});
p_task_5.wait();
p_task_6.wait();
p_task_3.wait();
p_task_4.wait();
p_task_1.wait();
auto end_V32 = walltime();
Grappa::Metrics::stop_tracing();
auto runtime_V32 = end_V32 - start_V32;

in_memory_runtime += runtime_V32;
VLOG(1) << "pipeline group V32: " << runtime_V32 << " s";


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
