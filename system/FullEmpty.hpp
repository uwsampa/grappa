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
// Energy. The Government has certain rights in the software.

#pragma once

#include "FullEmptyLocal.hpp"
#include "Delegate.hpp"

namespace Grappa {
  
  /// @addtogroup Synchronization
  /// @{
  
  template< typename T >
  void fill_remote(GlobalAddress<FullEmpty<T>> result_addr, const T& val) {
    send_heap_message(result_addr.core(), [result_addr,val]{
      result_addr->writeXF(val);
    });
  }
  
  template< typename T >
  T readFF(GlobalAddress<FullEmpty<T>> fe_addr) {
    if (fe_addr.core() == mycore()) {
      DVLOG(2) << "local";
      return fe_addr.pointer()->readFF();
    }
    
    FullEmpty<T> result;
    auto result_addr = make_global(&result);
    
    send_message(fe_addr.core(), [fe_addr,result_addr]{
      auto& fe = *fe_addr.pointer();
      
      if (fe.full()) {
        // DVLOG(2) << "no need to block";
        fill_remote(result_addr, fe.readFF());
        return;
      }
      
      VLOG(2) << "setting up to block (" << fe_addr << ")";
      auto* c = SuspendedDelegate::create([&fe,result_addr]{
          //VLOG(0) << __PRETTY_FUNCTION__ << ": suspended_delegate for " << &fe << "!";
        fill_remote(result_addr, fe.readFF());
      });
      add_waiter(&fe, c);
    });
    
    return result.readFF();
  }

  template< typename T, typename U >
  void writeXF(GlobalAddress<FullEmpty<T>> fe_addr, const U& val) {
    if (fe_addr.core() == mycore()) {
      DVLOG(2) << "local";
      fe_addr.pointer()->writeXF(val);
    } else {
      delegate::call(fe_addr, [val](FullEmpty<T> * fe){
          fe->writeXF(val);
          // DVLOG(2) << "writeXF(" << make_global(fe) << ", " << val << ") done";
        });
    }
  }
  
  template< typename T >
  T readXX(GlobalAddress<FullEmpty<T>> fe_addr) {
    if (fe_addr.core() == mycore()) {
      DVLOG(2) << "local";
      return fe_addr.pointer()->readXX();
    } else {
      return delegate::call(fe_addr, [] (FullEmpty<T> * fe) -> T {
          return fe->readXX();
        });
    }
  }
  
  template< typename T, typename F >
  struct Future {
    using Delegate = T (*)(Core,F);
    
    Core target;
    F apply; // local operation to get the value
    Delegate del;
    
    Future(Core target, F apply, Delegate del): target(target), apply(apply), del(del) {}
    
    T operator*() { del(target, apply); }
    
  };
  
  template< typename T, typename F >
  T basic_delegate(Core target, F apply) {
    if (mycore() == target) {
      return apply();
    } else {
      return delegate::call(target, [=]{ return apply(); });
    }
  }
  
  template< typename T, typename F,  >
  Future<T,F> basic_future(Core target, F apply) {
    return Future<T,F>(target, apply, &basic_delegate<T,F>);
  }
  
  namespace impl {
    
    template< typename A, typename B >
    struct FuncSum {
      A a;
      B b;
      
      FuncSum(const A& a, const B& b): a(a), b(b) {}
      
      FuncSum(A&& a, B&& b):
      a(std::forward<A>(a)),
      b(std::forward<B>(b)){}
      
      auto operator()() -> decltype(a()+b()) {
        return a() + b();
      }
    };
    
    template< typename T, typename A, typename B >
    struct FutureSum {
      Future<T,A> fa;
      Future<T,B> fb;
      
      FutureSum(const Future<T,A>& fa, const Future<T,B>& fb): fa(fa), fb(fb) {}
      
      T operator*() {
        auto a = fa.apply;
        auto b = fb.apply;

        if (fa.core() == fb.core()) {
          return delegate::call(fa.core(), [=]{
            return a() + b();
          });
        } else {
          FullEmpty<T> _r;
          auto r = make_global(&_r);
          send_message(fa.core(), [a,fb,r]{
            auto ar = a.apply();
            auto b = fb.apply;
            send_message(fb.core(), [ar,b,r]{
              auto sum = ar + b();
              send_message(r.core(), [r,sum]{
                r->writeXF(sum);
              });
            });
          });
          return _r.readFF();
        }
      }
      
    };
    
    template< typename T, typename A, typename B >
    auto operator+(Future<T,A>&& a, Future<T,B>&& b) -> Future<T>,FuncSum<A,B>> {
      if (a.target == b.target) {
        return Future<T,FuncSum<A,B>>(a.target, FuncSum<A,B>(a.apply, b.apply));
      } else {
        auto aa = a.apply;
        auto f = [=]{
          // on(a.target)
          aa();
        };
      }
    }
    
    // template< typename T >
    // struct Get {
    //   T* addr;
    //   Get(T* addr): addr(addr) {}
    //   T operator()() const { return *addr; }
    // };
    //
    // template< typename T >
    // Future<T,Get<T>> read(T* t) { return Future<T,Get<T>>(Get<T>(t)); }
    //
    // template< typename T >
    // struct ReadFF {
    //   GlobalAddress<FullEmpty<T>> fe;
    //   ReadFF(GlobalAddress<FullEmpty<T>> fe): fe(fe) {}
    //   T operator()() const {
    //     DCHECK_EQ(mycore(), fe.core());
    //     return fe->readFF();
    //   }
    // };
  }
  
  namespace future {
    
    template< typename T >
    Future<T,ReadFF<T>> readFF(GlobalAddress<FullEmpty<T>> fe_addr) {
      return basic_future(fe_addr.core(), ReadFF<T>(fe_addr));
    }
    
  }
  
  
  
  /// @}
}
