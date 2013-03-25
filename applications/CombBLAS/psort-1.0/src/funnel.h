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

#ifndef FUNNEL_H_INCLUDED__
#define FUNNEL_H_INCLUDED__

#include <cassert>
#include <memory>
#include <utility>
#include <iterator>
#include <functional>
#include <stdexcept>
#include <cmath>

namespace iosort
{

typedef unsigned short height_t;
typedef unsigned short basic_order_t;
typedef unsigned int order_t;

template<int order>
inline height_t logc(order_t k)
{
	height_t h = 0;
	for( order_t i=k-1; i; h++, i/=order );
	return h;
}
/* Computes floor(log()), not ceil(log()) :(  - Also beware of little- and bigendianness(?)
template<>
inline height_t logf<2>(order_t k)
{
	float f = static_cast<float>(k);
	return static_cast<height_t>(((*reinterpret_cast<int*>(&f)) >> 23) - 127);
}*/
template<int order>
inline order_t pow_of_order(height_t h)
{
	order_t k = 1;
	for( ; h; h--, k*=order );
	return k;
}
template<>
inline order_t pow_of_order<2>(height_t h)
	{ return 1<<h; }
template<>
inline order_t pow_of_order<4>(height_t h)
	{ return 1<<(2*h); }
template<>
inline order_t pow_of_order<8>(height_t h)
	{ return 1<<(3*h); }
template<>
inline order_t pow_of_order<16>(height_t h)
	{ return 1<<(4*h); }

template<int order>
class bfs_index
{
public:
	inline bfs_index() : i(1) { }
	inline operator order_t() const
		{ return i; }
	inline bfs_index<order> operator=(const bfs_index<order>& rhs)
		{ i = rhs.i; return *this; }
	inline bool operator==(const bfs_index<order>& rhs) const
		{ return i == rhs.i; }
	inline bool operator!=(const bfs_index<order>& rhs) const
		{ return i != rhs.i; }
    inline void child(basic_order_t ch)
		{ i = static_cast<order_t>(order*(i-1)+ch+2); }
    inline void parent()
		{ /*assert( i>1 );*/ i = static_cast<order_t>((i-2)/order+1); }
	inline basic_order_t child_index() const
		{ /*assert( i>1 );*/ return static_cast<basic_order_t>((i+order-2) % order); }
private:
	order_t i;
};

template<int order=2>
class default_splitter
{
	static const int alpha = 16;
public:
	template<class diff_type>
	static inline order_t prefered_order_output(diff_type N)
		{ return static_cast<order_t>(std::sqrt(static_cast<double>(N)/alpha)); }
	template<class diff_type>
	static inline order_t prefered_order_input(diff_type N)
		{ assert( false ); return static_cast<order_t>(std::sqrt(static_cast<double>(N)/alpha)); }
	static inline size_t switch_to_cache_aware()
		{ return static_cast<size_t>(alpha*(order+1)*(order+1)); }
	static inline height_t split(height_t h)
		{ return h/2; }
	static inline size_t out_buffer_size(order_t k)
		{ return static_cast<size_t>(alpha*k*k); }
};

template<class FwIt> struct nop_refill { template<class Merger> inline bool operator()(const Merger*, FwIt, FwIt)
	{ return false; } };


template<class,int,class,class,class,class>
class special_ { };

template<class RanIt, int Order=2, class Splitter = default_splitter<Order>, class Pred = std::less<typename std::iterator_traits<RanIt>::value_type>, class Refiller = nop_refill<RanIt>, class Alloc = std::allocator<typename std::iterator_traits<RanIt>::value_type> >
class merge_tree
{
	friend class special_<RanIt,Order,Splitter,Pred,Refiller,Alloc>;
public:
	typedef unsigned int order_t;
	enum order_tag_ { order = Order };
	enum mmth_tag_ { MAX_MERGE_TREE_HEIGHT = 16 };
	typedef Splitter splitter;
	typedef typename std::iterator_traits<RanIt>::value_type value_type;
	typedef Alloc allocator;
	typedef RanIt iterator;
	typedef Pred predicate;
	template<class NewRefiller>
	struct rebind { typedef merge_tree<iterator, order, splitter, predicate, NewRefiller, allocator> other; };
private:
	struct Node;
	friend struct Node;
	typedef typename allocator::pointer TPtr;
	typedef typename allocator::size_type size_type;
	typedef typename allocator::template rebind<Node>::other nallocator;
	typedef typename nallocator::pointer NPtr;
	struct Node
	{
		struct Edge
		{
			typedef typename allocator::pointer TPtr;
			typedef typename allocator::template rebind<Node>::other nallocator;
			typedef typename nallocator::pointer NPtr;
			inline void construct(order_t k, height_t height, size_t *buf_size, allocator alloc, nallocator nalloc);
			inline void destroy(allocator alloc, nallocator nalloc);
			typename allocator::template rebind<Node>::other::pointer child;
			typename allocator::pointer head, tail, begin, end;
		private:
			static inline void fill_dflt(TPtr b, TPtr e, allocator alloc);
		} edges[order];
		inline void construct(order_t k, height_t height, size_t *buf_size, allocator alloc, nallocator nalloc);
		inline void destroy(allocator alloc, nallocator nalloc);
	};
	typedef std::pair<RanIt,RanIt> stream_t;
	typedef typename allocator::template rebind<stream_t>::other sallocator;
	typedef typename sallocator::pointer SPtr;
public:
	inline merge_tree(order_t k) : k(k), cold(true)
		{ construct(k); }
	inline merge_tree(order_t k, const allocator& alloc) : k(k), cold(true), alloc(alloc), salloc(alloc), nalloc(alloc)
		{ construct(k); }
	~merge_tree();

	static inline order_t min_order()
		{ return order; }

	class stream
	{
	private:
		SPtr pair;
	public:
		stream(SPtr pair) : pair(pair) { }
		inline RanIt& begin()
			{ return pair->first; }
		inline RanIt& end()
			{ return pair->second; }
	};
	class stream_iterator
	{
	private:
		SPtr ptr;
	public:
		stream_iterator(SPtr ptr) : ptr(ptr) { }
		inline order_t operator-(const stream_iterator& rhs)
			{ return static_cast<order_t>(ptr-rhs.ptr); }
		inline stream operator*()
			{ return stream(ptr); }
		inline stream_iterator& operator++()
			{ ++ptr; return *this; }
		inline stream_iterator& operator++(int)
		{
			stream_iterator ret = stream_iterator(ptr);
			++ptr;
			return ret;
		}
		inline bool operator==(const stream_iterator& rhs)
			{ return ptr == rhs.ptr; }
		inline bool operator!=(const stream_iterator& rhs)
			{ return ptr != rhs.ptr; }
	};

	inline void add_stream(RanIt begin, RanIt end)
		{ *last_stream++ = std::make_pair(begin,end); }
	inline stream_iterator begin()
		{ return stream_iterator(input_streams); }
	inline stream_iterator end()
		{ return stream_iterator(last_stream); }
	inline void reset();

	inline void set_refiller(const Refiller& r)
		{ refiller = r; }
	inline const Refiller& get_refiller() const
		{ return refiller; }
	template<class FwIt>
	inline FwIt empty(FwIt begin, FwIt end)
		{ return empty_buffers(root, begin, end); }
	template<class OutIt>
	inline OutIt empty(OutIt begin)
		{ return empty_buffers(root, begin); }

	template<class It>
	inline It operator()(It begin, It end);
	template<class OutIt>
	inline OutIt operator()(OutIt begin);

private:
	template<class FwIt>
	static inline FwIt empty_buffers(NPtr n, FwIt begin, FwIt end);
	template<class OutIt>
	static inline OutIt empty_buffers(NPtr n, OutIt begin);

	static void compute_buffer_sizes(size_type *b, size_type *e);
	void construct(order_t k);

	inline void go_down(typename Node::Edge& e, bfs_index<order> index);
	inline void go_down_cold(typename Node::Edge& e, bfs_index<order>& index);

	void warmup(NPtr n, typename Node::Edge& output, bfs_index<order> index);

	template<class FwIt>
	inline FwIt copy_root(typename Node::Edge& de, bfs_index<order> index, FwIt begin, FwIt end, std::forward_iterator_tag);
	template<class RIt>
	inline RIt copy_root(typename Node::Edge& de, bfs_index<order> index, RIt begin, RIt end, std::random_access_iterator_tag);
	template<class OutIt>
	inline OutIt copy_root(typename Node::Edge& de, bfs_index<order> index, OutIt begin);

	inline TPtr copy(typename Node::Edge& de, bfs_index<order> index, TPtr b, TPtr e);

	template<class FwIt>
	inline FwIt fill_root(FwIt begin, FwIt end)
		{ return special_<RanIt,order,Splitter,Pred,Refiller,Alloc>::fill_root(this, begin, end); }
	template<class OutIt>
	inline OutIt fill_root(OutIt begin)
		{ return special_<RanIt,order,Splitter,Pred,Refiller,Alloc>::fill_root(this, begin); }
	void fill(NPtr n, typename Node::Edge& output, bfs_index<order> index)
		{ special_<RanIt,order,Splitter,Pred,Refiller,Alloc>::fill(this, n, output, index); }
	void fill_leaf(typename Node::Edge& output, bfs_index<order> index)
		{ special_<RanIt,order,Splitter,Pred,Refiller,Alloc>::fill_leaf(this, output, index); }

private:
	order_t k;
	order_t leaf_index;
	bool cold;
	NPtr root;
	SPtr input_streams, last_stream;
	Pred comp;
	allocator alloc;
	sallocator salloc;
	nallocator nalloc;
	Refiller refiller;
};



/*****
 *  Alloc::pointer iterator template specialization
 */

template<int Order, class Splitter, class Pred, class Refiller, class Alloc>
class merge_tree<typename Alloc::pointer, Order, Splitter, Pred, Refiller, Alloc>
{
	typedef typename Alloc::pointer RanIt;
	friend class special_<typename Alloc::pointer,Order,Splitter,Pred,Refiller,Alloc>;
public:
	typedef unsigned int order_t;
	enum order_tag_ { order = Order };
	enum mmth_tag_ { MAX_MERGE_TREE_HEIGHT = 16 };
	typedef Splitter splitter;
	typedef typename std::iterator_traits<typename Alloc::pointer>::value_type value_type;
	typedef Alloc allocator;
	typedef typename Alloc::pointer iterator;
	typedef Pred predicate;
//	template<class NewRefiller>
//	struct rebind { typedef merge_tree<typename Alloc::pointer, Order, Splitter, Pred, NewRefiller, Alloc> other; };
private:
	struct Node;
	friend struct Node;
	typedef typename allocator::pointer TPtr;
	typedef typename allocator::size_type size_type;
	typedef typename allocator::template rebind<Node>::other nallocator;
	typedef typename nallocator::pointer NPtr;
	struct Node
	{
		struct Edge
		{
			typedef typename allocator::pointer TPtr;
			typedef typename allocator::template rebind<Node>::other nallocator;
			typedef typename nallocator::pointer NPtr;
			inline Edge** construct(order_t k, height_t height, size_t *buf_size, Edge** edge_list, allocator alloc, nallocator nalloc);
			inline void destroy(allocator alloc, nallocator nalloc);
			typename nallocator::pointer child;
			typename allocator::pointer head, tail, begin, end;
		private:
			static inline void fill_dflt(TPtr b, TPtr e, allocator alloc);
		} edges[order];
		inline Edge** construct(order_t k, height_t height, size_t *buf_size, Edge** edge_list, allocator alloc, nallocator nalloc);
		inline void destroy(allocator alloc, nallocator nalloc);
	};
	typedef typename Node::Edge* stream_t;
	typedef typename allocator::template rebind<stream_t>::other sallocator;
	typedef typename sallocator::pointer SPtr;
public:
	inline merge_tree(order_t k) : k(k), cold(true)
		{ construct(k); }
	inline merge_tree(order_t k, const allocator& alloc) : k(k), cold(true), alloc(alloc), salloc(alloc), nalloc(alloc)
		{ construct(k); }
	~merge_tree();

	static inline order_t min_order()
		{ return order; }

	class stream
	{
	private:
		stream_t edge;
	public:
		inline stream(stream_t edge) : edge(edge) { }
		inline typename allocator::pointer& begin()
			{ return edge->head; }
		inline typename allocator::pointer& end()
			{ return edge->tail; }
	};
	class stream_iterator
	{
	private:
		SPtr ptr;
	public:
		inline stream_iterator(SPtr ptr) : ptr(ptr) { }
		inline order_t operator-(const stream_iterator& rhs)
			{ return static_cast<order_t>(ptr-rhs.ptr); }
		inline stream operator*()
			{ return stream(*ptr); }
		inline stream_iterator& operator++()
			{ ++ptr; return *this; }
		inline stream_iterator& operator++(int)
		{
			stream_iterator ret = stream_iterator(ptr);
			++ptr;
			return ret;
		}
		inline bool operator==(const stream_iterator& rhs)
			{ return ptr == rhs.ptr; }
		inline bool operator!=(const stream_iterator& rhs)
			{ return ptr != rhs.ptr; }
	};

	inline void add_stream(typename Alloc::pointer begin, typename Alloc::pointer end)
		{ (*last_stream)->head = begin, (*last_stream)->tail = end, ++last_stream; }
	inline stream_iterator begin()
		{ return input_streams; }
	inline stream_iterator end()
		{ return last_stream; }
	inline void reset();

	inline void set_refiller(const Refiller& r)
		{ refiller = r; }
	inline const Refiller& get_refiller() const
		{ return refiller; }
	template<class FwIt>
	inline FwIt empty(FwIt begin, FwIt end)
		{ return empty_buffers(root, begin, end); }
	template<class OutIt>
	inline OutIt empty(OutIt begin)
		{ return empty_buffers(root, begin); }

	template<class FwIt>
	inline FwIt operator()(FwIt begin, FwIt end);
	template<class OutIt>
	inline OutIt operator()(OutIt begin);
private:
	template<class FwIt>
	static inline FwIt empty_buffers(NPtr n, FwIt begin, FwIt end);
	template<class OutIt>
	static inline OutIt empty_buffers(NPtr n, OutIt begin);

	void construct(order_t k);
	static void compute_buffer_sizes(size_type *b, size_type *e);

	static inline void go_down(typename Node::Edge& e, Pred comp);
	static inline void go_down_cold(typename Node::Edge& e, Pred comp);

	static void warmup(NPtr n, typename Node::Edge& output, Pred comp);

	template<class OutIt>
	static inline OutIt copy(typename Node::Edge& de, OutIt begin, Pred comp);
	template<class FwIt>
	static inline FwIt copy(typename Node::Edge& de, FwIt begin, FwIt end, Pred comp, std::forward_iterator_tag);
	template<class RIt>
	static inline RIt copy(typename Node::Edge& de, RIt begin, RIt end, Pred comp, std::random_access_iterator_tag);

	template<class OutIt>
	static inline OutIt fill(NPtr n, OutIt begin, Pred comp)
		{ return special_<typename Alloc::pointer,order,Splitter,Pred,Refiller,Alloc>::fill(n, begin, comp); }
	template<class FwIt>
	static inline FwIt fill(NPtr n, FwIt begin, FwIt end, Pred comp)
		{ return special_<typename Alloc::pointer,Order,Splitter,Pred,Refiller,Alloc>::fill(n, begin, end, comp); }
private:
	order_t k;
	order_t leaf_index;
	bool cold;
	NPtr root;
	SPtr input_streams, last_stream;
	Pred comp;
	allocator alloc;
	sallocator salloc;
	nallocator nalloc;
	Refiller refiller;
};

} // namespace iosort

#include "funnel.timpl.h"
#endif // FUNNEL_H_INCLUDED__
