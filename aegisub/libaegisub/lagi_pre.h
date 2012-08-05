#include "config.h"

#define LAGI_PRE

// Common C
#include <cassert>
#include <cerrno>
#include <climits>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <stdint.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
#else
#  include <ctime>
#endif

// Windows C
#ifdef _WIN32
// "Lean and mean" causes windows.h to include less stuff, mostly rarely-used things.
// We can't build without being "lean and mean", some of the things included by it has
// macros that clash with variable names around Aegisub causing strange build errors.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#endif

// Unix C
#ifndef _WIN32
#include <sys/statvfs.h>
#include <sys/param.h>
#endif

// Common C++
#include <algorithm>
#include <deque>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <list>
#include <locale>
#include <map>
#include <memory>
#include <numeric>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#ifdef __DEPRECATED // Dodge GCC warnings
# undef __DEPRECATED
# include <strstream>
# define __DEPRECATED
#else
# include <strstream>
#endif
