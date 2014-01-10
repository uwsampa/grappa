

// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
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
      
      DVLOG(2) << "setting up to block (" << fe_addr << ")";
      auto* c = SuspendedDelegate::create([&fe,result_addr]{
        VLOG(0) << "suspended_delegate!";
        fill_remote(result_addr, fe.readFF());
      });
      add_waiter(&fe, c);
    });
    
    return result.readFF();
  }

  template< typename T, typename U >
  void writeXF(GlobalAddress<FullEmpty<T>> fe_addr, const U& val) {
    delegate::call(fe_addr, [val](FullEmpty<T> * fe){
      fe->writeXF(val);
      // DVLOG(2) << "writeXF(" << make_global(fe) << ", " << val << ") done";
    });
  }
  
  /// @}
}
