#ifndef _STACK_ENTRY_H
#define _STACK_ENTRY_H

#include <utility>
using namespace std;

template <class T1, class T2>
class StackEntry
{
public:
	T1 value;
	T2 key;
};

#endif
