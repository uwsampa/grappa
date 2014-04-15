#pragma once

#include <unordered_set>
using std::unordered_set;

template< typename T, size_t SmallSize = 8 >
class SmallLocalSet {
public:
  static const T INVALID = -1;
private:
  
  using OverflowSet = unordered_set<T>;
  
  T buf[SmallSize];
  size_t sz;
  OverflowSet * overflow;
  
public:
  
  SmallLocalSet(): sz(0), overflow(nullptr) {}
    
  struct iterator {
    union {
      typename OverflowSet::iterator it;
      const T * ptr;
    };
    bool overflow;
    iterator(typename OverflowSet::iterator it): it(it), overflow(true) {}
    iterator(const T * ptr): ptr(ptr), overflow(false) {}
    
    bool operator==(const iterator& o) const {
      assert(overflow == o.overflow);
      if (overflow) return o.it == it;
      else return o.ptr == ptr;
    }
    bool operator!=(const iterator& o) const { return !operator==(o); }
    
    T operator*() const { return overflow ? *it : *ptr; }
    
    iterator& operator++() {
      if (overflow) it++;
      else ptr++;
      return *this;
    }
    
  };
  
  iterator begin() const {
    if (overflow) return iterator(overflow->begin());
    else return iterator(&buf[0]);
  }
  iterator end() const {
    if (overflow) return iterator(overflow->end());
    else return iterator(buf+sz);
  }
  
  size_t count(T k) const {
    CHECK_LE(sz, SmallSize);
    if (overflow) return overflow->count(k);
    for (auto i=0; i<sz; i++) {
      if (buf[i] == k) return 1;
    }
    return 0;
  }
  
  void insert(T c) {
    if (overflow) overflow->insert(c);
    else {
      if (count(c)) return;
      
      if (sz < SmallSize) {
        buf[sz++] = c;
      } else {
        overflow = new unordered_set<T>();
        for (auto i=0; i<sz; i++) {
          overflow->insert(buf[i]);
        }
        overflow->insert(c);
      }
    }
  }
  
  size_t size() const { return overflow ? overflow->size() : sz; }
  bool empty() const { return size() == 0; }
  
  void dump(const char* name = nullptr, std::ostream& o = std::cout) const {
    if (name) o << name << ": ";
    if (overflow) o << "*";
    o << "{ ";
    for (T c : *this) {
      o << c << " ";
    }
    o << "}\n";
  }
  
};

template< typename T, size_t S >
inline T intersect_choose_random(const SmallLocalSet<T,S>& a, const SmallLocalSet<T,S>& b) {
  std::vector<T> intersection; //(std::min(a.size(), b.size()));
  for (T ca : a) {
    if (b.count(ca)) {
      intersection.push_back(ca);
    }
  }
  if (intersection.size() == 0) {
    return SmallLocalSet<T,S>::INVALID;
  } else {
    return intersection[rand() % intersection.size()];
  }
}
