// 4ary heap data structure
#ifndef HEAP2
#define HEAP2
#include "util.h"

template <class Key, class Value>
struct KNElement {Key key; Value value;};

// fixed size binary heap
template <class Key, class Value>
class Heap2 {
  //  static const Key infimum  = 4;
  //static const Key supremum = numeric_limits<Key>.max();
  typedef KNElement<Key, Value> Element;
  Element *data;
  int capacity;
  int supremum;
  int size;  // index of last used element
public:
  Heap2(Key sup, Key infimum, int cap):size(0),capacity(cap) { 
    data = new Element[cap + 2];
    data[0].key = infimum; // sentinel
    data[capacity + 1].key = sup;
    supremum = sup;
    reset();
  }
  ~Heap2() { delete data; } 
  Key getSupremum() { return supremum; }
  void reset();
  int   getSize()     const { return size; }
  Key   getMinKey()   const { return data[1].key; }
  Value getMinValue() const { return data[1].value; }
  void  deleteMinBasic();
  void  deleteMin(Key *key, Value *value) {
    *key   = getMinKey();
    *value = getMinValue();
    deleteMinBasic();
  }
  void  insert(Key k, Value v);
  void  sortTo(Element *to); // sort in increasing order and empty
  //void  sortInPlace(); // in decreasing order
};


// reset size to 0 and fill data array with sentinels
template <class Key, class Value>
inline void Heap2<Key, Value>::
reset() {
  size = 0;
  Key sup = getSupremum();
  int cap = capacity;
  for (int i = 1;  i <= cap;  i++) {
    data[i].key = sup;
  }
  // if this becomes a bottle neck
  // we might want to replace this by log KNN 
  // memcpy-s
}

template <class Key, class Value>
inline void Heap2<Key, Value>::
deleteMinBasic()
{
  Assert2(size > 0);

  // first move up elements on a min-path
  int hole = 1; 
  int succ = 2;
  int sz   = size;
  Element *dat = data;
  while (succ < sz) {
    Key key1 = dat[succ].key;
    Key key2 = dat[succ + 1].key;
    if (key1 > key2) {
      succ++;
      dat[hole].key   = key2;
      dat[hole].value = dat[succ].value;
    } else {
      dat[hole].key   = key1;
      dat[hole].value = dat[succ].value;
    }
    hole = succ;
    succ <<= 1;
  }

  // bubble up rightmost element
  Key bubble = dat[sz].key;
  int pred = hole >> 1;
  while (dat[pred].key > bubble) { // must terminate since min at root
    dat[hole] = dat[pred];
    hole = pred;
    pred >>= 1;
  }

  // finally move data to hole
  dat[hole].key = bubble;
  dat[hole].value = dat[sz].value;

  dat[size].key = getSupremum(); // mark as deleted
  size = sz - 1;
}


// empty the heap and put the element to "to"
// sorted in increasing order
template <class Key, class Value>
inline void Heap2<Key, Value>::
sortTo(Element *to)
{
  const int           sz = size;
  const Key          sup = getSupremum();
  Element * const beyond = to + sz;
  Element * const root   = data + 1;
  while (to < beyond) {
    // copy minimun
    *to = *root;
    to++;

    // bubble up second smallest as in deleteMin
    int hole = 1;
    int succ = 2;
    while (succ <= sz) {
      Key key1 = data[succ    ].key;
      Key key2 = data[succ + 1].key;
      if (key1 > key2) {
        succ++;
        data[hole].key   = key2;
        data[hole].value = data[succ].value;
      } else {
        data[hole].key = key1;
        data[hole].value = data[succ].value;
      }
      hole = succ;
      succ <<= 1;
    }

    // just mark hole as deleted
    data[hole].key = sup;
  }
  size = 0;
}


template <class Key, class Value>
inline void Heap2<Key, Value>::
insert(Key k, Value v)
{
  Assert2(size < capacity);
  Debug4(cout << "insert(" << k << ", " << v << ")" << endl);

  Element *dat = data;
  size++;
  int hole = size; 
  int pred = hole >> 1;
  Key predKey = dat[pred].key;
  while (predKey > k) { // must terminate due to sentinel at 0
    dat[hole].key   = predKey;
    dat[hole].value = dat[pred].value;
    hole = pred;
    pred >>= 1;
    predKey = dat[pred].key;
  }

  // finally move data to hole
  dat[hole].key   = k;
  dat[hole].value = v;
}

#endif
