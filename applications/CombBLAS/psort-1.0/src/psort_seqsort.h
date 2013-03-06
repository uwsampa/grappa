/*
Copyright (c) 2009, David Cheng, Viral B. Shah.

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

#ifndef PSORT_SEQSORT_H
#define PSORT_SEQSORT_H

#include "psort_util.h"

namespace vpsort {
  template<typename SeqSortType>
    class SeqSort {
    public:
    template<typename _ValueType, typename _Compare>
      void seqsort (_ValueType *first, _ValueType *last, _Compare comp) {

      SeqSortType *s = static_cast<SeqSortType *>(this);
      s->real_seqsort (first, last, comp);
    }

    char *description () {
      SeqSortType *s = static_cast<SeqSortType *>(this);
      return s->real_description ();
    }
  };

  class STLSort : public SeqSort<STLSort> {
  public:
    template<typename _ValueType, typename _Compare>
      void real_seqsort (_ValueType *first, _ValueType *last, _Compare comp) {

      std::sort (first, last, comp);
    }

    char *real_description () {
      std::string s ("STL sort");
	return const_cast<char*>(s.c_str());
    }
  };

  class STLStableSort : public SeqSort<STLStableSort> {
  public:
    template<typename _ValueType, typename _Compare>
      void real_seqsort (_ValueType *first, _ValueType *last, _Compare comp) {

      std::stable_sort (first, last, comp);
    }

    char *real_description () {
      std::string s("STL stable sort");
	return const_cast<char*>(s.c_str());

    }
  };

}

#endif /* PSORT_SEQSORT_H */

