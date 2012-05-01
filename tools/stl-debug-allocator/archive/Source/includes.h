#ifndef INCLUDES_H_
#define INCLUDES_H_
/*
 * $Id: includes.h,v 1.1 2001/10/21 03:19:57 root Exp root $
 *
 * Include all needed STL and system headers.
 *
 * Also do some ugly tricks to silence off
 * compiler warnings due to sloppy coding in
 * the STL version that ships with MSVC 6.0
 *
 * I could as well name this file "stdafx.h"
 * but let's just say I live on the other side
 * of lake Washington :)
 */
#include <cassert>
#include <cerrno>

#include "msvcnowarn.h"
#include <algorithm>

#include "msvcnowarn.h"
#include <vector>
#include <iostream>

#if !defined(__SGI_STL_ALGORITHM)
 // The version of SGI STL shipped with
 // RedHat 7.1 does not have <limits>?
 // And I was harsh on Mr. Plauger...
 // looks like other have limits too :)
 #include <limits>
#endif

#include "CriticalSection.h"

#if defined (_MSC_VER) && !defined(__MWERKS__)
 #pragma warning(default: 4018)
 #pragma warning(default: 4245)
#endif
#endif // INCLUDES_H_

