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


#include <cassert>
#include <algorithm>
#include <utility>
#include <memory>
#include <iterator>
#include <stdexcept>

namespace iosort
{

namespace base_
{

enum tag_RECURSELIMIT { RECURSELIMIT = 24 };

template<class It, class Pred>
inline void insertion_sort(It begin, It end, Pred comp)
{
	for( It j=begin+1; j<end; j++ )
	{
		typename std::iterator_traits<It>::value_type e = *j;
		for( It i=j; begin<i; *i=e )
			if( comp(e,*(i-1)) )
				*i = *(i-1), --i;
			else
				break;
	}
}

template<typename T, class Pred>
inline T median(const T& a, const T& b, const T& c, Pred comp)
{
	if( comp(a,b) )
		if( comp(b,c) )
			return b;
		else if( comp(a,c) )
			return c;
		else
			return a;
	else
		if( comp(a,c) )
			return a;
		else if( comp(b,c) )
			return c;
		else
			return b;
}

template<class BidIt, class T, class Pred>
inline BidIt partition(BidIt begin, BidIt end, T pivot, Pred comp)
{
	for( ;; )
	{
		for( ; comp(*begin,pivot); ++begin );
		for( --end; comp(pivot,*end); --end );
		if( !(begin<end) )
			return begin;
		std::iter_swap(begin, end);
		++begin;
	}
}

template<class RanIt, class Pred>
inline void quicksort(RanIt First, RanIt Last, Pred comp)
{
	typename std::iterator_traits<RanIt>::difference_type Count = Last-First;
	while( RECURSELIMIT < Count )
	{
		RanIt part = partition(First, Last, median(*First,*(First+(Last-First)/2),*(Last-1),comp),comp);
		if( part-First < Last-part ) // loop on larger half
			quicksort(First, part, comp), First = part;
		else
			quicksort(part, Last, comp), Last = part;
		Count = Last-First;
	}
	if( Count > 1 )
        insertion_sort(First, Last, comp);
}

template<class Int>
inline Int logc(Int k)
{
	height_t h = 0;
	for( order_t i=k-1; i; h++, i/=2 );
	return h;
}

template<class RanIt, class Pred>
void batcher_sort(RanIt begin, RanIt end, Pred comp)
{
	typedef typename std::iterator_traits<RanIt>::difference_type ItDiff;
	ItDiff t = logc(end-begin)-1;
	ItDiff p = 1<<t;
	for( ; p!=1; p=p/2 )
	{
		RanIt i, j;
		for( i=begin; i<end-p-p; i+=p )
		{
			j = i+p;
			for( ; i<j; i++ )
				if( comp(*(i+p),*i) )
					std::iter_swap(i+p, i);
		}
		for( ; i<end-p; i++ )
			if( comp(*(i+p),*i) )
				std::iter_swap(i+p, i);
		ItDiff q = 1<<t;
		while( q != p )
		{
			ItDiff d=q-p;
			q = q/2;
			for( i=begin+p; i<end-d-p; i+=p )
			{
				j = i+p;
				for( ; i<j; i++ )
					if( comp(*(i+d),*i) )
						std::iter_swap(i+d, i);
			}
			for( ; i<end-d; i++ )
				if( comp(*(i+d),*i) )
					std::iter_swap(i+d, i);
		}
	}
	if( p )
	{
		for( RanIt i=begin; i<end-1; i+=2 )
			if( comp(*(i+1),*i) )
				std::iter_swap(i+1, i);
		ItDiff q = 1<<t;
		while( q != 1 )
		{
			ItDiff d=q-1;
			q = q/2;
			for( RanIt i=begin+1; i<end-d; i+=2 )
				if( comp(*(i+d),*i) )
					std::iter_swap(i+d, i);
		}
	}
}

template<class RanIt, class Pred>
void inplace_base_sort(RanIt begin, RanIt end, Pred comp)
{
#ifdef __ia64__
	batcher_sort(begin, end, comp);
#else // __ia64__
	quicksort(begin, end, comp);
#endif // __ia64__
}

} // namespace base_

template<class RanIt, class Splitter>
class stream_slices
{
	typedef typename std::iterator_traits<RanIt>::difference_type diff;
public:
	class iterator : public std::iterator<std::bidirectional_iterator_tag, std::pair<RanIt,RanIt>, diff>
	{
		friend class stream_slices;
	public:
		inline iterator& operator++()
		{
			begin = end;
			end = end+run_size;
			if( --big_runs == 0 )
				--run_size;
			return *this;
		}
		inline iterator& operator--()
		{
			end = begin;
			begin = end-run_size;
			if( ++big_runs == 0 )
				++run_size;
			return *this;
		}
		std::pair<RanIt,RanIt> operator*()
			{ return make_pair(begin,end); }
		bool operator==(const iterator& rhs)
			{ return begin == rhs.begin; }
	private:
		inline iterator(RanIt begin, diff run_size, size_t big_runs) : begin(begin), run_size(run_size), big_runs(big_runs)
		{
			if( big_runs )
				++run_size;
			end = begin+run_size;
		}
		RanIt begin, end;
		diff run_size;
		size_t big_runs;
	};
	stream_slices(RanIt begin, RanIt end) : b(begin), e(end), order_(Splitter::prefered_order_output(end-begin))
	{
		if( order_ < 2 )
			throw std::logic_error("Splitter::prefered_order_output returned less than two");
		run_size = (end-begin)/order_;
		size_t rem = static_cast<size_t>((end-begin) % order_);
		run_size += rem/order_;
		big_runs = rem % order_;
	}
	inline diff order() const
		{ return order_; }
	inline iterator begin()
		{ return iterator(b, run_size, big_runs); }
	inline iterator end()
		{ return iterator(e, run_size, big_runs); }
private:
	RanIt b, e;
	diff run_size;
	size_t order_, big_runs;
};

template<class Splitter, class Diff>
inline Diff run_size_(Diff d)
{
	size_t order = Splitter::prefered_order_output(d);
	Diff run_size = d/order;
	size_t rem = static_cast<size_t>(d % order);
	run_size += rem/order;
	size_t big_runs = rem % order;
	if( big_runs )
		run_size++;
	return run_size;
}

template<class Merger, class Splitter, class Diff, class MAlloc, class TAlloc>
inline std::pair<Merger*,Merger*> build_cache_(Diff run_size, MAlloc& malloc, TAlloc& talloc)
{
	Merger *begin_cache, *end_cache;
	int rec_calls = 0;
	for( Diff d=run_size; d>static_cast<Diff>(Splitter::switch_to_cache_aware()); d=run_size_<Splitter>(d), ++rec_calls );
	begin_cache = end_cache = malloc.allocate(rec_calls);
	for( Diff d=run_size; d>static_cast<Diff>(Splitter::switch_to_cache_aware()); d=run_size_<Splitter>(d), ++end_cache )
		new(end_cache) Merger(Splitter::prefered_order_output(d), talloc);
	return make_pair(begin_cache,end_cache);
}

template<class Merger, class Splitter, class It, class OutIt, class Alloc>
void merge_sort_(It begin, It end, OutIt dest, Merger* cache, Alloc alloc);

template<class Merger, class Splitter, class MPtr, class It, class Alloc>
inline void sort_streams_(MPtr cache, It begin, size_t order, size_t run_size, size_t big_runs, Alloc alloc)
{
	typedef typename std::iterator_traits<It>::value_type T;
	typename Alloc::template rebind<T>::other talloc(alloc);
	typedef typename Alloc::template rebind<T>::other::pointer TPtr;
	TPtr tmp = talloc.allocate((size_t)(run_size));
	It last = begin;
	size_t i;
	if( big_runs )
	{
		merge_sort_<Merger,Splitter>(last, last+run_size, std::raw_storage_iterator<TPtr,T>(tmp), cache, alloc);
		for( i=1, last+=run_size; i!=big_runs; i++, last+=run_size )
			merge_sort_<Merger,Splitter>(last, last+run_size, last-run_size, cache, alloc);
		run_size--;
		for( ; i!=order; i++, last+=run_size )
			merge_sort_<Merger,Splitter>(last, last+run_size, last-run_size-1, cache, alloc);
		run_size++;
	}
	else
	{
		merge_sort_<Merger,Splitter>(last, last+run_size, tmp, cache, alloc);
		for( --order, last+=run_size; order; --order, last+=run_size )
			merge_sort_<Merger,Splitter>(last, last+run_size, last-run_size, cache, alloc);
	}
	std::copy(tmp, tmp+run_size, last-run_size);
	for( TPtr p=tmp+run_size-1; p>=tmp; --p )
		talloc.destroy(p);
	talloc.deallocate(tmp,static_cast<size_t>(run_size));
}

template<class MPtr, class It>
inline void add_sorted_streams_(MPtr merger, It end, size_t order, size_t run_size, size_t big_runs)
{
	if( big_runs )
	{
		merger->add_stream(end-run_size, end);
		end -= run_size;
		--run_size, --big_runs;
		for( --order; order!=big_runs; --order, end-=run_size )
			merger->add_stream(end-run_size, end);
		++run_size;
		for( ; order; --order, end-=run_size )
			merger->add_stream(end-run_size, end);
	}
	else
		for( ; order; --order, end-=run_size )
			merger->add_stream(end-run_size, end);
}

template<class MPtr, class It, class Pred>
inline void add_streams_(MPtr merger, It begin, size_t order, size_t run_size, size_t big_runs, Pred comp)
{
	size_t i;
	for( i=0; i!=order-big_runs; ++i, begin+=run_size )
		base_::inplace_base_sort(begin, begin+run_size, comp);
	++run_size;
	for( ; i!=order; ++i, begin+=run_size )
		base_::inplace_base_sort(begin, begin+run_size, comp);

	for( ; i!=order-big_runs; --i, begin-=run_size )
		merger->add_stream(begin-run_size, begin);
	--run_size;
	for( ; i; --i, begin-=run_size )
		merger->add_stream(begin-run_size, begin);
}

template<class Merger, class Splitter, class It, class OutIt, class Alloc>
void merge_sort_(It begin, It end, OutIt dest, Merger* cache, Alloc alloc)
{
	typedef typename std::iterator_traits<It>::value_type T;
	typedef typename std::iterator_traits<It>::difference_type ItDiff;
	typedef typename Merger::stream Stream;

	order_t order = Splitter::prefered_order_output(end-begin);
	if( order < 2 )
		throw std::logic_error("Splitter::prefered_order_output returned less than two");
	ItDiff run_size = (end-begin)/order;
	size_t rem = (size_t)((end-begin) % order);
	run_size += rem/order;
	size_t big_runs = rem % order;

	cache->reset();
	if( run_size > static_cast<ItDiff>(Splitter::switch_to_cache_aware()) )
	{
		if( big_runs )
			run_size++;
		sort_streams_<Merger,Splitter>(cache+1,begin,order,run_size,big_runs,alloc);
		add_sorted_streams_(cache,end,order,run_size,big_runs);
	}
	else
		add_streams_(cache,begin,order,run_size,big_runs, typename Merger::predicate());
	(*cache)(dest);
}

template<class Merger, class Splitter, class It, class OutIt>
inline void merge_sort(It begin, It end, OutIt dest, const typename Merger::allocator& alloc)
{
	typedef typename std::iterator_traits<It>::difference_type ItDiff;
	if( end-begin > static_cast<ItDiff>(Splitter::switch_to_cache_aware()) )
	{
		order_t order = Splitter::prefered_order_output(end-begin);
		if( order < 2 )
			throw std::logic_error("Splitter::prefered_order_output returned less than two");
		ItDiff run_size = (end-begin)/order;
		size_t rem = (size_t)((end-begin) % order);
		run_size += rem/order;
		size_t big_runs = rem % order;
		if( run_size > static_cast<ItDiff>(Splitter::switch_to_cache_aware()) )
		{
			if( big_runs )
				run_size++;
			typename Merger::allocator::template rebind<Merger>::other malloc(alloc);
			std::pair<Merger*,Merger*> cache = build_cache_<Merger,Splitter>(run_size, malloc, alloc);
			sort_streams_<Merger,Splitter>(cache.first, begin, order, run_size, big_runs, alloc);
			for( Merger *pm=cache.second-1; pm>=cache.first; --pm )
				malloc.destroy(pm);
			malloc.deallocate(cache.first, cache.second-cache.first);
		}
		Merger merger(order,alloc);
		if( run_size > static_cast<ItDiff>(Splitter::switch_to_cache_aware()) )
			add_sorted_streams_(&merger, end, order, run_size, big_runs);
		else
			add_streams_(&merger, begin, order, run_size, big_runs, typename Merger::predicate());
#ifdef _DEBUG
		OutIt e = merger(dest, dest+(end-begin));
		merger.empty(dest, dest+(end-begin));
		size_t N = 0;
		for( typename Merger::stream_iterator i=merger.begin(); i!=merger.end(); ++i )
			N += (*i).end()-(*i).begin();
		assert( e == dest+(end-begin) );
#else // _DEBUG
		merger(dest);
#endif // _DEBUG
	}
	else
	{
		base_::inplace_base_sort(begin, end, typename Merger::predicate());
		std::copy(begin, end, dest);
	}
}
} // namespace iosort
