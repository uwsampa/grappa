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
#pragma once
#include <Collective.hpp>
#include <functional>

/// 
/// A Reducer object encapsulates a reduction
/// during the computation to a single location
///
/// Warning: may only use one object of type AllReducer<T, ReduceOp>
/// concurrently because relies on `Grappa::allreduce`,
/// which has no way of tagging messages
///
template <typename T, T (*ReduceOp)(const T&, const T&)>
class AllReducer {
  private:
    T * const localSum;
    const T * const init;
    bool finished;

  public:
    AllReducer(T initV) : init(new T(initV))
                       , localSum(new T) {}
    ~AllReducer() {
      delete init;
      delete localSum;
    }

    void reset() {
      finished = false;
      *localSum = *init;
    }

    void accumulate(T val) {
      *localSum = ReduceOp(*localSum, val);
    } 

    /// Finish the reduction and return the final value.
    /// This must not be called until all corresponding
    /// local reduction objects have been finished and
    /// synchronized externally.
    ///
    /// ALLCORES
    T finish() {
      if (!finished) {
        finished = true;
        //TODO init version and specialized version for non-template allowed
        *localSum = Grappa::allreduce<T,ReduceOp> (*localSum);
      }
      return *localSum;
    }


    //TODO, a non collective call to finish would be nice
    //any such scheme will require knowing where each participant's
    //value lives (e.g. process-global variable)
} GRAPPA_BLOCK_ALIGNED;

template <typename T, typename AllReducerType, typename CF>
T reduction(T init, CF f) {
  Core master = Grappa::mycore();
  auto r = Grappa::symmetric_global_alloc<AllReducerType>();
  Grappa::on_all_cores( [=] {
      // call the constructor 
      new (r.localize()) AllReducerType(init);
      r->reset();
  });
  
  // user code, calling accumulate
  f(r);

  T result;
  Grappa::on_all_cores( [master,r,&result] {
    T localcopy = r->finish();
    if (Grappa::mycore() == master) 
      result = localcopy;
  });

  // FIXME: memory leak on r; but free is problematic with symmetric_global_alloc
  // Grappa::global_free(r);

  return result;
}       

namespace Grappa {

  /// Symmetric object with no special built-in semantics, but several
  /// reduction-oriented operations defined.
  /// 
  /// Intended to be used as a building block for symmetric objects or
  /// when manual management of reductions on symmetric data.
  /// 
  /// This is closely related to Reducer, but rather than implicitly performing 
  /// reduction operations whenever the value is observed, SimpleSymmetric's 
  /// require one of the helper reducer operations to be applied manually. This
  /// flexibility could be useful for doing more than one kind of reduction on
  /// the same symmetric object.
  /// 
  /// Example usage for a global counter:
  /// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  /// SimpleSymmetric<int> global_x;
  /// 
  /// // ...
  /// Grappa::run([]{
  ///   GlobalAddress<int> A = /* initialize array A */;
  ///   
  ///   // increment counter locally 
  ///   forall(A, N, [](int& A_i){ if (A_i == 0) global_x++; });
  /// 
  ///   // compute reduction explicitly to get total
  ///   int total = sum(global_x);
  ///   
  /// });
  /// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  /// 
  /// Or to use it for global boolean checks:
  /// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  /// SimpleSymmetric<bool> x;
  /// 
  /// // ...
  /// Grappa::run([]{
  ///   GlobalAddress<int> A = /* initialize array A */;
  ///   
  ///   // initialize x on all cores to false
  ///   set(x, false);
  ///   
  ///   // find if all elements are > 0 (and's all symmetric bools together)
  ///   forall(A, N, [](int& A_i){ if (A_i > 0) x &= true; });
  ///   bool all_postive = all(x);
  ///   
  ///   // find if any are negative (or's all symmetric bools together)
  ///   set(x, false);
  ///   forall(A, N, [](int& A_i){ if (A_i < 0) x = true; });
  ///   bool any_negative = any(x);
  ///   
  /// });
  /// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  template< typename T >
  class SimpleSymmetric {
    T local_value;
    // GlobalAddress<SimpleSymmetric> self;
  public:
    SimpleSymmetric(): local_value() {}
  
    T& local() { return local_value; }
    const T& local() const { return local_value; }
  
    // static GlobalAddress<SimpleSymmetric> create() {
    //   auto s = symmetric_global_alloc<SimpleSymmetric>();
    //   call_on_all_cores([s]{
    //     s->self = s;
    //   });
    // }
    
    /// Reduction across symmetric object doing 'and' ('&')
    friend T all(SimpleSymmetric * r) {
      return reduce<T,collective_and>(&r->local_value);
    }
    
    /// Reduction across symmetric object doing 'or' ('|')
    friend T any(SimpleSymmetric * r) {
      return reduce<T,collective_or >(&r->local_value);
    }
    
    /// Reduction across symmetric object doing 'sum' ('+')
    friend T sum(SimpleSymmetric * r) {
      return reduce<T,collective_add>(&r->local_value);
    }
    
    /// Set instances on all cores to the given value.
    friend void set(SimpleSymmetric * r, const T& val) {
      call_on_all_cores([=]{ r->local_value = val; });
    }

    /// Reduction across symmetric object doing 'and' ('&')
    friend T all(SimpleSymmetric& r) { return all(&r); }
    
    /// Reduction across symmetric object doing 'or' ('|')
    friend T any(SimpleSymmetric& r) { return any(&r); }
    
    /// Reduction across symmetric object doing 'sum' ('+')
    friend T sum(SimpleSymmetric& r) { return sum(&r); }

    /// Set instances on all cores to the given value.
    friend void set(SimpleSymmetric& r, const T& val) { return set(&r, val); }
    
    /// Operate on local copy.
    T& operator&=(const T& val) { return local() &= val; }

    /// Operate on local copy.
    T& operator|=(const T& val) { return local() |= val; }

    /// Operate on local copy.
    T& operator+=(const T& val) { return local() += val; }

    /// Operate on local copy.
    T& operator++() { return ++local(); }

    /// Operate on local copy.
    T& operator++(int) { return local()++; }
    
  } GRAPPA_BLOCK_ALIGNED;
  
  /// Base class for Reducer implementing some operations common to all 
  /// specializations.
  template< typename T, T (*ReduceOp)(const T&, const T&) >
  class ReducerImpl {
  protected:
    T local_value;
  public:
    ReducerImpl(): local_value() {}
    
    /// Read out value; does expensive global reduce.
    /// 
    /// Called implicitly when the Reducer is used as the underlying type, 
    /// or by an explicit cast operation.
    operator T () const { return reduce<T,ReduceOp>(&this->local_value); }
    
    /// Globally set the value; expensive global synchronization.
    void operator=(const T& v){ reset(); local_value = v; }
    
    /// Globally reset to default value for the type.
    void reset() { call_on_all_cores([this]{ this->local_value = T(); }); }
  };
  
#define Super(...) \
  using Super = __VA_ARGS__; \
  using Super::operator=; \
  using Super::reset
  
  enum class ReducerType { Add, Or, And, Max, Min };
  
  /// Reducers are a special kind of *symmetric* object that, when read, 
  /// compute a reduction over all instances. They can be inexpensively updated
  /// (incremented/decremented, etc), so operations that modify the value
  /// without observing it are cheap and require no communication. However,
  /// reading the value forces a global reduction operation each time, 
  /// and directly setting the value forces global synchronization.
  /// 
  /// *Reducers must be declared in the C++ global scope* ("global variables")
  /// so they have identical pointers on each core. Declaring one as a local
  /// variable (on the stack), or in a symmetric allocation will lead to
  /// segfaults or worse.
  /// 
  /// By default, Reducers do a global "sum", using the `+` operator; other
  /// operations can be specified by ReducerType (second template parameter).
  /// 
  /// Example usage (count non-negative values in an array):
  /// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  /// // declare symmetric global object
  /// Reducer<int> non_negative_count;
  /// 
  /// int main(int argc, char* argv[]) {
  ///   Grappa::init(&argc, &argv);
  ///   Grappa::run([]{
  ///     GlobalAddress<double> A;
  ///     // allocate & initialize array A...
  /// 
  ///     // set global count to 0 (expensive global sync.)
  ///     non_negative_count = 0;
  /// 
  ///     forall(A, N, [](double& A_i){
  ///       if (A_i > 0) {
  ///         // cheap local increment
  ///         non_negative_count += 1;
  ///       }
  ///     });
  /// 
  ///     // read out total value and print it (expensive global reduction)
  ///     LOG(INFO) << non_negative_count << " non negative values";
  ///   });
  ///   Grappa::finalize();
  /// }
  /// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  template< typename T, ReducerType R >
  class Reducer : public ReducerImpl<T,collective_add> {};
  
  /// Reducer for sum (+), useful for global accumulators.
  /// Provides cheap operators for increment and decrement 
  /// (`++`, `+=`, `--`, `-=`).
  template< typename T >
  class Reducer<T,ReducerType::Add> : public ReducerImpl<T,collective_add> {
  public:
    Super(ReducerImpl<T,collective_add>);
    void operator+=(const T& v){ this->local_value += v; }
    void operator++(){ this->local_value++; }
    void operator++(int){ this->local_value++; }
    void operator-=(const T& v){ this->local_value -= v; }
    void operator--(){ this->local_value--; }
    void operator--(int){ this->local_value--; }
  };

  /// Reducer for "or" (`operator|`).
  /// Useful for "any" checks, such as "are any values non-zero?". 
  /// Provides cheap operator for "or-ing" something onto the value: `|=`.
  /// 
  /// Example:
  /// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  /// Reducer<bool,ReducerType::Or> any_nonzero;
  /// 
  /// // ... (somewhere in main task)
  /// any_nonzero = false;
  /// forall(A, N, [](double& A_i){
  ///   if (A_i != 0) {
  ///     any_nonzero |= true
  ///   }
  /// });
  /// LOG(INFO) << ( any_nonzero ? "some" : "no" ) << " nonzeroes.";
  /// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  template< typename T >
  class Reducer<T,ReducerType::Or> : public ReducerImpl<T,collective_or> {
  public:
    Super(ReducerImpl<T,collective_or>);
    void operator|=(const T& v){ this->local_value |= v; }
  };

  /// Reducer for "and" (`operator&`). 
  /// Useful for "all" checks, such as "are all values non-zero?".
  /// Provides cheap operator for "and-ing" something onto the value: `&=`.
  /// 
  /// Example:
  /// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  /// Reducer<bool,ReducerType::And> all_nonzero;
  /// 
  /// // ... (somewhere in main task)
  /// all_zero = true;
  /// forall(A, N, [](double& A_i){
  ///   all_zero &= (A_i == 0);
  /// });
  /// LOG(INFO) << ( all_zero ? "" : "not " ) << "all zero.";
  /// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  template< typename T >
  class Reducer<T,ReducerType::And> : public ReducerImpl<T,collective_and> {
  public:
    Super(ReducerImpl<T,collective_and>);
    void operator&=(const T& v){ this->local_value &= v; }
  };
  
  /// Reducer for finding the maximum of many values.
  /// Provides cheap operator (`<<`) for "inserting" potential max values.
  /// 
  /// Example:
  /// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  /// Reducer<double,ReducerType::Max> max_val;
  /// 
  /// // ... (somewhere in main task)
  /// max_val = 0.0;
  /// forall(A, N, [](double& A_i){
  ///   max_val << A_i;
  /// });
  /// LOG(INFO) << "maximum value: " << max_val;
  /// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  template< typename T >
  class Reducer<T,ReducerType::Max> : public ReducerImpl<T,collective_max> {
  public:
    Super(ReducerImpl<T,collective_max>);
    void operator<<(const T& v){
      if (v > this->local_value) {
        this->local_value = v;
      }
    }
  };
  
  /// Reducer for finding the minimum of many values.
  /// Provides cheap operator (`<<`) for "inserting" potential min values.
  /// 
  /// Example:
  /// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  /// Reducer<double,ReducerType::Min> min_val;
  /// 
  /// // ... (somewhere in main task)
  /// min_val = 0.0;
  /// forall(A, N, [](double& A_i){
  ///   min_val << A_i;
  /// });
  /// LOG(INFO) << "minimum value: " << min_val;
  /// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  template< typename T >
  class Reducer<T,ReducerType::Min> : public ReducerImpl<T,collective_min> {
  public:
    Super(ReducerImpl<T,collective_min>);
    void operator<<(const T& v){
      if (v < this->local_value) {
        this->local_value = v;
      }
    }
  };
  
#undef Super  
  
  /// Helper class for implementing reduceable tuples, where reduce is based on 
  /// just one of the two elements
  /// 
  /// Example usage to find the vertex with maximum degree in a graph (from SSSP app):
  /// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  /// using MaxDegree = CmpElement<VertexID,int64_t>;
  /// Reducer<MaxDegree,ReducerType::Max> max_degree;
  /// 
  /// ...
  /// GlobalAddress<G> g;
  /// // ...initialize graph structure...
  /// 
  /// // find max degree vertex
  /// forall(g, [](VertexID i, G::Vertex& v){
  ///   max_degree << MaxDegree(i, v.nadj);
  /// });
  /// root = static_cast<MaxDegree>(max_degree).idx();
  /// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  template< typename Id, typename Cmp >
  class CmpElement {
    Id i; Cmp c;
  public:
    CmpElement(): i(), c() {}
    CmpElement(Id i, Cmp c): i(i), c(c) {}
    Id idx() const { return i; }
    Cmp elem() const { return c; }
    bool operator<(const CmpElement& e) const { return elem() < e.elem(); }
    bool operator>(const CmpElement& e) const { return elem() > e.elem(); }
  };
  
} // namespace Grappa
