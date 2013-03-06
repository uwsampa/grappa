#ifndef _MY_ENABLE_IF_
#define _MY_ENABLE_IF_

namespace CombBLAS {
// ABAB: A simple enable_if construct for now
template <bool, class T = void> struct enable_if {};
template <class T> struct enable_if<true, T> { typedef T type; };

template <bool, class T = void> struct disable_if { typedef T type; };
template <class T> struct disable_if<true, T> {};

template <typename T> struct is_boolean { static const bool value = false; };
template <> struct is_boolean<bool> { static const bool value = true; };
}

#endif

