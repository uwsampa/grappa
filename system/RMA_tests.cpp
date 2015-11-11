////////////////////////////////////////////////////////////////////////
// Copyright (c) 2010-2015, University of Washington and Battelle
// Memorial Institute.  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//     * Redistributions of source code must retain the above
//       copyright notice, this list of conditions and the following
//       disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials
//       provided with the distribution.
//     * Neither the name of the University of Washington, Battelle
//       Memorial Institute, or the names of their contributors may be
//       used to endorse or promote products derived from this
//       software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// UNIVERSITY OF WASHINGTON OR BATTELLE MEMORIAL INSTITUTE BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
////////////////////////////////////////////////////////////////////////

#include <boost/test/unit_test.hpp>

#include <Grappa.hpp>
#include <RMA.hpp>

BOOST_AUTO_TEST_SUITE( RMA_tests );

using namespace Grappa;

BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS );
  Grappa::run([]{
    Grappa::on_all_cores([]{
      const int count = 16;

      // allocate array
      void * p1 = Grappa::impl::global_rma.allocate( sizeof(int64_t) * count );
      int64_t * base = static_cast<int64_t*>( p1 );

      // initialize array
      for( int i = 0; i < count; ++i ) {
        base[i] = 0;
      }

      Grappa::barrier();

      { // test async non-blocking puts
        
        // put modified array to all other cores
        if( Grappa::mycore() == 0 ) {
          for( int i = 0; i < Grappa::cores(); ++i ) {
            int64_t src[ count ];
            for( int j = 0; j < count; ++j ) {
              src[j] = i;
            }
            Grappa::impl::global_rma.put_bytes_nbi( i, base, src, sizeof(src) );
          }
        }
        
        Grappa::impl::global_rma.fence();
        
        Grappa::barrier();
        
        // check array
        for( int i = 0; i < count; ++i ) {
          BOOST_CHECK_EQUAL( base[i], Grappa::mycore() );
        }
        
        Grappa::barrier();
      }

      { // test non-blocking puts with blocking wait
        
        // initialize array
        for( int i = 0; i < count; ++i ) {
          base[i] = 0;
        }

        Grappa::barrier();

        // put modified array to all other cores
        if( Grappa::mycore() == 0 ) {
          for( int i = 0; i < Grappa::cores(); ++i ) {
            int64_t src[ count ];
            for( int j = 0; j < count; ++j ) {
              src[j] = i;
            }
            auto req = Grappa::impl::global_rma.put_bytes_nb( i, base, src, sizeof(src) );
            req.wait();
          }
        }
        
        Grappa::barrier();
        
        // check array
        for( int i = 0; i < count; ++i ) {
          BOOST_CHECK_EQUAL( base[i], Grappa::mycore() );
        }

        Grappa::barrier();
      }

      { // test non-blocking puts with suspending wait
        
        // initialize array
        for( int i = 0; i < count; ++i ) {
          base[i] = 0;
        }

        Grappa::barrier();

        // put modified array to all other cores
        if( Grappa::mycore() == 0 ) {
          for( int i = 0; i < Grappa::cores(); ++i ) {
            int64_t src[ count ];
            for( int j = 0; j < count; ++j ) {
              src[j] = i * 10;
            }
            global_communicator.with_request_do_blocking( [=] ( MPI_Request * request_p ) {
              Grappa::impl::global_rma.put_bytes_nb( i, base, src, sizeof(src), request_p );
            });
          }
        }
        
        Grappa::barrier();
        
        // check array
        for( int i = 0; i < count; ++i ) {
          BOOST_CHECK_EQUAL( base[i], Grappa::mycore() * 10 );
        }

        Grappa::barrier();
      }

      { // test non-blocking puts with test
        
        // initialize array
        for( int i = 0; i < count; ++i ) {
          base[i] = 0;
        }

        Grappa::barrier();

        // put modified array to all other cores
        if( Grappa::mycore() == 0 ) {
          for( int i = 0; i < Grappa::cores(); ++i ) {
            int64_t src[ count ];
            for( int j = 0; j < count; ++j ) {
              src[j] = i;
            }
            auto req = Grappa::impl::global_rma.put_bytes_nb( i, base, src, sizeof(src) );
            while( !req.test() ) {
              ;
            }
          }
        }
        
        Grappa::barrier();
        
        // check array
        for( int i = 0; i < count; ++i ) {
          BOOST_CHECK_EQUAL( base[i], Grappa::mycore() );
        }

        Grappa::barrier();
      }

      { // test blocking puts
        
        // initialize array
        for( int i = 0; i < count; ++i ) {
          base[i] = 0;
        }

        Grappa::barrier();

        // put modified array to all other cores
        if( Grappa::mycore() == 0 ) {
          for( int i = 0; i < Grappa::cores(); ++i ) {
            int64_t src[ count ];
            for( int j = 0; j < count; ++j ) {
              src[j] = i;
            }
            Grappa::impl::global_rma.put_bytes( i, base, src, sizeof(src) );
          }
        }
        
        Grappa::barrier();
        
        // check array
        for( int i = 0; i < count; ++i ) {
          BOOST_CHECK_EQUAL( base[i], Grappa::mycore() );
        }

        Grappa::barrier();
      }




      { // test async non-blocking gets
        
        // initialize array
        for( int i = 0; i < count; ++i ) {
          base[i] = Grappa::mycore();
        }

        Grappa::barrier();
        
        // check remote arrays
        if( Grappa::mycore() == 0 ) {
          for( int i = 0; i < Grappa::cores(); ++i ) {
            int64_t remote[ count ];
            for( int j = 0; j < count; ++j ) {
              remote[j] = -1;
            }

            Grappa::impl::global_rma.get_bytes_nbi( remote, i, base, sizeof(remote) );
            Grappa::impl::global_rma.fence();
            
            for( int j = 0; j < count; ++j ) {
              BOOST_CHECK_EQUAL( remote[j], i );
            }
          }
        }
        
        Grappa::barrier();
      }

      { // test non-blocking gets
        
        // initialize array
        for( int i = 0; i < count; ++i ) {
          base[i] = Grappa::mycore();
        }

        Grappa::barrier();
        
        // check remote arrays
        if( Grappa::mycore() == 0 ) {
          for( int i = 0; i < Grappa::cores(); ++i ) {
            int64_t remote[ count ];
            for( int j = 0; j < count; ++j ) {
              remote[j] = -1;
            }

            auto req = Grappa::impl::global_rma.get_bytes_nb( remote, i, base, sizeof(remote) );
            req.wait();
            
            for( int j = 0; j < count; ++j ) {
              BOOST_CHECK_EQUAL( remote[j], i );
            }
          }
        }
        
        Grappa::barrier();
      }

      { // test non-blocking gets with test
        
        // initialize array
        for( int i = 0; i < count; ++i ) {
          base[i] = Grappa::mycore();
        }

        Grappa::barrier();
        
        // check remote arrays
        if( Grappa::mycore() == 0 ) {
          for( int i = 0; i < Grappa::cores(); ++i ) {
            int64_t remote[ count ];
            for( int j = 0; j < count; ++j ) {
              remote[j] = -1;
            }

            auto req = Grappa::impl::global_rma.get_bytes_nb( remote, i, base, sizeof(remote) );
            while( !req.test() ) {
              ;
            }
            
            for( int j = 0; j < count; ++j ) {
              BOOST_CHECK_EQUAL( remote[j], i );
            }
          }
        }
        
        Grappa::barrier();
      }

      { // test blocking gets
        
        // initialize array
        for( int i = 0; i < count; ++i ) {
          base[i] = Grappa::mycore();
        }

        Grappa::barrier();
        
        // check remote arrays
        if( Grappa::mycore() == 0 ) {
          for( int i = 0; i < Grappa::cores(); ++i ) {
            int64_t remote[ count ];
            for( int j = 0; j < count; ++j ) {
              remote[j] = -1;
            }

            Grappa::impl::global_rma.get_bytes( remote, i, base, sizeof(remote) );
            
            for( int j = 0; j < count; ++j ) {
              BOOST_CHECK_EQUAL( remote[j], i );
            }
          }
        }
        
        Grappa::barrier();
      }

      { // test other allocations

        BOOST_CHECK_EQUAL( Grappa::impl::global_rma.size(), 1 );
        
        // allocate an array for each core
        int64_t * other[ Grappa::cores() ];
        for( int i = 0; i < Grappa::cores(); ++i ) {
          other[i] = (int64_t*) Grappa::impl::global_rma.allocate( sizeof(int64_t) * Grappa::cores() );
        }

        Grappa::barrier();

        for( int64_t i = 0; i < Grappa::cores(); ++i ) {
          Grappa::impl::global_rma.put_bytes_nbi( i, &(other[i][Grappa::mycore()]), &i, sizeof(i) );
        }

        Grappa::impl::global_rma.fence();
        Grappa::barrier();
        
        // check that the writes showed up in the right place
        for( int i = 0; i < Grappa::cores(); ++i ) {
          BOOST_CHECK_EQUAL( other[Grappa::mycore()][i], Grappa::mycore() );
        }
        
        Grappa::barrier();

        // free arrays
        for( int i = Grappa::cores()-1; i >= 0; --i ) {
          Grappa::impl::global_rma.free( other[i] );
        }

        Grappa::barrier();

        BOOST_CHECK_EQUAL( Grappa::impl::global_rma.size(), 1 );

      }
      
      { // test atomic ops
        base[0] = Grappa::mycore();

        Grappa::barrier();

        int64_t source = Grappa::mycore();
        int64_t result = Grappa::impl::global_rma.atomic_op< int64_t, op::replace<int64_t> >( (Grappa::mycore() + 1) % Grappa::cores(),
                                                                                              &base[0], &source );
        BOOST_CHECK_EQUAL( result, (Grappa::mycore() + 1) % Grappa::cores() );
        
        Grappa::barrier();

        BOOST_CHECK_EQUAL( base[0], (Grappa::mycore() + Grappa::cores() - 1) % Grappa::cores() );

        base[0] = 0;
        
        Grappa::barrier();

      }

      Grappa::impl::global_rma.free( p1 );
      BOOST_CHECK_EQUAL( Grappa::impl::global_rma.size(), 0 );
    });
  });
}

BOOST_AUTO_TEST_SUITE_END();

