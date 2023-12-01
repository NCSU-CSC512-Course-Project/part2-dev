// Common.h
// ~~~~~~~~
// Common macros and other globals.
#include <clang-c/Index.h>

#define CXSTR(X) clang_getCString(X)

#define EXE_OUT "program.out"