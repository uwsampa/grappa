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


namespace iosort
{

template<class RanIt, int order, class Splitter, class Pred, class Refiller, class Alloc>
void merge_tree<RanIt,order,Splitter,Pred,Refiller,Alloc>::Node::Edge::fill_dflt(TPtr b, TPtr e, allocator alloc)
{
	TPtr p = b;
	try
	{
		value_type dflt;
		for( ; p!=e; ++p )
			alloc.construct(p,dflt);
	}
	catch( ... )
	{
		if( p != b )
		{
			for( --p; p!=b; --p )
				alloc.destroy(p);
			alloc.destroy(p);
		}
		alloc.deallocate(b,e-b);
		throw;
	}
}

template<class RanIt, int order, class Splitter, class Pred, class Refiller, class Alloc>
void merge_tree<RanIt,order,Splitter,Pred,Refiller,Alloc>::Node::construct(order_t k, height_t height, size_t *buf_size, allocator alloc, nallocator nalloc)
{
	order_t lk = pow_of_order<order>(height-1);
	int i = 0;
	try
	{
		for( ; i!=order && lk < k; ++i, k-=lk )
		{
			edges[i].construct(lk, height-1, buf_size, alloc, nalloc);
		}
		if( i != order )
		{
			assert( k <= lk );
			edges[i].construct(k, height-1, buf_size, alloc, nalloc);
			++i;
		}
		for( ; i!=order; ++i )
		{
			edges[i].child = NPtr();
			edges[i].begin = edges[i].end = TPtr();
		}
	}
	catch( ... )
	{
		if( i )
		{
			for( --i; i; --i )
				edges[i].destroy(alloc, nalloc);
			edges[i].destroy(alloc, nalloc);
		}
		throw;
	}
}

template<class RanIt, int order, class Splitter, class Pred, class Refiller, class Alloc>
void merge_tree<RanIt,order,Splitter,Pred,Refiller,Alloc>::Node::Edge::construct(order_t k, height_t height, size_t *buf_size, allocator alloc, nallocator nalloc)
{
	begin = alloc.allocate(*buf_size);
	head = tail = end = begin+*buf_size;
	try
	{
		fill_dflt(begin,end,alloc);
		if( height > 1 )
			try
			{
				child = nalloc.allocate(1);
				try
				{
					nalloc.construct(child, Node());
					child->construct(k,height,buf_size+1,alloc,nalloc);
				}
				catch( ... )
				{
					child->destroy(alloc, nalloc);
					nalloc.destroy(child);
					throw;
				}
			}
			catch( ... )
			{
				nalloc.deallocate(child, 1);
				throw;
			}
		else
			child = NPtr();
	}
	catch( ... )
	{
		alloc.deallocate(begin,*buf_size);
		throw;
	}
}

template<class RanIt, int order, class Splitter, class Pred, class Refiller, class Alloc>
void merge_tree<RanIt,order,Splitter,Pred,Refiller,Alloc>::Node::destroy(allocator alloc, nallocator nalloc)
{
	for( int i=0; i!=order; ++i )
		edges[i].destroy(alloc, nalloc);
}


template<class RanIt, int order, class Splitter, class Pred, class Refiller, class Alloc>
void merge_tree<RanIt,order,Splitter,Pred,Refiller,Alloc>::Node::Edge::destroy(allocator alloc, nallocator nalloc)
{
	if( child != NPtr() )
	{
		child->destroy(alloc,nalloc);
		nalloc.destroy(child);
		nalloc.deallocate(child,1);
	}
	if( begin != TPtr() )
	{
		for( TPtr p=begin; p!=end; ++p )
			alloc.destroy(p);
		alloc.deallocate(begin,end-begin);
	}
}



template<class RanIt, int order, class Splitter, class Pred, class Refiller, class Alloc>
void merge_tree<RanIt,order,Splitter,Pred,Refiller,Alloc>::compute_buffer_sizes(size_type *b, size_type *e)
{
	height_t sp = Splitter::split(static_cast<height_t>(e-b));
	if( e-b > 2 )
	{
		compute_buffer_sizes(b, b+sp);
		compute_buffer_sizes(b+sp, e);
	}
	b[sp] = Splitter::out_buffer_size(pow_of_order<order>(static_cast<height_t>(e-b-sp)));
}

template<class RanIt, int order, class Splitter, class Pred, class Refiller, class Alloc>
void merge_tree<RanIt,order,Splitter,Pred,Refiller,Alloc>::construct(order_t k)
{
	if(!( order < k ))
		throw std::invalid_argument("Merge tree too small");
	height_t height = static_cast<height_t>(logc<order>(k));
	if( height > MAX_MERGE_TREE_HEIGHT )
		throw std::invalid_argument("Merge tree too high");
	bfs_index<order> bfs;
	for( height_t l=1; l!=logc<order>(k); ++l )
		bfs.child(0);
	leaf_index = bfs;
	size_type buffer_sizes[MAX_MERGE_TREE_HEIGHT];
	compute_buffer_sizes(buffer_sizes,buffer_sizes+height);

	root = nalloc.allocate(1);
	try
	{
		nalloc.construct(root, Node());
		root->construct(k,height,buffer_sizes+1,alloc,nalloc);
		try
		{
			order_t K = k+order-1;
			K = K - K%order;
			last_stream = input_streams = salloc.allocate(K);
			SPtr p = input_streams;
			try
			{
				stream_t dflt = stream_t(RanIt(),RanIt());
				for( ; p!=last_stream+K; ++p )
					salloc.construct(p,dflt);
			}
			catch( ... )
			{
				if( p != input_streams )
				{
					for( --p; p!=input_streams; --p )
						salloc.destroy(p);
					salloc.destroy(p);
				}
				salloc.deallocate(input_streams,K);
				throw;
			}
		}
		catch( ... )
		{
			root->destroy(alloc, nalloc);
			nalloc.destroy(root);
			throw;
		}
	}
	catch( ... )
	{
		nalloc.deallocate(root,1);
		throw;
	}
}

template<class RanIt, int order, class Splitter, class Pred, class Refiller, class Alloc>
merge_tree<RanIt,order,Splitter,Pred,Refiller,Alloc>::~merge_tree()
{
	order_t K = k+order-1;
	K = K - K%order;
	for( SPtr s=input_streams; s!=input_streams+K; ++s )
		salloc.destroy(s);
	salloc.deallocate(input_streams, K);
	root->destroy(alloc, nalloc);
	nalloc.destroy(root);
	nalloc.deallocate(root,1);
}

template<class RanIt, int order, class Splitter, class Pred, class Refiller, class Alloc>
void merge_tree<RanIt,order,Splitter,Pred,Refiller,Alloc>::reset()
{
	if( !cold )
	{
		stream_t dflt;
		for( SPtr s=input_streams; s!=last_stream; ++s )
		{
			salloc.destroy(s);
			salloc.construct(s,dflt);
		}
		cold = true;
	}
	last_stream = input_streams;
}

template<class RanIt, int order, class Splitter, class Pred, class Refiller, class Alloc>
template<class FwIt>
FwIt merge_tree<RanIt,order,Splitter,Pred,Refiller,Alloc>::operator()(FwIt begin, FwIt end)
{
	if( cold )
	{
		bfs_index<order> index;
		for( basic_order_t i=0; i!=order; ++i )
		{
			index.child(i);
			go_down_cold(root->edges[i],index);
		}
	}
	cold = false;
	return fill_root(begin, end);
}

template<class RanIt, int order, class Splitter, class Pred, class Refiller, class Alloc>
template<class OutIt>
OutIt merge_tree<RanIt,order,Splitter,Pred,Refiller,Alloc>::operator()(OutIt begin)
{
	if( cold )
	{
		bfs_index<order> index;
		for( basic_order_t i=0; i!=order; ++i )
		{
			index.child(i);
			go_down_cold(root->edges[i],index);
		}
	}
	cold = false;
	return fill_root(begin);
}

template<class RanIt, int order, class Splitter, class Pred, class Refiller, class Alloc>
template<class FwIt>
FwIt merge_tree<RanIt,order,Splitter,Pred,Refiller,Alloc>::empty_buffers(NPtr n, FwIt begin, FwIt end)
{
	for( int i=0; i!=order; ++i )
	{
		for( ; n->edges[i].head!=n->edges[i].tail && begin!=end; ++n->edges[i].head, ++begin )
			*begin = *n->edges[i].head;
		if( n->edges[i].child )
			begin = empty_buffers(n->edges[i].child, begin, end);
	}
	return begin;
}

template<class RanIt, int order, class Splitter, class Pred, class Refiller, class Alloc>
template<class OutIt>
OutIt merge_tree<RanIt,order,Splitter,Pred,Refiller,Alloc>::empty_buffers(NPtr n, OutIt begin)
{
	for( int i=0; i!=order; ++i )
	{
		begin = std::copy(n->edges[i].head, n->edges[i].tail, begin);
		if( n->edges[i].child )
			begin = empty_buffers(n->edges[i].child, begin);
	}
	return begin;
}

template<class RanIt, int order, class Splitter, class Pred, class Refiller, class Alloc>
void merge_tree<RanIt,order,Splitter,Pred,Refiller,Alloc>::go_down(typename Node::Edge& e, bfs_index<order> index)
{
	assert( e.tail == e.head );
	e.head = e.begin;
	if( e.child == NPtr() )
		fill_leaf(e,index);
	else
		fill(e.child,e,index);
	e.tail = e.head, e.head = e.begin;
	index.parent();
}

template<class RanIt, int order, class Splitter, class Pred, class Refiller, class Alloc>
void merge_tree<RanIt,order,Splitter,Pred,Refiller,Alloc>::go_down_cold(typename Node::Edge& e, bfs_index<order>& index)
{
	e.tail = e.end, e.head = e.begin;
	if( e.child )
		warmup(e.child, e, index);
	else if( leaf_index <= index && input_streams+order*(index-leaf_index) < last_stream )
		fill_leaf(e,index);
	e.tail = e.head, e.head = e.begin;
	index.parent();
}

template<class RanIt, int order, class Splitter, class Pred, class Refiller, class Alloc>
void merge_tree<RanIt,order,Splitter,Pred,Refiller,Alloc>::warmup(NPtr n, typename Node::Edge& output, bfs_index<order> index)
{
	for( basic_order_t i=0; i!=order; ++i )
	{
		index.child(i);
		go_down_cold(n->edges[i],index);
	}
	fill(n,output,index);
}

template<class RanIt, int order, class Splitter, class Pred, class Refiller, class Alloc>
template<class FwIt>
FwIt merge_tree<RanIt,order,Splitter,Pred,Refiller,Alloc>::copy_root(typename Node::Edge& de, bfs_index<order> index, FwIt begin, FwIt end, std::forward_iterator_tag)
{
	for( ;; )
	{
		for( ; de.tail!=de.head && begin!=end; ++begin, ++de.head )
			*begin = *de.head;
		if( de.tail == de.head )
		{
			go_down(de,index);
			if( de.head == de.tail )
				return begin;
		}
		else
			return begin;
	}
}

template<class RanIt, int order, class Splitter, class Pred, class Refiller, class Alloc>
template<class RIt>
RIt merge_tree<RanIt,order,Splitter,Pred,Refiller,Alloc>::copy_root(typename Node::Edge& de, bfs_index<order> index, RIt begin, RIt end, std::random_access_iterator_tag)
{
	for( ;; )
	{
		for( ; de.tail!=de.head && begin!=end; ++begin, ++de.head )
			*begin = *de.head;
		if( de.tail == de.head )
		{
			go_down(de,index);
			if( de.head == de.tail )
				return begin;
		}
		else
			return begin;
	}
}

template<class RanIt, int order, class Splitter, class Pred, class Refiller, class Alloc>
template<class OutIt>
OutIt merge_tree<RanIt,order,Splitter,Pred,Refiller,Alloc>::copy_root(typename Node::Edge& de, bfs_index<order> index, OutIt begin)
{
	do
	{
		for( ; de.tail!=de.head; ++begin, ++de.head )
			*begin = *de.head;
		go_down(de,index);
	}
	while( de.head != de.tail );
	return begin;
}

template<class RanIt, int order, class Splitter, class Pred, class Refiller, class Alloc>
typename merge_tree<RanIt,order,Splitter,Pred,Refiller,Alloc>::TPtr merge_tree<RanIt,order,Splitter,Pred,Refiller,Alloc>::copy(typename Node::Edge& de, bfs_index<order> index, TPtr b, TPtr e)
{
	for( ;; )
	{
		if( e-b < de.tail-de.head )
		{
			for( ; b!=e; ++b, ++de.head )
				*b = *de.head;
			return b;
		}
		else
		{
			for( ; de.tail!=de.head; ++b, ++de.head )
				*b = *de.head;
			go_down(de,index);
			if( de.head == de.tail )
				return b;
		}
	}
}


/*****
 *  Order 2 basic merger template specialization
 */

template<class RanIt, class Splitter, class Pred, class Refiller, class Alloc>
class special_<RanIt,2,Splitter,Pred,Refiller,Alloc>
{
	friend class merge_tree<RanIt,2,Splitter,Pred,Refiller,Alloc>;

	template<class FwIt>
	static FwIt fill_root(merge_tree<RanIt,2,Splitter,Pred,Refiller,Alloc>* that, FwIt begin, FwIt end)
	{
		typedef typename merge_tree<RanIt,2,Splitter,Pred,Refiller,Alloc>::TPtr TPtr;
		typedef typename merge_tree<RanIt,2,Splitter,Pred,Refiller,Alloc>::Node Node;
		bfs_index<2> index;
		for( ;; )
			if( that->root->edges[0].head != that->root->edges[0].tail )
				if( that->root->edges[1].head != that->root->edges[1].tail )
					for( TPtr l=that->root->edges[0].head, r=that->root->edges[1].head;; )
					{
						if( begin == end )
						{
							that->root->edges[0].head = l;
							that->root->edges[1].head = r;
							return begin;
						}
						if( that->comp(*l,*r) )
						{
							*begin = *l, ++l, ++begin;
							if( l == that->root->edges[0].tail )
							{
								typename Node::Edge& de = that->root->edges[0];
								that->root->edges[0].head = l; that->root->edges[1].head = r;
								index.child(0);
								that->go_down(de,index);
								index.parent();
								if( de.head == de.tail )
									break;
								l = that->root->edges[0].head;
							}
						}
						else
						{
							*begin = *r, ++r, ++begin;
							if( r == that->root->edges[1].tail )
							{
								typename Node::Edge& de = that->root->edges[1];
								that->root->edges[0].head = l; that->root->edges[1].head = r;
								index.child(1);
								that->go_down(de,index);
								index.parent();
								if( de.head == de.tail )
									break;
								r = that->root->edges[1].head;
							}
						}
					}
				else
				{
					index.child(0);
					return that->copy_root(that->root->edges[0], index, begin, end, typename std::iterator_traits<FwIt>::iterator_category());
				}
			else if( that->root->edges[1].head != that->root->edges[1].tail )
			{
				index.child(1);
				return that->copy_root(that->root->edges[1], index, begin, end, typename std::iterator_traits<FwIt>::iterator_category());
			}
			else
				return begin;
	}

	template<class OutIt>
	static OutIt fill_root(merge_tree<RanIt,2,Splitter,Pred,Refiller,Alloc>* that, OutIt begin)
	{
		typedef typename merge_tree<RanIt,2,Splitter,Pred,Refiller,Alloc>::TPtr TPtr;
		typedef typename merge_tree<RanIt,2,Splitter,Pred,Refiller,Alloc>::Node Node;
		bfs_index<2> index;
		for( ;; )
			if( that->root->edges[0].head != that->root->edges[0].tail )
				if( that->root->edges[1].head != that->root->edges[1].tail )
					for( TPtr l=that->root->edges[0].head, r=that->root->edges[1].head;; )
						if( that->comp(*l,*r) )
						{
							*begin = *l, ++l, ++begin;
							if( l == that->root->edges[0].tail )
							{
								typename Node::Edge& de = that->root->edges[0];
								that->root->edges[0].head = l; that->root->edges[1].head = r;
								index.child(0);
								that->go_down(de,index);
								index.parent();
								if( de.head == de.tail )
									break;
								l = that->root->edges[0].head;
							}
						}
						else
						{
							*begin = *r, ++r, ++begin;
							if( r == that->root->edges[1].tail )
							{
								typename Node::Edge& de = that->root->edges[1];
								that->root->edges[0].head = l; that->root->edges[1].head = r;
								index.child(1);
								that->go_down(de,index);
								index.parent();
								if( de.head == de.tail )
									break;
								r = that->root->edges[1].head;
							}
						}
				else
				{
					index.child(0);
					return that->copy_root(that->root->edges[0], index, begin);
				}
			else if( that->root->edges[1].head != that->root->edges[1].tail )
			{
				index.child(1);
				return that->copy_root(that->root->edges[1], index, begin);
			}
			else
				return begin;
	}

	static void fill(merge_tree<RanIt,2,Splitter,Pred,Refiller,Alloc>* that, typename merge_tree<RanIt,2,Splitter,Pred,Refiller,Alloc>::NPtr n, typename merge_tree<RanIt,2,Splitter,Pred,Refiller,Alloc>::Node::Edge& output, bfs_index<2> index)
	{
		typedef typename merge_tree<RanIt,2,Splitter,Pred,Refiller,Alloc>::TPtr TPtr;
		typedef typename merge_tree<RanIt,2,Splitter,Pred,Refiller,Alloc>::Node Node;
		for( TPtr p = output.head;; )
			if( n->edges[0].head != n->edges[0].tail )
				if( n->edges[1].head != n->edges[1].tail )
					for( TPtr l=n->edges[0].head, r=n->edges[1].head;; )
					{
						if( p == output.tail )
						{
							output.head = p;
							n->edges[0].head = l;
							n->edges[1].head = r;
							return;
						}
#ifdef USE_COND_MOVS_
#ifdef _MSC_VER
						__asm
						{
							mov eax, p
							mov ebx, l
							mov ecx, r
							mov edi, dword ptr [ebx]		; edges[0] key in edi
							mov edx, dword ptr [ecx]		; edges[1] key in edx
							cmp edi, edx					; compare keys
							cmovl edx, edi					; small key in edx
							cmovl edi, dword ptr [ebx+4]	; small pointer in edi
							cmovge edi, dword ptr [ecx+4]	; small pointer in edi
							mov dword ptr [eax], edx		; output small key
							mov dword ptr [eax+4], edi		; output small pointer
							setl dl
							mov edi, ebx					; cond-inc edges[0]
							add edi, 8
							test dl, dl
							cmovnz ebx, edi
							mov edi, ecx					; cond-inc edges[1]
							add edi, 8
							test dl, dl
							cmovz ecx, edi
							add eax, 8						; inc out
							mov r, ecx
							mov l, ebx
							mov p, eax
						}
#else // _MSC_VER
#ifdef __GNUC__
						asm("movl\t(%1), %%edi\n\t"		// edges[0] key in edi
							"movl\t(%2), %%edx\n\t"		// edges[1] key in edx
							"cmpl\t%%edx, %%edi\n\t"	// compare keys
							"cmovll\t%%edi, %%edx\n\t"	// small key in edx
							"cmovll\t4(%1), %%edi\n\t"	// small pointer in edi
							"cmovgel\t4(%2), %%edi\n\t"	// small pointer in edi
							"movl\t%%edx, (%0)\n\t"		// output small key
							"movl\t%%edi, 4(%0)\n\t"	// output small pointer
							"setb\t%%dl\n\t"
							"movl\t%1, %%edi\n\t"		// cond-inc edges[0]
							"addl\t$8, %%edi\n\t"
							"testb\t%%dl, %%dl\n\t"
							"cmovnzl\t%%edi, %1\n\t"
							"movl\t%2, %%edi\n\t"		// cond-inc edges[1]
							"addl\t$8, %%edi\n\t"
							"testb\t%%dl, %%dl\n\t"
							"cmovzl\t%%edi, %2\n\t"
							"addl\t$8, %0"				// inc out
							: "=r"(p), "=r"(l), "=r"(r) : "0"(p), "1"(l), "2"(r) : "edx", "edi");
#else // __GNUC__
						register TPtr nh = l+1;
						bool b = that->comp(*l,*r);
						*p = b? *l : *r;
						l = b? nh : l;
						nh = r+1;
						r = b? r : nh;
						++p;
#endif // __GNUC__
#endif // _MSC_VER
						if( l == n->edges[0].tail )
						{
							typename Node::Edge& de = n->edges[0];
							n->edges[0].head = l; n->edges[1].head = r;
							index.child(0);
							that->go_down(de,index);
							if( de.head == de.tail )
								break;
							l = n->edges[0].head;
						}
						if( r == n->edges[1].tail )
						{
							typename Node::Edge& de = n->edges[1];
							n->edges[0].head = l; n->edges[1].head = r;
							index.child(1);
							that->go_down(de,index);
							if( de.head == de.tail )
								break;
							r = n->edges[1].head;
						}
#else // USE_COND_MOVS_
						if( that->comp(*l,*r) )
						{
							*p = *l, ++l, ++p;
							if( l == n->edges[0].tail )
							{
								typename Node::Edge& de = n->edges[0];
								n->edges[0].head = l; n->edges[1].head = r;
								index.child(0);
								that->go_down(de,index);
								index.parent();
								if( de.head == de.tail )
									break;
								l = n->edges[0].head;
							}
						}
						else
						{
							*p = *r, ++r, ++p;
							if( r == n->edges[1].tail )
							{
								typename Node::Edge& de = n->edges[1];
								n->edges[0].head = l; n->edges[1].head = r;
								index.child(1);
								that->go_down(de,index);
								index.parent();
								if( de.head == de.tail )
									break;
								r = n->edges[1].head;
							}
						}
#endif // USE_COND_MOVS_
					}
				else
				{
					index.child(0);
					output.head = that->copy(n->edges[0], index, p, output.tail);
					return;
				}
			else if( n->edges[1].head != n->edges[1].tail )
			{
				index.child(1);
				output.head = that->copy(n->edges[1], index, p, output.tail);
				return;
			}
			else
				return;
	}

	static void fill_leaf(merge_tree<RanIt,2,Splitter,Pred,Refiller,Alloc>* that, typename merge_tree<RanIt,2,Splitter,Pred,Refiller,Alloc>::Node::Edge& output, bfs_index<2> index)
	{
		typedef typename merge_tree<RanIt,2,Splitter,Pred,Refiller,Alloc>::TPtr TPtr;
		typedef typename merge_tree<RanIt,2,Splitter,Pred,Refiller,Alloc>::stream_iterator stream_iterator;
		stream_iterator left = that->input_streams+2*(index-that->leaf_index);
		stream_iterator right = left;
		++right;
		//assert( left < that->last_stream );
		//assert( that->input_streams <= left );
		RanIt l=(*left).begin(), r=(*right).begin();
		for( TPtr p = output.head;; )
			if( l != (*left).end() )
				if( r != (*right).end() )
					for( ;; )
					{
						if( p == output.tail )
						{
							output.head = p;
							(*left).begin() = l;
							(*right).begin() = r;
							// TODO: Refill
							return;
						}
						if( that->comp(*l,*r) )
						{
							*p = *l, ++l, ++p;
							if( l == (*left).end() )
								break;
						}
						else
						{
							*p = *r, ++r, ++p;
							if( r == (*right).end() )
								break;
						}
					}
				else
				{
					assert( r == (*right).end() );
					if( output.tail-p < (*left).end()-l )
						for( ; p!=output.tail; ++p, ++l )
							*p = *l;
					else
						for( ; (*left).end()!=l; ++p, ++l )
							*p = *l;
					output.head = p;
					(*left).begin() = l;
					(*right).begin() = r;
					// TODO: Refill
					return;
				}
			else if( r != (*right).end() )
			{
				assert( l == (*left).end() );
				if( output.tail-p < (*right).end()-r )
					for( ; p!=output.tail; ++p, ++r )
						*p = *r;
				else
					for( ; (*right).end()!=r; ++p, ++r )
						*p = *r;
				output.head = p;
				(*left).begin() = l;
				(*right).begin() = r;
				// TODO: Refill
				return;
			}
			else
				return;
	}
};

/*****
 *  Order 4 basic merger template specialization
 */

#define MOVE_SMALL(i,n)		*begin = *head[i], ++head[i], ++begin;  \
							if( head[i] == tail[i] )  \
							{  \
								stream[i]->head = head[i];  \
								index.child(static_cast<basic_order_t>(stream[i]-n->edges));  \
								that->go_down(*stream[i],index);  \
								index.parent();  \
								head[i] = stream[i]->head; tail[i] = stream[i]->tail;  \
								if( head[i] == tail[i] )  \
								{  \
									--active;  \
									head[i] = head[active];  \
									tail[i] = tail[active];  \
									stream[i] = stream[active];  \
									break;  \
								}  \
							}
#define MOVE_SMALL_(i,n)	*begin = *head[i], ++head[i], ++begin;  \
							if( head[i] == tail[i] )  \
								if( go_down(that, i, active-1, index, head, tail, n->edges, stream) )  \
								{  \
									--active;  \
									break;  \
								}

template<class RanIt, class Splitter, class Pred, class Refiller, class Alloc>
class special_<RanIt,4,Splitter,Pred,Refiller,Alloc>
{
	friend class merge_tree<RanIt,4,Splitter,Pred,Refiller,Alloc>;

	template<class TPtr>
	static bool go_down(merge_tree<RanIt,4,Splitter,Pred,Refiller,Alloc> *that, order_t i, order_t active, bfs_index<4> index, TPtr *head, TPtr *tail, typename merge_tree<RanIt,4,Splitter,Pred,Refiller,Alloc>::Node::Edge* edges, typename merge_tree<RanIt,4,Splitter,Pred,Refiller,Alloc>::Node::Edge** stream)
	{
		stream[i]->head = head[i];
		index.child(static_cast<basic_order_t>(stream[i]-edges));
		that->go_down(*stream[i],index);
		head[i] = stream[i]->head; tail[i] = stream[i]->tail;
		if( head[i] == tail[i] )
		{
			head[i] = head[active];
			tail[i] = tail[active];
			stream[i] = stream[active];
			return true;
		}
		else
			return false;
	}

	template<class FwIt>
	static FwIt fill_root(merge_tree<RanIt,4,Splitter,Pred,Refiller,Alloc>* that, FwIt begin, FwIt end)
	{
		typedef typename merge_tree<RanIt,4,Splitter,Pred,Refiller,Alloc>::TPtr TPtr;
		typedef typename merge_tree<RanIt,4,Splitter,Pred,Refiller,Alloc>::Node::Edge Edge;
		Pred& comp = that->comp;
		bfs_index<4> index;
		TPtr head[4], tail[4];
		Edge *stream[4];
		order_t active = 0;
		for( Edge *p=that->root->edges; p!=that->root->edges+4; ++p )
			if( p->head != p->tail )
			{
				stream[active] = p;
				head[active] = p->head, tail[active] = p->tail;
				++active;
			}

		switch( active )
		{
		case 4:
			for( ;; )
			{
				if( begin == end )
				{
					stream[0]->head = head[0]; stream[1]->head = head[1]; stream[2]->head = head[2]; stream[3]->head = head[3];
					return begin;
				}
				if( comp(*head[0],*head[3]) )
				{
					if( comp(*head[0],*head[2]) )
					{
						if( comp(*head[0],*head[1]) )
						{
							MOVE_SMALL(0,that->root)
						}
						else
						{
							MOVE_SMALL(1,that->root)
						}
					}
					else
					{
						if( comp(*head[1],*head[2]) )
						{
							MOVE_SMALL(1,that->root)
						}
						else
						{
							MOVE_SMALL(2,that->root)
						}
					}
				}
				else
				{
					if( comp(*head[1],*head[3]) )
					{
						if( comp(*head[1],*head[2]) )
						{
							MOVE_SMALL(1,that->root)
						}
						else
						{
							MOVE_SMALL(2,that->root)
						}
					}
					else
					{
						if( comp(*head[2],*head[3]) )
						{
							MOVE_SMALL(2,that->root)
						}
						else
						{
							MOVE_SMALL(3,that->root)
						}
					}
				}
			}
		case 3:
			for( ;; )
			{
				if( begin == end )
				{
					stream[0]->head = head[0]; stream[1]->head = head[1]; stream[2]->head = head[2];
					return begin;
				}
				if( comp(*head[0],*head[2]) )
				{
					if( comp(*head[0],*head[1]) )
					{
						MOVE_SMALL(0,that->root)
					}
					else
					{
						MOVE_SMALL(1,that->root)
					}
				}
				else
				{
					if( comp(*head[1],*head[2]) )
					{
						MOVE_SMALL(1,that->root)
					}
					else
					{
						MOVE_SMALL(2,that->root)
					}
				}
			}
		case 2:
			for( ;; )
			{
				if( begin == end )
				{
					stream[0]->head = head[0]; stream[1]->head = head[1];
					return begin;
				}
				if( comp(*head[0],*head[1]) )
				{
					MOVE_SMALL(0,that->root)
				}
				else
				{
					MOVE_SMALL(1,that->root)
				}
			}
		case 1:
			stream[0]->head = head[0];
			index.child(static_cast<basic_order_t>(stream[0]-that->root->edges));
			begin = that->copy_root(*stream[0], index, begin, end, typename std::iterator_traits<FwIt>::iterator_category());
		case 0:
			return begin;
		default:
			assert( false );
			return begin;
		}
	}

	template<class OutIt>
	static OutIt fill_root(merge_tree<RanIt,4,Splitter,Pred,Refiller,Alloc>* that, OutIt begin)
	{
		typedef typename merge_tree<RanIt,4,Splitter,Pred,Refiller,Alloc>::TPtr TPtr;
		typedef typename merge_tree<RanIt,4,Splitter,Pred,Refiller,Alloc>::Node::Edge Edge;
		Pred& comp = that->comp;
		bfs_index<4> index;
		TPtr head[4], tail[4];
		Edge *stream[4];
		order_t active = 0;
		for( Edge *p=that->root->edges; p!=that->root->edges+4; ++p )
			if( p->head != p->tail )
			{
				stream[active] = p;
				head[active] = p->head, tail[active] = p->tail;
				++active;
			}

		switch( active )
		{
		case 4:
			for( ;; )
				if( comp(*head[0],*head[3]) )
				{
					if( comp(*head[0],*head[2]) )
					{
						if( comp(*head[0],*head[1]) )
						{
							MOVE_SMALL(0,that->root)
						}
						else
						{
							MOVE_SMALL(1,that->root)
						}
					}
					else
					{
						if( comp(*head[1],*head[2]) )
						{
							MOVE_SMALL(1,that->root)
						}
						else
						{
							MOVE_SMALL(2,that->root)
						}
					}
				}
				else
				{
					if( comp(*head[1],*head[3]) )
					{
						if( comp(*head[1],*head[2]) )
						{
							MOVE_SMALL(1,that->root)
						}
						else
						{
							MOVE_SMALL(2,that->root)
						}
					}
					else
					{
						if( comp(*head[2],*head[3]) )
						{
							MOVE_SMALL(2,that->root)
						}
						else
						{
							MOVE_SMALL(3,that->root)
						}
					}
				}
		case 3:
			for( ;; )
				if( comp(*head[0],*head[2]) )
				{
					if( comp(*head[0],*head[1]) )
					{
						MOVE_SMALL(0,that->root)
					}
					else
					{
						MOVE_SMALL(1,that->root)
					}
				}
				else
				{
					if( comp(*head[1],*head[2]) )
					{
						MOVE_SMALL(1,that->root)
					}
					else
					{
						MOVE_SMALL(2,that->root)
					}
				}
		case 2:
			for( ;; )
				if( comp(*head[0],*head[1]) )
				{
					MOVE_SMALL(0,that->root)
				}
				else
				{
					MOVE_SMALL(1,that->root)
				}
		case 1:
			stream[0]->head = head[0];
			index.child(static_cast<basic_order_t>(stream[0]-that->root->edges));
			begin = that->copy_root(*stream[0], index, begin);
		case 0:
			return begin;
		default:
			assert( false );
			return begin;
		}
	}

	static void fill(merge_tree<RanIt,4,Splitter,Pred,Refiller,Alloc>* that, typename merge_tree<RanIt,4,Splitter,Pred,Refiller,Alloc>::NPtr n, typename merge_tree<RanIt,4,Splitter,Pred,Refiller,Alloc>::Node::Edge& output, bfs_index<4> index)
	{
		typedef typename merge_tree<RanIt,4,Splitter,Pred,Refiller,Alloc>::TPtr TPtr;
		typedef typename merge_tree<RanIt,4,Splitter,Pred,Refiller,Alloc>::Node::Edge Edge;
		Pred& comp = that->comp;
		TPtr head[4], tail[4];
		Edge *stream[4];
		order_t active = 0;
		for( Edge *p=n->edges; p!=n->edges+4; ++p )
			if( p->head != p->tail )
			{
				stream[active] = p;
				head[active] = p->head, tail[active] = p->tail;
				++active;
			}

		TPtr begin = output.head;
		switch( active )
		{
		case 4:
			for( ;; )
			{
				if( begin == output.tail )
				{
					output.head = begin;
					stream[0]->head = head[0]; stream[1]->head = head[1]; stream[2]->head = head[2]; stream[3]->head = head[3];
					return;
				}
				if( comp(*head[0],*head[3]) )
				{
					if( comp(*head[0],*head[2]) )
					{
						if( comp(*head[0],*head[1]) )
						{
							MOVE_SMALL(0,n)
						}
						else
						{
							MOVE_SMALL(1,n)
						}
					}
					else
					{
						if( comp(*head[1],*head[2]) )
						{
							MOVE_SMALL(1,n)
						}
						else
						{
							MOVE_SMALL(2,n)
						}
					}
				}
				else
				{
					if( comp(*head[1],*head[3]) )
					{
						if( comp(*head[1],*head[2]) )
						{
							MOVE_SMALL(1,n)
						}
						else
						{
							MOVE_SMALL(2,n)
						}
					}
					else
					{
						if( comp(*head[2],*head[3]) )
						{
							MOVE_SMALL(2,n)
						}
						else
						{
							MOVE_SMALL(3,n)
						}
					}
				}
			}
		case 3:
			for( ;; )
			{
				if( begin == output.tail )
				{
					output.head = begin;
					stream[0]->head = head[0]; stream[1]->head = head[1]; stream[2]->head = head[2];
					return;
				}
				if( comp(*head[0],*head[2]) )
				{
					if( comp(*head[0],*head[1]) )
					{
						MOVE_SMALL(0,n)
					}
					else
					{
						MOVE_SMALL(1,n)
					}
				}
				else
				{
					if( comp(*head[1],*head[2]) )
					{
						MOVE_SMALL(1,n)
					}
					else
					{
						MOVE_SMALL(2,n)
					}
				}
			}
		case 2:
			for( ;; )
			{
				if( begin == output.tail )
				{
					output.head = begin;
					stream[0]->head = head[0]; stream[1]->head = head[1];
					return;
				}
				if( comp(*head[0],*head[1]) )
				{
					MOVE_SMALL(0,n)
				}
				else
				{
					MOVE_SMALL(1,n)
				}
			}
		case 1:
			stream[0]->head = head[0];
			index.child(static_cast<basic_order_t>(stream[0]-n->edges));
			output.head = that->copy(*stream[0], index, begin, output.tail);
		case 0:
			return;
		default:
			assert( false );
			return;
		}
	}

	static void fill_leaf(merge_tree<RanIt,4,Splitter,Pred,Refiller,Alloc>* that, typename merge_tree<RanIt,4,Splitter,Pred,Refiller,Alloc>::Node::Edge& output, bfs_index<4> index)
	{
		typedef typename merge_tree<RanIt,4,Splitter,Pred,Refiller,Alloc>::TPtr TPtr;
		typedef typename merge_tree<RanIt,4,Splitter,Pred,Refiller,Alloc>::SPtr SPtr;
		assert( that->input_streams+4*(index-that->leaf_index) < that->last_stream );
		assert( that->input_streams <= that->input_streams+4*(index-that->leaf_index) );
		Pred& comp = that->comp;
		RanIt head[4], tail[4];
		SPtr stream[4];
		order_t active = 0;
		for( SPtr p=that->input_streams+4*(index-that->leaf_index); p!=that->input_streams+4*(index-that->leaf_index+1); ++p )
			if( p->first != p->second )
			{
				stream[active] = p;
				head[active] = p->first, tail[active] = p->second;
				++active;
			}

		TPtr begin = output.head;
		switch( active )
		{
		case 4:
			for( ;; )
			{
				if( begin == output.tail )
				{
					output.head = begin;
					stream[0]->first = head[0]; stream[1]->first = head[1]; stream[2]->first = head[2]; stream[3]->first = head[3];
					// TODO: Refill
					return;
				}
				if( comp(*head[0],*head[3]) )
				{
					if( comp(*head[0],*head[2]) )
					{
						if( comp(*head[0],*head[1]) )
						{
							*begin = *head[0], ++head[0], ++begin;
							if( head[0] == tail[0] )
							{
								stream[0]->first = head[0];
								head[0] = head[3];
								tail[0] = tail[3];
								stream[0] = stream[3];
								break;
							}
						}
						else
						{
							*begin = *head[1], ++head[1], ++begin;
							if( head[1] == tail[1] )
							{
								stream[1]->first = head[1];
								head[1] = head[3];
								tail[1] = tail[3];
								stream[1] = stream[3];
								break;
							}
						}
					}
					else
					{
						if( comp(*head[1],*head[2]) )
						{
							*begin = *head[1], ++head[1], ++begin;
							if( head[1] == tail[1] )
							{
								stream[1]->first = head[1];
								head[1] = head[3];
								tail[1] = tail[3];
								stream[1] = stream[3];
								break;
							}
						}
						else
						{
							*begin = *head[2], ++head[2], ++begin;
							if( head[2] == tail[2] )
							{
								stream[2]->first = head[2];
								head[2] = head[3];
								tail[2] = tail[3];
								stream[2] = stream[3];
								break;
							}
						}
					}
				}
				else
				{
					if( comp(*head[1],*head[3]) )
					{
						if( comp(*head[1],*head[2]) )
						{
							*begin = *head[1], ++head[1], ++begin;
							if( head[1] == tail[1] )
							{
								stream[1]->first = head[1];
								head[1] = head[3];
								tail[1] = tail[3];
								stream[1] = stream[3];
								break;
							}
						}
						else
						{
							*begin = *head[2], ++head[2], ++begin;
							if( head[2] == tail[2] )
							{
								stream[2]->first = head[2];
								head[2] = head[3];
								tail[2] = tail[3];
								stream[2] = stream[3];
								break;
							}
						}
					}
					else
					{
						if( comp(*head[2],*head[3]) )
						{
							*begin = *head[2], ++head[2], ++begin;
							if( head[2] == tail[2] )
							{
								stream[2]->first = head[2];
								head[2] = head[3];
								tail[2] = tail[3];
								stream[2] = stream[3];
								break;
							}
						}
						else
						{
							*begin = *head[3], ++head[3], ++begin;
							if( head[3] == tail[3] )
							{
								stream[3]->first = head[3];
								break;
							}
						}
					}
				}
			}
		case 3:
			for( ;; )
			{
				if( begin == output.tail )
				{
					output.head = begin;
					stream[0]->first = head[0]; stream[1]->first = head[1]; stream[2]->first = head[2];
					// TODO: Refill
					return;
				}
				if( comp(*head[0],*head[2]) )
				{
					if( comp(*head[0],*head[1]) )
					{
						*begin = *head[0], ++head[0], ++begin;
						if( head[0] == tail[0] )
						{
							stream[0]->first = head[0];
							head[0] = head[2];
							tail[0] = tail[2];
							stream[0] = stream[2];
							break;
						}
					}
					else
					{
						*begin = *head[1], ++head[1], ++begin;
						if( head[1] == tail[1] )
						{
							stream[1]->first = head[1];
							head[1] = head[2];
							tail[1] = tail[2];
							stream[1] = stream[2];
							break;
						}
					}
				}
				else
				{
					if( comp(*head[1],*head[2]) )
					{
						*begin = *head[1], ++head[1], ++begin;
						if( head[1] == tail[1] )
						{
							stream[1]->first = head[1];
							head[1] = head[2];
							tail[1] = tail[2];
							stream[1] = stream[2];
							break;
						}
					}
					else
					{
						*begin = *head[2], ++head[2], ++begin;
						if( head[2] == tail[2] )
						{
							stream[2]->first = head[2];
							break;
						}
					}
				}
			}
		case 2:
			for( ;; )
			{
				if( begin == output.tail )
				{
					output.head = begin;
					stream[0]->first = head[0]; stream[1]->first = head[1];
					// TODO: Refill
					return;
				}
				if( comp(*head[0],*head[1]) )
				{
					*begin = *head[0], ++head[0], ++begin;
					if( head[0] == tail[0] )
					{
						stream[0]->first = head[0];
						
						if( output.tail-begin < stream[1]->second-head[1] )
							for( ; begin!=output.tail; ++begin, ++head[1] )
								*begin = *head[1];
						else
							for( ; stream[1]->second!=head[1]; ++begin, ++head[1] )
								*begin = *head[1];
						output.head = begin;
						stream[1]->first = head[1];
						
						//head[0] = head[1];
						//tail[0] = tail[1];
						//stream[0] = stream[1];
						//break;
						return;
					}
				}
				else
				{
					*begin = *head[1], ++head[1], ++begin;
					if( head[1] == tail[1] )
					{
						stream[1]->first = head[1];
						break;
					}
				}
			}
		case 1:
			if( output.tail-begin < stream[0]->second-head[0] )
				for( ; begin!=output.tail; ++begin, ++head[0] )
					*begin = *head[0];
			else
				for( ; stream[0]->second!=head[0]; ++begin, ++head[0] )
					*begin = *head[0];
			output.head = begin;
			stream[0]->first = head[0];
			// TODO: Refill
			return;
		case 0:
			return;
		default:
			assert( false );
			return;
		}
	}
};



/*****
 *  Alloc::pointer iterator template specialization
 */

template<int order, class Splitter, class Pred, class Refiller, class Alloc>
void merge_tree<typename Alloc::pointer,order,Splitter,Pred,Refiller,Alloc>::Node::Edge::fill_dflt(TPtr b, TPtr e, allocator alloc)
{
	TPtr p = b;
	try
	{
		value_type dflt;
		for( ; p!=e; ++p )
			alloc.construct(p,dflt);
	}
	catch( ... )
	{
		if( p != b )
		{
			for( --p; p!=b; --p )
				alloc.destroy(p);
			alloc.destroy(p);
		}
		alloc.deallocate(b,e-b);
		throw;
	}
}

template<int Order, class Splitter, class Pred, class Refiller, class Alloc>
typename merge_tree<typename Alloc::pointer,Order,Splitter,Pred,Refiller,Alloc>::Node::Edge** merge_tree<typename Alloc::pointer,Order,Splitter,Pred,Refiller,Alloc>::Node::construct(order_t k, height_t height, size_t *buf_size, typename merge_tree<typename Alloc::pointer,Order,Splitter,Pred,Refiller,Alloc>::Node::Edge** edge_list, allocator alloc, nallocator nalloc)
{
	order_t lk = pow_of_order<order>(height-1);
	int i = 0;
	try
	{
		for( ; i!=order && lk < k; ++i, k-=lk )
		{
			edge_list = edges[i].construct(lk, height-1, buf_size, edge_list, alloc, nalloc);
		}
		if( i != order )
		{
			assert( k <= lk );
			edge_list = edges[i].construct(k, height-1, buf_size, edge_list, alloc, nalloc);
			++i;
		}
		for( ; i!=order; ++i )
		{
			edges[i].child = NPtr();
			edges[i].begin = edges[i].end = TPtr();
		}
		return edge_list;
	}
	catch( ... )
	{
		if( i )
		{
			for( --i; i; --i )
				edges[i].destroy(alloc, nalloc);
			edges[i].destroy(alloc, nalloc);
		}
		throw;
	}
}

template<int order, class Splitter, class Pred, class Refiller, class Alloc>
typename merge_tree<typename Alloc::pointer,order,Splitter,Pred,Refiller,Alloc>::Node::Edge** merge_tree<typename Alloc::pointer,order,Splitter,Pred,Refiller,Alloc>::Node::Edge::construct(order_t k, height_t height, size_t *buf_size, Edge** edge_list, allocator alloc, nallocator nalloc)
{
	if( height > 0 )
	{
		begin = alloc.allocate(*buf_size);
		head = tail = end = begin+*buf_size;
		try
		{
			fill_dflt(begin,end,alloc);
			try
			{
				child = nalloc.allocate(1);
				try
				{
					nalloc.construct(child, Node());
					return child->construct(k,height,buf_size+1,edge_list,alloc,nalloc);
				}
				catch( ... )
				{
					child->destroy(alloc, nalloc);
					nalloc.destroy(child);
					throw;
				}
			}
			catch( ... )
			{
				nalloc.deallocate(child, 1);
				throw;
			}
		}
		catch( ... )
		{
			alloc.deallocate(begin,*buf_size);
			throw;
		}
	}
	else
	{
		child = NPtr();
		head = tail = end = begin = TPtr();
		*edge_list = this;
		return edge_list+1;
	}
}

template<int order, class Splitter, class Pred, class Refiller, class Alloc>
void merge_tree<typename Alloc::pointer,order,Splitter,Pred,Refiller,Alloc>::Node::destroy(allocator alloc, nallocator nalloc)
{
	for( int i=0; i!=order; ++i )
		edges[i].destroy(alloc, nalloc);
}


template<int order, class Splitter, class Pred, class Refiller, class Alloc>
void merge_tree<typename Alloc::pointer,order,Splitter,Pred,Refiller,Alloc>::Node::Edge::destroy(allocator alloc, nallocator nalloc)
{
	if( child != NPtr() )
	{
		child->destroy(alloc,nalloc);
		nalloc.destroy(child);
		nalloc.deallocate(child,1);
	}
	if( begin != TPtr() )
	{
		for( TPtr p=begin; p!=end; ++p )
			alloc.destroy(p);
		alloc.deallocate(begin,end-begin);
	}
}


template<int order, class Splitter, class Pred, class Refiller, class Alloc>
void merge_tree<typename Alloc::pointer,order,Splitter,Pred,Refiller,Alloc>::compute_buffer_sizes(size_type *b, size_type *e)
{
	height_t sp = Splitter::split(static_cast<height_t>(e-b));
	if( e-b > 2 )
	{
		compute_buffer_sizes(b, b+sp);
		compute_buffer_sizes(b+sp, e);
	}
	b[sp] = Splitter::out_buffer_size(pow_of_order<order>(static_cast<height_t>(e-b-sp)));
}

template<int order, class Splitter, class Pred, class Refiller, class Alloc>
void merge_tree<typename Alloc::pointer,order,Splitter,Pred,Refiller,Alloc>::construct(order_t k)
{
	if(!( order < k ))
		throw std::invalid_argument("Merge tree too small");
	height_t height = logc<order>(k);
	if( height > MAX_MERGE_TREE_HEIGHT )
		throw std::invalid_argument("Merge tree too high");
	bfs_index<order> bfs;
	for( height_t l=1; l!=logc<order>(k); ++l )
		bfs.child(0);
	leaf_index = bfs;
	size_type buffer_sizes[MAX_MERGE_TREE_HEIGHT];
	compute_buffer_sizes(buffer_sizes,buffer_sizes+height);

	last_stream = input_streams = salloc.allocate(k);
	SPtr p = input_streams;
	try
	{
		stream_t dflt;
		for( ; p!=last_stream+k; ++p )
			salloc.construct(p,dflt);

		try
		{
			root = nalloc.allocate(1);
			try
			{
				nalloc.construct(root, Node());
				root->construct(k,height,buffer_sizes+1,input_streams,alloc,nalloc);
			}
			catch( ... )
			{
				root->destroy(alloc, nalloc);
				nalloc.destroy(root);
				throw;
			}
		}
		catch( ... )
		{
			nalloc.deallocate(root,1);
			throw;
		}
	}
	catch( ... )
	{
		if( p != input_streams )
		{
			for( --p; p!=input_streams; --p )
				salloc.destroy(p);
			salloc.destroy(p);
		}
		salloc.deallocate(input_streams,k);
		throw;
	}
}

template<int order, class Splitter, class Pred, class Refiller, class Alloc>
merge_tree<typename Alloc::pointer,order,Splitter,Pred,Refiller,Alloc>::~merge_tree()
{
	for( SPtr s=input_streams; s!=input_streams+k; ++s )
		salloc.destroy(s);
	salloc.deallocate(input_streams, k);
	root->destroy(alloc, nalloc);
	nalloc.destroy(root);
	nalloc.deallocate(root,1);
}

template<int order, class Splitter, class Pred, class Refiller, class Alloc>
void merge_tree<typename Alloc::pointer,order,Splitter,Pred,Refiller,Alloc>::reset()
{
	cold = true;
	last_stream = input_streams;
}

template<int order, class Splitter, class Pred, class Refiller, class Alloc>
template<class FwIt>
FwIt merge_tree<typename Alloc::pointer,order,Splitter,Pred,Refiller,Alloc>::operator()(FwIt begin, FwIt end)
{
	if( cold )
		for( int i=0; i!=order; ++i )
			if( root->edges[i].child != NPtr() )
				go_down_cold(root->edges[i], comp);
	cold = false;
	return fill(root, begin, end, comp);
}

template<int order, class Splitter, class Pred, class Refiller, class Alloc>
template<class OutIt>
OutIt merge_tree<typename Alloc::pointer,order,Splitter,Pred,Refiller,Alloc>::operator()(OutIt begin)
{
	if( cold )
		for( int i=0; i!=order; ++i )
			if( root->edges[i].child != NPtr() )
				go_down_cold(root->edges[i], comp);
	cold = false;
	return fill(root, begin, comp);
}

template<int order, class Splitter, class Pred, class Refiller, class Alloc>
template<class FwIt>
FwIt merge_tree<typename Alloc::pointer,order,Splitter,Pred,Refiller,Alloc>::empty_buffers(NPtr n, FwIt begin, FwIt end)
{
	for( int i=0; i!=order; ++i )
	{
		for( ; n->edges[i].head!=n->edges[i].tail && begin!=end; ++n->edges[i].head, ++begin )
			*begin = *n->edges[i].head;
		if( n->edges[i].child )
			begin = empty_buffers(n->edges[i].child, begin, end);
	}
	return begin;
}

template<int order, class Splitter, class Pred, class Refiller, class Alloc>
template<class OutIt>
OutIt merge_tree<typename Alloc::pointer,order,Splitter,Pred,Refiller,Alloc>::empty_buffers(NPtr n, OutIt begin)
{
	for( int i=0; i!=order; ++i )
	{
		begin = std::copy(n->edges[i].head, n->edges[i].tail, begin);
		if( n->edges[i].child )
			begin = empty_buffers(n->edges[i].child, begin);
	}
	return begin;
}

template<int order, class Splitter, class Pred, class Refiller, class Alloc>
void merge_tree<typename Alloc::pointer,order,Splitter,Pred,Refiller,Alloc>::go_down(typename Node::Edge& e, Pred comp)
{
	assert( e.tail == e.head );
	assert( e.child != NPtr() );
	e.tail = fill(e.child, e.begin, e.tail, comp);
	e.head = e.begin;
}

template<int order, class Splitter, class Pred, class Refiller, class Alloc>
void merge_tree<typename Alloc::pointer,order,Splitter,Pred,Refiller,Alloc>::go_down_cold(typename Node::Edge& e, Pred comp)
{
	assert( e.child != NPtr() );
	e.tail = e.end, e.head = e.begin;
	warmup(e.child, e, comp);
	e.tail = e.head, e.head = e.begin;
}

template<int order, class Splitter, class Pred, class Refiller, class Alloc>
void merge_tree<typename Alloc::pointer,order,Splitter,Pred,Refiller,Alloc>::warmup(NPtr n, typename Node::Edge& output, Pred comp)
{
	for( int i=0; i!=order; ++i )
		if( n->edges[i].child != NPtr() )
			go_down_cold(n->edges[i], comp);
	output.head = fill(n, output.head, output.tail, comp);
}

template<int order, class Splitter, class Pred, class Refiller, class Alloc>
template<class FwIt>
FwIt merge_tree<typename Alloc::pointer,order,Splitter,Pred,Refiller,Alloc>::copy(typename Node::Edge& de, FwIt begin, FwIt end, Pred comp, std::forward_iterator_tag)
{
	for( ;; )
	{
		for( ; de.tail!=de.head && begin!=end; ++begin, ++de.head )
			*begin = *de.head;
		if( de.tail == de.head && de.child != NPtr() )
		{
			go_down(de, comp);
			if( de.head == de.tail )
				return begin;
		}
		else
			return begin;
	}
}

template<int order, class Splitter, class Pred, class Refiller, class Alloc>
template<class RIt>
RIt merge_tree<typename Alloc::pointer,order,Splitter,Pred,Refiller,Alloc>::copy(typename Node::Edge& de, RIt begin, RIt end, Pred comp, std::random_access_iterator_tag)
{
/*	for( ;; )
	{
		for( ; de.tail!=de.head && begin!=end; ++begin, ++de.head )
			*begin = *de.head;
		if( de.tail == de.head )
		{
			go_down(de, comp);
			if( de.head == de.tail )
				return begin;
		}
		else
			return begin;
	}
*/
	for( ;; )
	{
		if( end-begin < de.tail-de.head )
		{
			for( ; begin!=end; ++begin, ++de.head )
				*begin = *de.head;
			return begin;
		}
		else
		{
			for( ; de.tail!=de.head; ++begin, ++de.head )
				*begin = *de.head;
			if( de.child != NPtr() )
			{
				go_down(de, comp);
				if( de.head == de.tail )
					return begin;
			}
			else
				return begin;
		}
	}

}

template<int order, class Splitter, class Pred, class Refiller, class Alloc>
template<class OutIt>
OutIt merge_tree<typename Alloc::pointer,order,Splitter,Pred,Refiller,Alloc>::copy(typename Node::Edge& de, OutIt begin, Pred comp)
{
	do
	{
		for( ; de.tail!=de.head; ++begin, ++de.head )
			*begin = *de.head;
		if( de.child != NPtr() )
			go_down(de, comp);
		else
			break;
	}
	while( de.head != de.tail );
	return begin;
}

/*****
 *  Alloc::pointer iterator, order 2 basic merger template specialization
 */

template<class Splitter, class Pred, class Refiller, class Alloc>
class special_<typename Alloc::pointer,2,Splitter,Pred,Refiller,Alloc>
{
	friend class merge_tree<typename Alloc::pointer,2,Splitter,Pred,Refiller,Alloc>;

	template<class FwIt>
	static FwIt fill(typename merge_tree<typename Alloc::pointer,2,Splitter,Pred,Refiller,Alloc>::NPtr root, FwIt begin, FwIt end, Pred comp)
	{
		typedef typename merge_tree<typename Alloc::pointer,2,Splitter,Pred,Refiller,Alloc>::NPtr NPtr;
		typedef typename merge_tree<typename Alloc::pointer,2,Splitter,Pred,Refiller,Alloc>::TPtr TPtr;
		typedef typename merge_tree<typename Alloc::pointer,2,Splitter,Pred,Refiller,Alloc>::Node Node;
		for( ;; )
			if( root->edges[0].head != root->edges[0].tail )
				if( root->edges[1].head != root->edges[1].tail )
					for( TPtr l=root->edges[0].head, r=root->edges[1].head;; )
					{
						if( begin == end )
						{
							root->edges[0].head = l;
							root->edges[1].head = r;
							return begin;
						}
						if( comp(*l,*r) )
						{
							*begin = *l, ++l, ++begin;
							if( l == root->edges[0].tail )
							{
								typename Node::Edge& de = root->edges[0];
								root->edges[0].head = l; root->edges[1].head = r;
								if( de.child != NPtr() )
									merge_tree<typename Alloc::pointer,2,Splitter,Pred,Refiller,Alloc>::go_down(de, comp);
								if( de.head == de.tail )
									break;
								l = root->edges[0].head;
							}
						}
						else
						{
							*begin = *r, ++r, ++begin;
							if( r == root->edges[1].tail )
							{
								typename Node::Edge& de = root->edges[1];
								root->edges[0].head = l; root->edges[1].head = r;
								if( de.child != NPtr() )
									merge_tree<typename Alloc::pointer,2,Splitter,Pred,Refiller,Alloc>::go_down(de, comp);
								if( de.head == de.tail )
									break;
								r = root->edges[1].head;
							}
						}
					}
				else
					return merge_tree<typename Alloc::pointer,2,Splitter,Pred,Refiller,Alloc>::copy(root->edges[0], begin, end, comp, typename std::iterator_traits<FwIt>::iterator_category());
			else if( root->edges[1].head != root->edges[1].tail )
				return merge_tree<typename Alloc::pointer,2,Splitter,Pred,Refiller,Alloc>::copy(root->edges[1], begin, end, comp, typename std::iterator_traits<FwIt>::iterator_category());
			else
				return begin;
	}

	template<class OutIt>
	static OutIt fill(typename merge_tree<typename Alloc::pointer,2,Splitter,Pred,Refiller,Alloc>::NPtr root, OutIt begin, Pred comp)
	{
		typedef typename merge_tree<typename Alloc::pointer,2,Splitter,Pred,Refiller,Alloc>::NPtr NPtr;
		typedef typename merge_tree<typename Alloc::pointer,2,Splitter,Pred,Refiller,Alloc>::TPtr TPtr;
		typedef typename merge_tree<typename Alloc::pointer,2,Splitter,Pred,Refiller,Alloc>::Node Node;
		for( ;; )
			if( root->edges[0].head != root->edges[0].tail )
				if( root->edges[1].head != root->edges[1].tail )
					for( TPtr l=root->edges[0].head, r=root->edges[1].head;; )
						if( comp(*l,*r) )
						{
							*begin = *l, ++l, ++begin;
							if( l == root->edges[0].tail )
							{
								typename Node::Edge& de = root->edges[0];
								root->edges[0].head = l; root->edges[1].head = r;
								if( de.child != NPtr() )
									merge_tree<typename Alloc::pointer,2,Splitter,Pred,Refiller,Alloc>::go_down(de, comp);
								if( de.head == de.tail )
									break;
								l = root->edges[0].head;
							}
						}
						else
						{
							*begin = *r, ++r, ++begin;
							if( r == root->edges[1].tail )
							{
								typename Node::Edge& de = root->edges[1];
								root->edges[0].head = l; root->edges[1].head = r;
								if( de.child != NPtr() )
									merge_tree<typename Alloc::pointer,2,Splitter,Pred,Refiller,Alloc>::go_down(de, comp);
								if( de.head == de.tail )
									break;
								r = root->edges[1].head;
							}
						}
				else
					return merge_tree<typename Alloc::pointer,2,Splitter,Pred,Refiller,Alloc>::copy(root->edges[0], begin, comp);
			else if( root->edges[1].head != root->edges[1].tail )
				return merge_tree<typename Alloc::pointer,2,Splitter,Pred,Refiller,Alloc>::copy(root->edges[1], begin, comp);
			else
				return begin;
	}

};

/*****
 *  Alloc::pointer iterator, order 4 basic merger template specialization
 */

#define MOVE_SMALL_PTR(i,a)		*begin = *head[i], ++head[i], ++begin;  \
								if( head[i] == tail[i] )  \
								{  \
									stream[i]->head = head[i];  \
									if( stream[i]->child != NPtr() )  \
									{  \
										merge_tree<typename Alloc::pointer,4,Splitter,Pred,Refiller,Alloc>::go_down(*stream[i], comp);  \
										head[i] = stream[i]->head; tail[i] = stream[i]->tail;  \
									}  \
									if( head[i] == tail[i] )  \
									{  \
										head[i] = head[a-1];  \
										tail[i] = tail[a-1];  \
										stream[i] = stream[a-1];  \
										break;  \
									}  \
								}
#define MOVE_SMALL_PTR_(i,a)	*begin = *head[i], ++head[i], ++begin;  \
								if( head[i] == tail[i] )  \
									if( go_down<i,a>(stream,head,tail,comp) )  \
										break;  \

template<class Splitter, class Pred, class Refiller, class Alloc>
class special_<typename Alloc::pointer,4,Splitter,Pred,Refiller,Alloc>
{
	friend class merge_tree<typename Alloc::pointer,4,Splitter,Pred,Refiller,Alloc>;

	template<int i, int a>
	static bool go_down(typename merge_tree<typename Alloc::pointer,4,Splitter,Pred,Refiller,Alloc>::Node::Edge** stream, typename merge_tree<typename Alloc::pointer,4,Splitter,Pred,Refiller,Alloc>::TPtr* head, typename merge_tree<typename Alloc::pointer,4,Splitter,Pred,Refiller,Alloc>::TPtr* tail, Pred comp)
	{
		typedef typename merge_tree<typename Alloc::pointer,4,Splitter,Pred,Refiller,Alloc>::NPtr NPtr;
		stream[i]->head = head[i];
		if( stream[i]->child != NPtr() )
		{
			merge_tree<typename Alloc::pointer,4,Splitter,Pred,Refiller,Alloc>::go_down(*stream[i], comp);
			head[i] = stream[i]->head; tail[i] = stream[i]->tail;
		}
		if( head[i] == tail[i] )
		{
			head[i] = head[a-1];
			tail[i] = tail[a-1];
			stream[i] = stream[a-1];
			return true;
		}
		return false;
	}

	template<class FwIt>
	static FwIt fill(typename merge_tree<typename Alloc::pointer,4,Splitter,Pred,Refiller,Alloc>::NPtr n, FwIt begin, FwIt end, Pred comp)
	{
		typedef typename merge_tree<typename Alloc::pointer,4,Splitter,Pred,Refiller,Alloc>::NPtr NPtr;
		typedef typename merge_tree<typename Alloc::pointer,4,Splitter,Pred,Refiller,Alloc>::TPtr TPtr;
		typedef typename merge_tree<typename Alloc::pointer,4,Splitter,Pred,Refiller,Alloc>::Node::Edge Edge;
		TPtr head[4], tail[4];
		Edge *stream[4];
		order_t active = 0;
		for( Edge *p=n->edges; p!=n->edges+4; ++p )
			if( p->head != p->tail )
			{
				stream[active] = p;
				head[active] = p->head, tail[active] = p->tail;
				++active;
			}

		switch( active )
		{
		case 4:
			for( ;; )
			{
				if( begin == end )
				{
					stream[0]->head = head[0]; stream[1]->head = head[1]; stream[2]->head = head[2]; stream[3]->head = head[3];
					return begin;
				}
				if( comp(*head[0],*head[3]) )
				{
					if( comp(*head[0],*head[2]) )
					{
						if( comp(*head[0],*head[1]) )
						{
							MOVE_SMALL_PTR(0,4)
						}
						else
						{
							MOVE_SMALL_PTR(1,4)
						}
					}
					else
					{
						if( comp(*head[1],*head[2]) )
						{
							MOVE_SMALL_PTR(1,4)
						}
						else
						{
							MOVE_SMALL_PTR(2,4)
						}
					}
				}
				else
				{
					if( comp(*head[1],*head[3]) )
					{
						if( comp(*head[1],*head[2]) )
						{
							MOVE_SMALL_PTR(1,4)
						}
						else
						{
							MOVE_SMALL_PTR(2,4)
						}
					}
					else
					{
						if( comp(*head[2],*head[3]) )
						{
							MOVE_SMALL_PTR(2,4)
						}
						else
						{
							MOVE_SMALL_PTR(3,4)
						}
					}
				}
			}
		case 3:
			for( ;; )
			{
				if( begin == end )
				{
					stream[0]->head = head[0]; stream[1]->head = head[1]; stream[2]->head = head[2];
					return begin;
				}
				if( comp(*head[0],*head[2]) )
				{
					if( comp(*head[0],*head[1]) )
					{
						MOVE_SMALL_PTR(0,3)
					}
					else
					{
						MOVE_SMALL_PTR(1,3)
					}
				}
				else
				{
					if( comp(*head[1],*head[2]) )
					{
						MOVE_SMALL_PTR(1,3)
					}
					else
					{
						MOVE_SMALL_PTR(2,3)
					}
				}
			}
		case 2:
			for( ;; )
			{
				if( begin == end )
				{
					stream[0]->head = head[0]; stream[1]->head = head[1];
					return begin;
				}
				if( comp(*head[0],*head[1]) )
				{
					MOVE_SMALL_PTR(0,2)
				}
				else
				{
					MOVE_SMALL_PTR(1,2)
				}
			}
		case 1:
			stream[0]->head = head[0];
			begin = merge_tree<typename Alloc::pointer,4,Splitter,Pred,Refiller,Alloc>::copy(*stream[0], begin, end, comp, typename std::iterator_traits<FwIt>::iterator_category());
		case 0:
			return begin;
		default:
			assert( false );
			return begin;
		}
	}

	template<class OutIt>
	static OutIt fill(typename merge_tree<typename Alloc::pointer,4,Splitter,Pred,Refiller,Alloc>::NPtr n, OutIt begin, Pred comp)
	{
		typedef typename merge_tree<typename Alloc::pointer,4,Splitter,Pred,Refiller,Alloc>::NPtr NPtr;
		typedef typename merge_tree<typename Alloc::pointer,4,Splitter,Pred,Refiller,Alloc>::TPtr TPtr;
		typedef typename merge_tree<typename Alloc::pointer,4,Splitter,Pred,Refiller,Alloc>::Node::Edge Edge;
		TPtr head[4], tail[4];
		Edge *stream[4];
		order_t active = 0;
		for( Edge *p=n->edges; p!=n->edges+4; ++p )
			if( p->head != p->tail )
			{
				stream[active] = p;
				head[active] = p->head, tail[active] = p->tail;
				++active;
			}

		switch( active )
		{
		case 4:
			for( ;; )
				if( comp(*head[0],*head[3]) )
				{
					if( comp(*head[0],*head[2]) )
					{
						if( comp(*head[0],*head[1]) )
						{
							MOVE_SMALL_PTR(0,4)
						}
						else
						{
							MOVE_SMALL_PTR(1,4)
						}
					}
					else
					{
						if( comp(*head[1],*head[2]) )
						{
							MOVE_SMALL_PTR(1,4)
						}
						else
						{
							MOVE_SMALL_PTR(2,4)
						}
					}
				}
				else
				{
					if( comp(*head[1],*head[3]) )
					{
						if( comp(*head[1],*head[2]) )
						{
							MOVE_SMALL_PTR(1,4)
						}
						else
						{
							MOVE_SMALL_PTR(2,4)
						}
					}
					else
					{
						if( comp(*head[2],*head[3]) )
						{
							MOVE_SMALL_PTR(2,4)
						}
						else
						{
							MOVE_SMALL_PTR(3,4)
						}
					}
				}
		case 3:
			for( ;; )
				if( comp(*head[0],*head[2]) )
				{
					if( comp(*head[0],*head[1]) )
					{
						MOVE_SMALL_PTR(0,3)
					}
					else
					{
						MOVE_SMALL_PTR(1,3)
					}
				}
				else
				{
					if( comp(*head[1],*head[2]) )
					{
						MOVE_SMALL_PTR(1,3)
					}
					else
					{
						MOVE_SMALL_PTR(2,3)
					}
				}
		case 2:
			for( ;; )
				if( comp(*head[0],*head[1]) )
				{
					MOVE_SMALL_PTR(0,2)
				}
				else
				{
					MOVE_SMALL_PTR(1,2)
				}
		case 1:
			stream[0]->head = head[0];
			begin = merge_tree<typename Alloc::pointer,4,Splitter,Pred,Refiller,Alloc>::copy(*stream[0], begin, comp);
		case 0:
			return begin;
		default:
			assert( false );
			return begin;
		}
	}

};


} // namespace iosort
