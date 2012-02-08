// Resizable circular array.
// Based on Java source from D Chase, Y Lev. Dynamic Circular Workstealing deque.

#ifndef _CIRCULARARRAY_HPP_
#define _CIRCULARARRAY_HPP_

#include <stdint.h>
#include <stdlib.h>

template <class T>
class CircularArray {
    private:
        int log_size;
        T* segment;

   public:
        CircularArray(int log_size);
        ~CircularArray();
        int64_t size();
        T get(int64_t i);
        void put(int64_t i, const T& obj);

        // for these size changing methods, this array should be freed subsequently if no longer in use
        CircularArray<T>* grow(int64_t bottom, int64_t top);
        CircularArray<T>* shrink(int64_t bottom, int64_t top);
};

template<class T>
CircularArray<T>::CircularArray(int log_size)
    : log_size(log_size)
    , segment((T*)malloc(sizeof(T)*(1<<log_size))) { }

template<class T>
CircularArray<T>::~CircularArray() {
    free(segment);
}

template<class T>
int64_t CircularArray<T>::size() {
    return 1<<log_size;
}

template<class T>
T CircularArray<T>::get(int64_t i) {
    return segment[i % size()];
}

template<class T>
void CircularArray<T>::put(int64_t i, const T& obj) {
    segment[i % size()] = obj;
}

template<class T>
CircularArray<T>* CircularArray<T>::grow(int64_t bottom, int64_t top) {
    CircularArray<T>* a = new CircularArray<T>(log_size+1);
    for (int64_t i=top; i<bottom; i++) {
        a->put(i, get(i));
    }
    return a;
}

template<class T>
CircularArray<T>* CircularArray<T>::shrink(int64_t bottom, int64_t top) {
    CircularArray<T>* a = new CircularArray<T>(log_size-1);
    for (int64_t i=top; i<bottom; i++) {
        a->put(i, get(i));
    }
    return a;
}

#endif
