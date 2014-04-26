#pragma once

#include <Addressing.hpp>

template < typename T >                                                                                                
struct Relation {
    GlobalAddress<T> data;
    size_t numtuples;
};  
