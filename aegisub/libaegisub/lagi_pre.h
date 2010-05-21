#define LAGI_PRE

// Common C
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <time.h>

// Windows C
#ifdef _MSC_VER
#include <io.h>
#endif

// Common C++
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

// Cajun
#include "libaegisub/cajun/elements.h"
#include "libaegisub/cajun/reader.h"
#include "libaegisub/cajun/visitor.h"
#include "libaegisub/cajun/writer.h"

