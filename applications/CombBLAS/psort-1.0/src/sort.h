// The Funnelsort Project  -  Cache oblivious sorting algorithm implementation
// Copyright (C) 2005  Kristoffer Vinther
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

#ifndef SORT_H_INCLUDED__
#define SORT_H_INCLUDED__

namespace iosort
{

template<class Merger, class Splitter, class It, class OutIt>
inline void merge_sort(It begin, It end, OutIt dest, const typename Merger::allocator& alloc/* = typename Merger::allocator()*/);

template<class Merger, class Splitter, class It, class OutIt>
inline void merge_sort(It begin, It end, OutIt dest)
{
	merge_sort<Merger, Splitter>(begin, end, dest, typename Merger::allocator());
}

} // namespace iosort

#include "sort.timpl.h"
#endif // SORT_H_INCLUDED__
