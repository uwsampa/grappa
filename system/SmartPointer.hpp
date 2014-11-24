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


#ifndef _SMTPTR_HPP__
#define _SMTPTR_HPP__

#include <Grappa.hpp>

using namespace Grappa;

#if 0
# include <iomanip>
using namespace std;
# define DPRINT(m) cout << setw(14) << (this) << ": " << m << endl
# define DPRINT2(m,n) cout << setw(14) << (this) << ": " << m << n << endl
#else
# define DPRINT(m)
# define DPRINT2(m,n)
#endif

/*
 * Reference manager class.
 *
 *
 */
template <class T>
class RefManager
{
  T *ptr;           /* pointer to the object */
  size_t refcnt;    /* reference counter */

public:

  explicit RefManager(T *p = 0) : ptr(p), refcnt(1) 
  {
    DPRINT("*** RefManager constructor ***");
  }

  ~RefManager() 
  {
    DPRINT("*** RefManager destructor ***");
  }

  T& getRef() { return *ptr; }
  T* getPtr() { return ptr; }

  void Increment() 
  { 
    refcnt++; 
    DPRINT2("   -> RefManager incremented: ", refcnt); 
  }

  size_t Decrement() 
  { 
    refcnt--; 
    DPRINT2("   -> RefManager decremented: ", refcnt);
    if (refcnt == 0) {
      DPRINT("      -> RefManager: Object deleted");
      delete ptr;
    }
    return refcnt;
  }

  size_t numRefs() const { return refcnt; }
};


/* forward declaration for the clone function */
template <typename T>
T *clone(GlobalAddress<T> obj);




//
// Smart Pointer class
//
//
template <class T>
class SmtPtr
{
  GlobalAddress< RefManager<T> > refMan;     /* Global address of the reference manager */ 

public:
  explicit SmtPtr(T *ptr): refMan(make_global(new RefManager<T>(ptr))) 
  {
    DPRINT("*** SmtPtr constructor ***");
  }

  // copy constructor
  SmtPtr(const SmtPtr &other): refMan(other.refMan)
  {
    DPRINT("*** SmtPtr copy constructor ***");

    delegate::call(refMan, [](RefManager<T> *ref) { ref->Increment(); });
  }

//  T& operator*() { return reference_->getRef(); }

  T* operator->() 
  {
    // TODO: assert it is not called from remote node
    return refMan->getPtr();
  }


  SmtPtr<T> clone() const 
  {
    DPRINT("   -> SmtPtr clone");

    T *local = ::clone(delegate::call(refMan, [](RefManager<T> *ref) 
          { return make_global(ref->getPtr()); }));
 
    return SmtPtr<T>(local);
  }

  size_t getNumRefs()
  {
    return delegate::call(refMan, [](RefManager<T> *ref) { return ref->numRefs(); });
  }

  // destructor
  ~SmtPtr()
  {
    DPRINT("*** SmtPtr destructor ***");
    
    RefManager<T> *ref = (RefManager<T> *)refMan.pointer();
    delegate::call(refMan.core(), [ref] { if (!ref->Decrement()) delete ref; });
  }

};




#endif /* _SMTPTR_HPP_ */
