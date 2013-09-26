#include <iostream>
#include <string>
#include <utility>
#include <map>
#include <cstdio>
#include <vector>
#include <sys/file.h>
#include <stdint.h>
#include <algorithm>


using namespace std;

template<typename _ForwardIterator>
bool is_sorted(_ForwardIterator __first, _ForwardIterator __last)
{
	if (__first == __last)
		return true;

	_ForwardIterator __next = __first;
	for (++__next; __next != __last; __first = __next, ++__next)
		if (*__next < *__first)
			return false;
	return true;
}

template<typename _ForwardIter, typename T>
void iota(_ForwardIter __first, _ForwardIter __last, T __value)
{
	while (__first != __last)
		*__first++ = __value++;
}


int main()
{
	int64_t nverts = 133633040;
	int64_t nedges = 5507679822;
	FILE * infp = fopen("uk-union.graph.bin", "rb");
	
	uint32_t * gen_edges = new uint32_t[2*nedges];
	fread(gen_edges, 2*nedges, sizeof(uint32_t), infp);
	cout << "Read data" << endl;	

	vector< vector<uint32_t> > adj(nverts);
	vector< uint32_t > shuffler(nverts);
	iota(shuffler.begin(), shuffler.end(), static_cast<uint32_t>(0));
	random_shuffle ( shuffler.begin(), shuffler.end() );

	for(int64_t i=0; i< 2*nedges; i+=2)
	{
		 adj[shuffler[gen_edges[i]]].push_back(shuffler[gen_edges[i+1]]);
		 adj[shuffler[gen_edges[i+1]]].push_back(shuffler[gen_edges[i]]);
	}
	cout << "Made adjacencies" << endl;
	delete [] gen_edges;
	
	FILE * outp = fopen("uk-union.symmetric.bin", "wb");

	int64_t count = 0;
	for(int64_t i=0; i< nverts; ++i)
	{
		uint32_t edge[2];
		edge[0] = i;
		sort(adj[i].begin(), adj[i].end());
		vector<uint32_t>::iterator p_end = unique(adj[i].begin(), adj[i].end()); // move consecutive duplicates past the end, store the new end
		for(vector<uint32_t>::iterator p = adj[i].begin(); p != p_end; ++p)
		{
			edge[1] = *p;
			fwrite(edge, 1, 2*sizeof(uint32_t), outp);
			++count;
		}
	}
	fclose(outp);
	cout << "Number of edges: " << count << endl;
}

