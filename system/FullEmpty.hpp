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

#include <tuple>
#include <type_traits>
using std::tuple;
using std::make_tuple;
using namespace Grappa;

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
        if (!fe.full()) return false;
        //VLOG(0) << __PRETTY_FUNCTION__ << ": suspended_delegate for " << &fe << "!";
        fill_remote(result_addr, fe.readFF());
        return true;
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
  
} // namespace Grappa


#ifdef GRAPPA_CXX14

  // template< typename T, typename F >
  // struct Future {
  //   using Delegate = T (*)(Core,F);
  //
  //   Core target;
  //   F apply; // local operation to get the value
  //   Delegate del;
  //
  //   Future(Core target, F apply, Delegate del): target(target), apply(apply), del(del) {}
  //
  //   // T operator*() { del(target, apply); }
  //
  //   T operator*() {
  //     if (mycore() == target) {
  //       return apply();
  //     } else {
  //       return delegate::call(target, [=]{ return apply(); });
  //     }
  //   }
  //
  // };
  
  template< class F >
  struct Delegate {
    Core target;
    F apply;
    Delegate(Core target, F apply): target(target), apply(apply) {}
    // auto operator()() const { return apply(); }
  };
  
  template<class F> auto make_delegate(Core c, F f) { return Delegate<F>(c,f); }
  
  // // declare Future class
  // template< class... Fs > struct Future;
  //
  // // base case
  // template< class F >
  // struct Future<F> {
  //   Delegate<F> d;
  //   Future(Delegate<F> d): d(d) {}
  //   auto operator*() { return delegate::call(d.target, [a=d.apply]{ return a(); }); }
  // };
  //
  // template< class F, class... Fs >
  // struct Future<F,Fs...> : private Future<Fs...> {
  //   Delegate<F> d;
  //   Future(Delegate<F> d, Delegate<Fs...> ds): Future<Fs...>(ds...), d(d) {}
  //
  //   auto operator*() {
  //     VLOG(0) << "@> combined!";
  //     return on(d.target)>>[apply=d.apply]{ return apply(); };
  //   }
  // };
  
  // need this `Result` wrapper type to distinguish between T as a value and T as a Delegate<U> (otherwise we have an ambiguous match)
  template<class T> struct Result { T val; };
  template<class T> Result<T> _r(T val) { return Result<T>{val}; }
  
  template< class T >
  constexpr auto _apply(Result<T> o) { return o.val; }

  template< class T, class F, class... Fs >
  constexpr auto _apply(Result<T> o, Delegate<F> d, Delegate<Fs>... ds) {
    return _apply(_r(d.apply(o.val)), ds...);
  }
  
  // helper for computing the final type if the output of each `Fs` is applied to the next
  template< class F, class... Fs >
  constexpr auto _apply(Delegate<F> d, Delegate<Fs>... ds) {
    return _apply(_r(d.apply()), ds...);
  }
  
  // template< class T, class F >
  // auto _apply(T o, Delegate<F> d) { return d(o); }
  
  // end base case (final step in expansion)
  template< class T, class U >
  constexpr auto _delegate(GlobalAddress<FullEmpty<T>> r, U o) {
    send_heap_message(r.core(), [r,o]{
      r->writeXF(o);
    });
  }
  
  // intermediate case
  template< class T, class U, class F, class... Fs>
  constexpr void _delegate(GlobalAddress<FullEmpty<T>> r, U o, Delegate<F> d, Delegate<Fs>... ds) {
    auto f = [r,o,apply=d.apply,ds...]{
      auto oo = apply(o);
      _delegate(r, oo, ds...);
    };
    if (mycore() == d.target) f();
    else send_heap_message(d.target, f);
  }
  
  // initial case (called by initial thread, blocks waiting for the final result)
  template< class F, class... Fs>
  constexpr auto _delegate(Delegate<F> d, Delegate<Fs>... ds) {
    // figure out the type that will be returned in the end
    using T = decltype(_apply(d,ds...));
    
    FullEmpty<T> _r; auto r = make_global(&_r);
    send_heap_message(d.target, [r,apply=d.apply,ds...]{
      auto o = apply();
      _delegate(r, o, ds...);
    });
    return _r.readFF();
  }
    
  // template< class T, class U, class F >
  // auto _delegate(GlobalAddress<FullEmpty<T>> r, U o, Delegate<F> d) {
  //   send_message(d.target, [r,o,apply=d.apply]{
  //     auto oo = apply(o);
  //     send_message(r.core(), [r,oo]{
  //       r->writeXF(oo);
  //     });
  //   });
  // }
  
  
  template<int ...> struct seq {};
  template<int N, int ...S> struct gens : gens<N-1, N-1, S...> {};
  template<int ...S> struct gens<0, S...>{ typedef seq<S...> type; };
  
  // template< class... Fs >
  // auto _delegate(tuple<Delegate<Fs...>> ds) {
  //   return _delegate(typename gens<sizeof...(Fs)>::type());
  // }
  
  template< class... Fs >
  struct Future {
    tuple<Delegate<Fs>...> ds;
    
    Future(Delegate<Fs>... ds): ds(make_tuple(ds...)) {}
    Future(tuple<Delegate<Fs>...>&& ds): ds(std::forward(ds)) {}
    Future(const tuple<Delegate<Fs>...>& ds): ds(ds) {}
    
    constexpr auto operator*() { return invoke(); }
    
    constexpr auto invoke() { return _invoke(typename gens<sizeof...(Fs)>::type()); }
            
    constexpr auto get_first() { return std::get<0>(ds); }
    constexpr auto get_rest() { return _cdr(ds); }
            
  private:
    
    template< class B, class...Bs > constexpr
    auto _cdr(tuple<Delegate<B>,Delegate<Bs>...>& t) {
      return _cdr(t, typename gens<sizeof...(Bs)>::type());
    }
    
    template< class B, class...Bs, int...I > constexpr
    auto _cdr(tuple<Delegate<B>,Delegate<Bs>...>& t, seq<I...> idx) {
      return tuple<Delegate<Bs>...>{ std::get<I+1>(t)... };
    }
    
    template< int...I > constexpr
    auto _invoke(seq<I...> idx) {
      return _delegate( std::get<I>(ds)... );
    }
  };
  
  template< class... Fs > constexpr
  Future<Fs...> make_future(Delegate<Fs>...ds) { return Future<Fs...>(ds...); }

  template< class... Fs > constexpr
  Future<Fs...> make_future(tuple<Delegate<Fs>...> t) { return Future<Fs...>(t); }

  template< class F > constexpr
  Future<F> make_future(Core c, F f) { return Future<F>(Delegate<F>(c,f)); }
  
  template< class...As, class...Bs > constexpr
  auto operator+(Future<As...>&& fa, Future<Bs...>&& fb) {
    using T = decltype(fa.invoke());
    auto b = fb.get_first();
    auto new_b = make_delegate(b.target, [ba=b.apply](const T& a){ return a + ba(); });
    return make_future(std::tuple_cat( fa.ds, make_tuple(new_b), fb.get_rest() ));
  }
  
  // template< class A, class B > constexpr
  // auto operator+(Future<A>&& fa, Future<B>&& fb) {
  //   using T = decltype(fa.invoke());
  //   auto a = fa.get_first();
  //   auto b = fb.get_first();
  //   return make_future(fa.target, [a,b,target=fb.target]{
  //     auto o = a();
  //     if (mycore() == target) {
  //       return o + b();
  //     } else {
  //
  //     }
  //   });
  //   auto new_b = make_delegate(b.target, [ba=b.apply](const T& a){ return a + ba(); });
  //   return make_future(std::tuple_cat( fa.ds, make_tuple(new_b), fb.get_rest() ));
  // }

  template< class...As, class...Bs >
  constexpr auto operator-(Future<As...>&& fa, Future<Bs...>&& fb) {
    using T = decltype(fa.invoke());
    auto b = fb.get_first();
    auto new_b = make_delegate(b.target, [ba=b.apply](const T& a){ return a - ba(); });
    return make_future(std::tuple_cat( fa.ds, make_tuple(new_b), fb.get_rest() ));
  }

  // template< typename A, typename B >
  // auto operator+(Future<A>&& a, Future<B>&& b) {
  //   // using T = decltype(a.d.apply() + b.d.apply());
  //   // FullEmpty<T> _r; auto r = make_global(&_r);
  //   // send_message(a.d.target, [a=a.d.apply,b,r]{
  //   //   auto ra = a();
  //   //   send_message(b.d.target, [ra,b=b.d.apply,r]{
  //   //     auto rr = ra + b();
  //   //     send_message(ra.core(),[r,rr]{
  //   //       r->writeXF(rr);
  //   //     });
  //   //   });
  //   // });
  //   // return r->readFF();
  //   using T = decltype(a.invoke());
  //   auto bb = [b=b.d.apply](const T& a) { return a + b(); };
  //
  //   return make_future(a.d, make_delegate(b.d.target, bb));
  // }
  //
  // template< typename A, typename B >
  // auto operator-(Future<A>&& a, Future<B>&& b) {
  //   using T = decltype(a.d.apply());
  //   auto bb = [b=b.d.apply](const T& a) { return a - b(); };
  //   return make_future(a.d, make_delegate(b.d.target, bb));
  // }
  //
  // template< typename T, typename F >
  // T basic_delegate(Core target, F apply) {
  //   if (mycore() == target) {
  //     return apply();
  //   } else {
  //     return delegate::call(target, [=]{ return apply(); });
  //   }
  // }
  
  // template< typename T, typename F,  >
  // Future<T,F> basic_future(Core target, F apply) {
  //   return Future<T,F>(target, apply, &basic_delegate<T,F>);
  // }
  
  // namespace impl {
  //
  //   template< typename A, typename B >
  //   struct FuncSum {
  //     A a;
  //     B b;
  //
  //     FuncSum(const A& a, const B& b): a(a), b(b) {}
  //
  //     FuncSum(A&& a, B&& b):
  //     a(std::forward<A>(a)),
  //     b(std::forward<B>(b)){}
  //
  //     auto operator()() -> decltype(a()+b()) {
  //       return a() + b();
  //     }
  //   };
  //
  //   template< typename T, typename A, typename B >
  //   struct FutureComposition {
  //     Future<T,A> fa;
  //     Future<T,B> fb;
  //
  //     FutureSum(const Future<T,A>& fa, const Future<T,B>& fb): fa(fa), fb(fb) {}
  //
  //     T operator*() {
  //       auto a = fa.apply;
  //       auto b = fb.apply;
  //
  //       if (fa.core() == fb.core()) {
  //         return delegate::call(fa.core(), [=]{
  //           return a() + b();
  //         });
  //       } else {
  //         FullEmpty<T> _r;
  //         auto r = make_global(&_r);
  //         send_message(fa.core(), [a,fb,r]{
  //           auto ar = a.apply();
  //           auto b = fb.apply;
  //           send_message(fb.core(), [ar,b,r]{
  //             auto sum = ar + b();
  //             send_message(r.core(), [r,sum]{
  //               r->writeXF(sum);
  //             });
  //           });
  //         });
  //         return _r.readFF();
  //       }
  //     }
  //
  //   };
  //
  //   template< typename T, typename A, typename B >
  //   auto operator+(Future<T,A>&& a, Future<T,B>&& b) -> Future<T>,FuncSum<A,B>> {
  //     if (a.target == b.target) {
  //       return Future<T,FuncSum<A,B>>(a.target, FuncSum<A,B>(a.apply, b.apply));
  //     } else {
  //       auto aa = a.apply;
  //       auto f = [=]{
  //         // on(a.target)
  //         aa();
  //       };
  //     }
  //   }
    
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
  // }
  
  namespace future {
    
    // template< typename T >
    // Future<T,ReadFF<T>> readFF(GlobalAddress<FullEmpty<T>> fe_addr) {
    //   return basic_future(fe_addr.core(), ReadFF<T>(fe_addr));
    // }
    
    template< class T >
    auto readFF(GlobalAddress<FullEmpty<T>> fe) {
      return make_future(fe.core(), [fe]{ return fe->readFF(); });
    }
    
  }
  
#endif // GRAPPA_CXX14
  
  /// @}
// }
