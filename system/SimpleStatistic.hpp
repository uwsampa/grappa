
#pragma once

#include "StatisticBase.hpp"

#include "Grappa.hpp"
#include "Addressing.hpp"
#include "Message.hpp"
#include "CompletionEvent.hpp"

namespace Grappa {

  template<typename T>
  class SimpleStatistic : public StatisticBase {
  protected:
    
    T value;
    
#ifdef VTRACE_SAMPLED
    unsigned vt_counter;
    static const int vt_type;
    
    inline void vt_sample() const;
#endif
    
  public:
    
    SimpleStatistic(const char * name, T initial_value, bool reg_new = true):
        value(initial_value), StatisticBase(name, reg_new) {
#ifdef VTRACE_SAMPLED
        if (SimpleStatistic::vt_type == -1) {
          LOG(ERROR) << "warning: VTrace sampling unsupported for this type of SimpleStatistic.";
        } else {
          vt_counter = VT_COUNT_DEF(name, name, SimpleStatistic::vt_type, VT_COUNT_DEFGROUP);
        }
#endif
    }
    
    virtual std::ostream& json(std::ostream& o) const {
      o << '"' << name << "\": \"" << value << '"';
      return o;
    }
    
    virtual void sample() const {
#ifdef VTRACE_SAMPLED
      // vt_sample() specialized for supported tracing types in Statistics.cpp
      vt_sample();
#endif
    }
    
    virtual SimpleStatistic<T>* clone() const {
      // (note: must do `reg_new`=false so we don't re-register this stat)
      return new SimpleStatistic<T>(name, value, false);
    }
    
    virtual void merge_all(StatisticBase* static_stat_ptr) {
      this->value = 0;
      
      // TODO: use more generalized `reduce` operation to merge all
      SimpleStatistic<T>* this_static = reinterpret_cast<SimpleStatistic<T>*>(static_stat_ptr);
      
      GlobalAddress<SimpleStatistic<T>> combined_addr = make_global(this);
      
      CompletionEvent ce(Grappa::cores());
      
      for (Core c = 0; c < Grappa::cores(); c++) {
        // we can compute the GlobalAddress here because we have pointers to globals,
        // which are guaranteed to be the same on all nodes
        GlobalAddress<SimpleStatistic<T>> remote_stat = make_global(this_static, c);
        
        send_heap_message(c, [remote_stat, combined_addr, &ce] {
          SimpleStatistic<T>* s = remote_stat.pointer();
          T s_value = s->value;
          
          send_heap_message(combined_addr.node(), [combined_addr, s_value, &ce] {
            // for this simple SimpleStatistic, merging is as simple as accumulating the value
            SimpleStatistic<T>* combined_ptr = combined_addr.pointer();
            combined_ptr->value += s_value;
            
            ce.complete();
          });
        });
      }
      ce.wait();
    }
    
    inline const SimpleStatistic<T>& count() { return (*this)++; }
    
    // <sugar>
    template<typename U>
    inline const SimpleStatistic<T>& operator+=(U increment) {
      value += increment;
      return *this;
    }
    template<typename U>
    inline const SimpleStatistic<T>& operator-=(U decrement) {
      value -= decrement;
      return *this;
    }
    inline const SimpleStatistic<T>& operator++() { return *this += 1; }
    inline const SimpleStatistic<T>& operator--() { return *this += -1; }
    inline T operator++(int) { *this += 1; return value; }
    inline T operator--(int) { *this += -1; return value; }
    
    // allow casting as just the value
    inline operator T() const { return value; }
    
    inline SimpleStatistic& operator=(T value) {
      this->value = value;
      return *this;
    }
    // </sugar>
  };
  
} // namespace Grappa