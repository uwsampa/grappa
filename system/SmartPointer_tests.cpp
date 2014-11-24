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


#include <boost/test/unit_test.hpp>
#include <Grappa.hpp>
#include <SmartPointer.hpp>

using namespace Grappa;
using namespace std;

// dummy class
class NonPOD
{
public:
  NonPOD(size_t size): iarray(new int[size]), size(size) 
  {
  }

  ~NonPOD() 
  { 
    delete[] iarray; 
  }

  int At(int i) { return iarray[i]; }
  void Set(int i, int v) { iarray[i] = v; }
  size_t Size() const { return size; }  

private:
  int *iarray;
  size_t size;

};


// clone
template<>
NonPOD *clone<NonPOD>(GlobalAddress<NonPOD> remoteObj)
{
  NonPOD *temp;

  int size = delegate::call(remoteObj, [](NonPOD *obj) { return obj->Size(); });

  temp = new NonPOD(size);
  for (size_t i=0; i<size; i++)
    temp->Set(i, delegate::call(remoteObj, [i](NonPOD *obj){ return obj->At(i);}));

  return temp;
}


GlobalCompletionEvent gce;      

BOOST_AUTO_TEST_SUITE( SmtPtr_tests );



BOOST_AUTO_TEST_CASE( test1 ) {

  Grappa::init( GRAPPA_TEST_ARGS );

  Grappa::run([]{
  
    // create a smart pointer for a non-POD object
    SmtPtr<NonPOD> myPtr(new NonPOD(257));

    // write something using the smart pointer
    for (int i=0; i<cores(); i++)
      myPtr->Set(i,i);
    
   
    // spawn some tasks ...
    for (int i=0; i<10; i++)
      spawn<unbound,&gce>([myPtr] {  
          // and clone the object using smart pointers
          SmtPtr<NonPOD> local = myPtr.clone();
          
          // check if copy was performed correctly
          BOOST_CHECK_EQUAL( local->At(mycore()), mycore() );
          //cout << "Core " << mycore() << " cloned correctly" << endl;
        });
   
    gce.wait();

    // Should only have one object alive at this point
    BOOST_CHECK_EQUAL( myPtr.getNumRefs(), 1 );

  });
  Grappa::finalize();

}

BOOST_AUTO_TEST_SUITE_END();

