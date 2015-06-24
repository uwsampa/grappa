////////////////////////////////////////////////////////////////////////
// This file is part of Grappa, a system for scaling irregular
// applications on commodity clusters. 

// Copyright (C) 2010-2014 University of Washington and Battelle
// Memorial Institute. University of Washington authorizes use of this
// Grappa software.

// Grappa is free software: you can redistribute it and/or modify it
// under the terms of the Affero General Public License as published
// by Affero, Inc., either version 1 of the License, or (at your
// option) any later version.

// Grappa is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// Affero General Public License for more details.

// You should have received a copy of the Affero General Public
// License along with this program. If not, you may obtain one from
// http://www.affero.org/oagpl.html.
////////////////////////////////////////////////////////////////////////

/// Tests for delegate operations

#include <boost/test/unit_test.hpp>

#include <Grappa.hpp>
#include <GlobalAllocator.hpp>

#include "relation_io.hpp"
#include "strings.h"

// Unfortunately Grappa addressing does not work for things > 64 bytes...

using namespace Grappa;

BOOST_AUTO_TEST_SUITE( Relation_io_tests );

std::vector<std::string> schema = {"a","b","c"};
 
class MaterializedTupleRef_V3_0_1 {
    // Invariant: data stored in _fields is always in the representation
    // specified by _scheme.

    public:
    
        std::array<char, MAX_STR_LEN> f0;
    
        double f1;
    

    static constexpr int numFields() {
      return 2;
    }

    static size_t fieldsSize() {
        const MaterializedTupleRef_V3_0_1 _t;
        return
        
        ((char*)&_t.f1) + sizeof(_t.f1) - ((char*)&_t);
    }

    MaterializedTupleRef_V3_0_1 () {
      // no-op
    }

    //template <typename OT>
    //MaterializedTupleRef_V3_0_1 (const OT& other) {
    //  std::memcpy(this, &other, sizeof(MaterializedTupleRef_V3_0_1));
    //}
    MaterializedTupleRef_V3_0_1 (
                               const std::array<char, MAX_STR_LEN>& a0
                               ,
                       
                       
                               const double& a1
                               
                       
                       ) {
        
            f0 = a0;
        
            f1 = a1;
        
        
    }

    
    


    MaterializedTupleRef_V3_0_1(const std::tuple<
        
        std::array<char, MAX_STR_LEN>
        ,
        
        double
        
        
            >& o) {
        
            f0 = std::get<0>(o);
        
            f1 = std::get<1>(o);
        
        
     }

     std::tuple<
        
        std::array<char, MAX_STR_LEN>
        ,
        
        double
        
        
     > to_tuple() {

        std::tuple<
        
        std::array<char, MAX_STR_LEN>
        ,
        
        double
        
        
        > r;
        
            std::get<0>(r) = f0;
        
            std::get<1>(r) = f1;
        
        
        return r;
     }

    // shamelessly terrible disambiguation: one solution is named factory methods
    //MaterializedTupleRef_V3_0_1 (std::vector<int64_t> vals, bool ignore1, bool ignore2) {
    //    
    //        f0 = vals[0];
    //    
    //        f1 = vals[1];
    //    
    //        f2 = vals[2];
    //    
    //}

    // use the tuple schema to interpret the input stream
    static MaterializedTupleRef_V3_0_1 fromIStream(std::istream& ss, char delim=' ') {
        MaterializedTupleRef_V3_0_1 _ret;

        
            
               {
               std::string _temp;
               std::getline(ss, _temp, delim);
               _ret.f0 = to_array<MAX_STR_LEN, std::string>(_temp);
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
       
         
      VLOG(1) << "writing " << this->fieldsSize();
        os.write((char*)this, this->fieldsSize());
         
       
    }

    //template <typename Tuple, typename T>
    //MaterializedTupleRef_V3_0_1 (const Tuple& v0, const T& from) {
    //    constexpr size_t v0_size = std::tuple_size<Tuple>::value;
    //    constexpr int from_size = T::numFields();
    //    static_assert(MaterializedTupleRef_V3_0_1::numFields() == (v0_size + from_size), "constructor only works on same number of total fields");
    //    TupleUtils::assign<0, decltype(_scheme)>(_fields, v0);
    //    std::memcpy(((char*)&_fields)+v0_size*sizeof(int64_t), &(from._fields), from_size*sizeof(int64_t));
    //}

    //template <typename Tuple>
    //MaterializedTupleRef_V3_0_1 (const Tuple& v0) {
    //    static_assert(MaterializedTupleRef_V3_0_1::numFields() == (std::tuple_size<Tuple>::value), "constructor only works on same number of total fields");
    //    TupleUtils::assign<0, decltype(_scheme)>(_fields, v0);
    //}

    inline std::ostream& dump(std::ostream& o) const {
      o << "Materialized(";

      
        o << "" << f0 << ",";
      
        o << "" << f1 << ",";
      
      o << ")";
      return o;
    }
  
    friend inline std::ostream& operator<< (std::ostream& o, const MaterializedTupleRef_V3_0_1& t) {
      return t.dump(o);
    }

    
  } GRAPPA_BLOCK_ALIGNED;


class MaterializedTupleRef_V1_0_1_2 {
    // Invariant: data stored in _fields is always in the representation
    // specified by _scheme.

    public:

        int64_t f0;

        std::array<char, MAX_STR_LEN> f1;

        std::array<char, MAX_STR_LEN> f2;


    static constexpr int numFields() {
      return 3;
    }

    static size_t fieldsSize() {
        const MaterializedTupleRef_V1_0_1_2 _t;
        return

        sizeof(_t.f0) +

        sizeof(_t.f1) +

        sizeof(_t.f2) +

        0;
    }

    MaterializedTupleRef_V1_0_1_2 () {
      // no-op
    }

    //template <typename OT>
    //MaterializedTupleRef_V1_0_1_2 (const OT& other) {
    //  std::memcpy(this, &other, sizeof(MaterializedTupleRef_V1_0_1_2));
    //}
    MaterializedTupleRef_V1_0_1_2 (
                               const int64_t& a0
                               ,

                               const std::array<char, MAX_STR_LEN>& a1
                               ,

                               const std::array<char, MAX_STR_LEN>& a2


                       ) {

            f0 = a0;

            f1 = a1;

            f2 = a2;

    }

    // shamelessly terrible disambiguation: one solution is named factory methods
    //MaterializedTupleRef_V1_0_1_2 (std::vector<int64_t> vals, bool ignore1, bool ignore2) {
    //
    //        f0 = vals[0];
    //
    //        f1 = vals[1];
    //
    //        f2 = vals[2];
    //
    //}

    // use the tuple schema to interpret the input stream
    static MaterializedTupleRef_V1_0_1_2 fromIStream(std::istream& ss, char d=' ') {
        MaterializedTupleRef_V1_0_1_2 _ret;


                {
               ss >> _ret.f0;
               // throw away comma
               std::string _temp;
               std::getline(ss, _temp, d);
                }



               {
               std::string _temp;
               std::getline(ss, _temp, d);
               _ret.f1 = to_array<MAX_STR_LEN, std::string>(_temp);
               }



               {
               std::string _temp;
               std::getline(ss, _temp, d);
               _ret.f2 = to_array<MAX_STR_LEN, std::string>(_temp);
               }



        return _ret;
    }

    void toOStream(std::ostream& os) const {


            os.write((char*)&f0, sizeof(int64_t));



            os.write(f1.data(), (size_t)MAX_STR_LEN * sizeof(char));



            os.write(f2.data(), (size_t)MAX_STR_LEN * sizeof(char));


    }


    //template <typename Tuple, typename T>
    //MaterializedTupleRef_V1_0_1_2 (const Tuple& v0, const T& from) {
    //    constexpr size_t v0_size = std::tuple_size<Tuple>::value;
    //    constexpr int from_size = T::numFields();
    //    static_assert(MaterializedTupleRef_V1_0_1_2::numFields() == (v0_size + from_size), "constructor only works on same number of total fields");
    //    TupleUtils::assign<0, decltype(_scheme)>(_fields, v0);
    //    std::memcpy(((char*)&_fields)+v0_size*sizeof(int64_t), &(from._fields), from_size*sizeof(int64_t));
    //}

    //template <typename Tuple>
    //MaterializedTupleRef_V1_0_1_2 (const Tuple& v0) {
    //    static_assert(MaterializedTupleRef_V1_0_1_2::numFields() == (std::tuple_size<Tuple>::value), "constructor only works on same number of total fields");
    //    TupleUtils::assign<0, decltype(_scheme)>(_fields, v0);
    //}

    inline std::ostream& dump(std::ostream& o) const {
      o << "Materialized(";


        o << f0 << ",";

        o << std::string(f1.data()) << ",";

        o << std::string(f2.data()) << ",";


      o << ")";
      return o;
    }
  
    friend inline std::ostream& operator<< (std::ostream& o, const MaterializedTupleRef_V1_0_1_2& t) {
      return t.dump(o);
    }

  } GRAPPA_BLOCK_ALIGNED;




std::vector<MaterializedTupleRef_V1_0_1_2> more_data;
std::vector<MaterializedTupleRef_V3_0_1> dt_results;

BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS );
  Grappa::run([]{

    BOOST_CHECK_EQUAL( 2, Grappa::cores() );

    // write to new file
    std::string write_file = "write.bin";
    MaterializedTupleRef_V1_0_1_2 one;
    MaterializedTupleRef_V1_0_1_2 two;
    one.f0 = 51;
    one.f1 = {'c','o','f','f','e','e','\0'};
    one.f2 = {'d', 'o', 'g', '\0'};
    two.f0 = 57;
    two.f1 = {'s','u','p','e','r','\0'};
    two.f2 = {'a','b','c','d', '\0'};
    more_data.push_back(one);
    more_data.push_back(two);

    writeTuplesUnordered<MaterializedTupleRef_V1_0_1_2>( &more_data, write_file );

    // try read
    BinaryRelationFileReader<MaterializedTupleRef_V1_0_1_2> reader1;
    Relation<MaterializedTupleRef_V1_0_1_2> results = reader1.read( write_file );

    BOOST_CHECK_EQUAL( 2, results.numtuples );
    auto r0 = Grappa::delegate::read(results.data);
    auto r1 = Grappa::delegate::read(results.data + 1);
    // the tuples might be read in either order
    if (one.f0 == r0.f0) {
      BOOST_CHECK_EQUAL( one.f0, r0.f0 );
      BOOST_CHECK( one.f1 == r0.f1 );
      BOOST_CHECK( one.f2 == r0.f2 );

      BOOST_CHECK_EQUAL( two.f0, r1.f0 );
      std::cout << "(" << std::string(r1.f1.data()) << ") (" << std::string(r1.f2.data()) << ")" << std::endl;
      BOOST_CHECK( two.f1 == r1.f1 );
      BOOST_CHECK( two.f2 == r1.f2 );
    } else {
      BOOST_CHECK_EQUAL( one.f0, r1.f0 );
      BOOST_CHECK( one.f1 == r1.f1 );
      BOOST_CHECK( one.f2 == r1.f2 );

      BOOST_CHECK_EQUAL( two.f0, r0.f0 );
      BOOST_CHECK( two.f1 == r0.f1 );
      BOOST_CHECK( two.f2 == r0.f2 );
    }

    // write to existing file
    MaterializedTupleRef_V1_0_1_2 three;
    MaterializedTupleRef_V1_0_1_2 four;
    three.f0 = 83;
    three.f1 = {'x','y','z','\0'};
    four.f0 = 87;
    four.f1 = {'c','h','e','c','k','\0'};
    more_data.clear();
    more_data.push_back(one);
    more_data.push_back(two);
    more_data.push_back(three);
    more_data.push_back(four);

    // more_data now has tuples 1 - 4
    writeTuplesUnordered<MaterializedTupleRef_V1_0_1_2>( &more_data, write_file );

    // verify write
      BinaryRelationFileReader<MaterializedTupleRef_V1_0_1_2> reader2;
      results = reader2.read( write_file );

    BOOST_CHECK_EQUAL( 4, results.numtuples );

    r0 = Grappa::delegate::read(results.data);
    r1 = Grappa::delegate::read(results.data + 1);
    auto r2 = Grappa::delegate::read(results.data + 2);
    auto r3 = Grappa::delegate::read(results.data + 3);
    if (one.f0 == r0.f0) {
      BOOST_CHECK_EQUAL( one.f0, r0.f0 );
      BOOST_CHECK( one.f1 == r0.f1 );
      BOOST_CHECK( one.f2 == r0.f2 );
    } else if (one.f0 == r1.f0) {
      BOOST_CHECK_EQUAL( one.f0, r1.f0 );
      BOOST_CHECK( one.f1 == r1.f1 );
      BOOST_CHECK( one.f2 == r1.f2 );
    } else if (one.f0 == r2.f0) {
      BOOST_CHECK_EQUAL( one.f0, r2.f0 );
      BOOST_CHECK( one.f1 == r2.f1 );
      BOOST_CHECK( one.f2 == r2.f2 );
    } else {
      BOOST_CHECK_EQUAL( one.f0, r3.f0 );
      BOOST_CHECK( one.f1 == r3.f1 );
      BOOST_CHECK( one.f2 == r3.f2 );
    }

  SplitsRelationFileReader<JSONRowParser<MaterializedTupleRef_V1_0_1_2, &schema>, MaterializedTupleRef_V1_0_1_2> reader3;
  results = reader3.read( "splits_test" );
  BOOST_CHECK_EQUAL(results.numtuples, 6);
  forall( results.data, results.numtuples, [=](MaterializedTupleRef_V1_0_1_2& t) {
      std::cout << t << std::endl;
      });

  // test with doubles
  MaterializedTupleRef_V3_0_1 dt1(to_array<MAX_STR_LEN>(std::string("compris indic")),-0.35686);
  MaterializedTupleRef_V3_0_1 dt2(to_array<MAX_STR_LEN>(std::string("distanc computerimpl")),1.73678);
  MaterializedTupleRef_V3_0_1 dt3(to_array<MAX_STR_LEN>(std::string("interv determin")),-0.0503026);
  MaterializedTupleRef_V3_0_1 dt4(to_array<MAX_STR_LEN>(std::string("select search")),0.74823);
  MaterializedTupleRef_V3_0_1 dt5(to_array<MAX_STR_LEN>(std::string("super gain")),1.997);
  MaterializedTupleRef_V3_0_1 dt6(to_array<MAX_STR_LEN>(std::string("cat pancake")),-0.1154);
  MaterializedTupleRef_V3_0_1 dt7(to_array<MAX_STR_LEN>(std::string("specialized device")),0.00131);

  dt_results.push_back(dt1);
  dt_results.push_back(dt2);
  dt_results.push_back(dt3);
  dt_results.push_back(dt4);
  dt_results.push_back(dt5);
  dt_results.push_back(dt6);
  dt_results.push_back(dt7);

  std::string write_file2("write2.bin");
  writeTuplesUnordered<MaterializedTupleRef_V3_0_1>( &dt_results, write_file2 );

  BinaryRelationFileReader<MaterializedTupleRef_V3_0_1> reader_d;
  Relation<MaterializedTupleRef_V3_0_1> results_d = reader_d.read( write_file2 );

  BOOST_CHECK_EQUAL( 7, results_d.numtuples );

  forall( results_d.data, results_d.numtuples, [=](MaterializedTupleRef_V3_0_1& t) {
      std::cout << t << std::endl;
        BOOST_CHECK( (t.f0 == dt1.f0 && t.f1 == dt1.f1)
          || (t.f0 == dt2.f0 && t.f1 == dt2.f1)
          || (t.f0 == dt3.f0 && t.f1 == dt3.f1)
          || (t.f0 == dt4.f0 && t.f1 == dt4.f1) 
          || (t.f0 == dt5.f0 && t.f1 == dt5.f1) 
          || (t.f0 == dt6.f0 && t.f1 == dt6.f1) 
          || (t.f0 == dt7.f0 && t.f1 == dt7.f1) );
  });
}); // end grappa::run()



  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();
