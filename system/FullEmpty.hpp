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
  
  /// @}
}
