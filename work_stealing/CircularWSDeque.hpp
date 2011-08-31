// Work stealing queue, based on a dynamically resizable circular array
// From D Chase, Y Lev. Dynamic Circular Workstealing deque.

#ifndef _CIRCULARWSDEQUE_HPP_
#define _CIRCULARWSDEQUE_HPP_

#include <stdint.h>
#include "CircularArray.hpp"

enum cwsd_status { CWSD_OK, CWSD_EMPTY, CWSD_ABORT };

template <class T>
class CircularWSDeque {
    private:
        volatile int64_t bottom;
        volatile int64_t top;
        CircularArray<T>* volatile activeArray;

        bool casTop(int64_t oldVal, int64_t newVal);

    public:
        CircularWSDeque();
        ~CircularWSDeque();

        void pushBottom(const T& obj);
        cwsd_status popBottom(T* element);
        cwsd_status steal(T* element);
        void perhapsShrink(int64_t b, int64_t t);
};



#define CWSD_INIT_LOGSIZE 7

// constant to decide shrink threshold; K >= 3
#define CWSD_RESIZE_K 4  

template <class T>
CircularWSDeque<T>::CircularWSDeque() 
    : bottom(0)
    , top(0)
    , activeArray(new CircularArray<T>(CWSD_INIT_LOGSIZE)) { }

template <class T>
CircularWSDeque<T>::~CircularWSDeque() { 
    delete activeArray;
}

template <class T>
bool CircularWSDeque<T>::casTop(int64_t oldVal, int64_t newVal) {
    return __sync_bool_compare_and_swap (&top, oldVal, newVal);
}

template <class T>
void CircularWSDeque<T>::pushBottom(const T& obj) {
    int64_t b = bottom;
    int64_t t = top;
    CircularArray<T>* a = activeArray;
    int64_t size = b - t;
    if (size >= a->size()-1) {
        a = a->grow(b, t);
        delete activeArray;  // TODO: could keep arrays in a pool
        activeArray = a;
    }
    a->put(b, obj);
    bottom = b + 1;
}

template <class T>
cwsd_status CircularWSDeque<T>::steal(T* element) {
    int64_t b = bottom;
    int64_t t = top;
    CircularArray<T>* a = activeArray;
    int64_t size = b - t;
    if (size <= 0) return CWSD_EMPTY;

    T result = a->get(t);

    // another stealer took a[t] before CAS succeeded
    if (!casTop(t, t+1)) return CWSD_ABORT;
     
    *element = result;
    return CWSD_OK;
}

template <class T>
cwsd_status CircularWSDeque<T>::popBottom(T* element) {
    int64_t b = bottom;
    CircularArray<T>* a = activeArray;
    b = b - 1;
    bottom = b;

    int64_t t = top;
    int64_t size = b - t;
    if (size < 0) {
        bottom = t;
        return CWSD_EMPTY;
    }

    *element = a->get(b);
    if (size > 0) {
        perhapsShrink(b, t);
        return CWSD_OK;
    }

    cwsd_status s = CWSD_OK;
    if (!casTop(t, t+1)) 
        return CWSD_EMPTY;

    bottom = t + 1;
    return s;
}

template <class T>
void CircularWSDeque<T>::perhapsShrink(int64_t b, int64_t t) {
    CircularArray<T>* a = activeArray;
    int64_t size = a->size();
    if ((size > (1<<CWSD_INIT_LOGSIZE)) && (b-t < size/CWSD_RESIZE_K)) {
        CircularArray<T>* aa = a->shrink(b, t);
        delete activeArray;
        activeArray = aa;
    }
}

#endif
