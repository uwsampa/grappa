/****************************************************************/
/* Parallel Combinatorial BLAS Library (for Graph Computations) */
/* version 1.2 -------------------------------------------------*/
/* date: 10/06/2011 --------------------------------------------*/
/* authors: Aydin Buluc (abuluc@lbl.gov), Adam Lugowski --------*/
/****************************************************************/
/*
Copyright (c) 2011, Aydin Buluc

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifndef _DELETER_H_
#define _DELETER_H_

#include <iostream>
using namespace std;

struct DeletePtrIf
{
        template<typename T, typename _BinaryPredicate, typename Pred>
        void operator()(const T *ptr, _BinaryPredicate cond, Pred first, Pred second) const
        {
                if(cond(first,second))
                        delete ptr;
        }
};


template<typename A>
void DeleteAll(A arr1)
{
        delete [] arr1;
}

template<typename A, typename B>
void DeleteAll(A arr1, B arr2)
{
        delete [] arr2;
        DeleteAll(arr1);
}

template<typename A, typename B, typename C>
void DeleteAll(A arr1, B arr2, C arr3)
{
        delete [] arr3;
        DeleteAll(arr1, arr2);
}

template<typename A, typename B, typename C, typename D>
void DeleteAll(A arr1, B arr2, C arr3, D arr4)
{
        delete [] arr4;
        DeleteAll(arr1, arr2, arr3);
}


template<typename A, typename B, typename C, typename D, typename E>
void DeleteAll(A arr1, B arr2, C arr3, D arr4, E arr5)
{
        delete [] arr5;
        DeleteAll(arr1, arr2, arr3, arr4);
}

template<typename A, typename B, typename C, typename D, typename E, typename F>
void DeleteAll(A arr1, B arr2, C arr3, D arr4, E arr5, F arr6)
{
        delete [] arr6;
        DeleteAll(arr1, arr2, arr3, arr4,arr5);
}

#endif
