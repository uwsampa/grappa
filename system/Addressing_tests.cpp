
#include <boost/test/unit_test.hpp>

#include "SoftXMT.hpp"
#include "Addressing.hpp"


BOOST_AUTO_TEST_SUITE( Addressing_tests );

struct array_element {
  Node node;
  int offset;
  int block;
  int index;
};

array_element global_array[ 1234 ];

BOOST_AUTO_TEST_CASE( test1 ) {

  SoftXMT_init( &(boost::unit_test::framework::master_test_suite().argc),
                &(boost::unit_test::framework::master_test_suite().argv) );

  SoftXMT_activate();

  if( 0 == SoftXMT_mynode() ) {
    int foo;
    int bar;
    GlobalAddress< int > foop = GlobalAddress< int >::TwoDimensional( &foo );
    GlobalAddress< int > barp = GlobalAddress< int >::TwoDimensional( &bar );
    BOOST_CHECK_EQUAL( &foo, foop.pointer() );
    BOOST_CHECK_EQUAL( &bar, barp.pointer() );
    BOOST_CHECK_EQUAL( 8, sizeof( barp ) );
    
    GlobalAddress< int > gp2 = localToGlobal( &foo );
    BOOST_CHECK_EQUAL( gp2.pointer(), &foo );
    

    BOOST_MESSAGE("sizeof(array_element) == " << sizeof(array_element));
    BOOST_MESSAGE("Filling in array");

    const int array_size = 12;
    
    // array for comparison
    array_element array[ array_size * SoftXMT_nodes()];
    
    // distribute elements to node arrays in round robin fashion. 
    array_element node_array[ SoftXMT_nodes() ][ array_size ];
    // these are the indices for each node
    int node_i[ SoftXMT_nodes() ];
    for( int i = 0; i < SoftXMT_nodes(); ++i ) {
      node_i[i] = 0;
    }

    for( int i = 0; i < array_size * SoftXMT_nodes(); ++i ) {
      intptr_t byte_address = i * sizeof( array_element );

      intptr_t block_index = byte_address / block_size;
      intptr_t offset_in_block = byte_address % block_size;

      Node node = block_index % SoftXMT_nodes();
      intptr_t node_block_index = block_index / SoftXMT_nodes();

      array[ i ].block = node_block_index; //( (i * sizeof(array_element)) / block_size ) / SoftXMT_nodes();
      array[ i ].offset = offset_in_block; //(i * sizeof(array_element)) % block_size;
      array[ i ].node = node; //( (i * sizeof(array_element)) / block_size ) % SoftXMT_nodes();
      //array[ i ].address = array[i].block * block_size + array[i].offset;
      array[ i ].index = i;
      //node_array[ array[i].node ][ node_i[i]++ ] = array[i];

      node_array[ node ][ node_i[ node ]++ ] = array[i];
      
      VLOG(1) << "   " << i << ":"
        //<< " local address " << array[i].address
              << " index " << array[i].index
              << " node " << array[i].node
              << " block " << array[i].block
              << " offset " << array[i].offset;
    }

    for( int node = 0; node < SoftXMT_nodes(); ++node ) {
      VLOG(1) << "Contents of node " << node << " memory";
      for( int i = 0; i < array_size; ++i ) {
        VLOG(1) << "   " << i << ":"
          //<< " local address " << array[i].address
                << " index " << node_array[node][i].index
                << " node " << node_array[node][i].node
                << " block " << node_array[node][i].block
                << " offset " << node_array[node][i].offset;
        BOOST_CHECK_EQUAL( node_array[node][i].node, node );
        //BOOST_CHECK_EQUAL( array[i].block * block_size + array[i].offset, i * sizeof(array_element) );
      }
    }


    GlobalAddress< array_element > l1 = GlobalAddress< array_element >::Linear( 0 );
    BOOST_CHECK_EQUAL( l1.pool(), 0 );
    BOOST_CHECK_EQUAL( l1.pointer(), (array_element *) 0 );
    
    ptrdiff_t offset = &array[0] - (array_element *)NULL;
    int j = 0;
    for( int i = 0; i < array_size * SoftXMT_nodes(); ++i ) {
      GlobalAddress< array_element > gap = l1 + i;
      ptrdiff_t node_base_address = &node_array[ array[i].node ][0] - (array_element *)NULL;
      BOOST_CHECK_EQUAL( array[i].node, gap.node() );

      array_element * ap = gap.pointer() + node_base_address;

      BOOST_CHECK_EQUAL( array[i].index, ap->index );
      BOOST_CHECK_EQUAL( array[i].node, ap->node );
      BOOST_CHECK_EQUAL( array[i].block, ap->block );
      BOOST_CHECK_EQUAL( array[i].offset, ap->offset );
    }
  }


  GlobalAddress< array_element > l2 = make_linear( &global_array[0] );
  BOOST_CHECK_EQUAL( l2.node(), 0 );
  BOOST_CHECK_EQUAL( l2.pointer(), &global_array[0] );

  ++l2;
  BOOST_CHECK_EQUAL( l2.node(), 0 );
  BOOST_CHECK_EQUAL( l2.pointer(), &global_array[1] );

  l2 += 3;
  BOOST_CHECK_EQUAL( l2.node(), 1 );
  BOOST_CHECK_EQUAL( l2.pointer(), &global_array[0] );

  ++l2;
  BOOST_CHECK_EQUAL( l2.node(), 1 );
  BOOST_CHECK_EQUAL( l2.pointer(), &global_array[1] );

  l2 += 4;
  BOOST_CHECK_EQUAL( l2.node(), 0 );
  BOOST_CHECK_EQUAL( l2.pointer(), &global_array[5] );

  SoftXMT_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

