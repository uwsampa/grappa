// Binary heaps using a straightforward implementation a la CLR
#ifndef HEAP2
#define HEAP2
// #include "util.h"

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
  void print() {
    for (int i = 1;  i <= size;  i++) {
      cout << data[i].key << " ";
    }
    cout << endl;
  }
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
{ int i, l, r, smallest;
  Assert2(size > 0);

  data[1] = data[size];
  data[size].key = getSupremum();
  size--;
  i = 1;
  for (;;) {
    Debug3(print());
    l = (i << 1);
    r = (i << 1) + 1;
    if ((l <= size) && data[l].key < data[i].key) {
      smallest = l;
    } else {
      smallest = i;
    }
    if ((r <= size) && data[r].key < data[smallest].key) {
      smallest = r;
    }
    if (smallest == i) break;
    Element temp = data[i];
    data[i] = data[smallest];
    data[smallest] = temp;
    i = smallest;
  }
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

  size++;
  int hole = size; 
  while (hole > 1 && data[hole >> 1].key > k) {
    data[hole] = data[hole >> 1];
    hole = hole >> 1;
  }
  data[hole].key   = k;
  data[hole].value = v;
}

#endif
