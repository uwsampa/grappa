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

#pragma once

#include <ostream>

namespace Grappa {
  namespace impl {
  
    class MetricBase {
    protected:
      const char * name;
      
    public:
      /// registers stat with global stats vector by default (reg_new=false to disable)
      MetricBase(const char * name, bool reg_new = true);
      
      /// prints as a JSON entry
      virtual std::ostream& json(std::ostream&) const = 0;
      
      /// reinitialize counter values, etc (so we can collect stats on specific region)
      virtual void reset() = 0;
      
      /// periodic sample (VTrace sampling triggered by GPerf stuff)
      virtual void sample() = 0;
      
      /// communicate with each cores' stat at the given pointer and aggregate them into
      /// this stat object
      virtual void merge_all(MetricBase* static_stat_ptr) = 0;
      
      /// create new copy of the class of the right instance (needed so we can create new copies of stats from their MetricBase pointer
      virtual MetricBase* clone() const = 0;
    };
    
  }
}
