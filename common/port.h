/* Platform-specific compatibility hacks.
 * The idea is to consolodate all the ifdef hackery into one place so
 * code can be simple elsewhere.
 */

#ifndef PORT_H
#define PORT_H

//#if defined(_MSC_VER)
//#   define DEPRECATED __declspec(deprecated)
//// kill microsoft deprecation warnings for C/C++ standard libs that aren't deprecated --Swordsman
//#   pragma warning(disable : 4996)
//#else
//#   define DEPRECATED
//#endif

#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

// signed/unsigned ints of whatever size.
typedef unsigned uint;
//typedef ptrdiff_t sint;

//#if defined(_MSC_VER) && _MSC_VER < 1300
//    // Silence stupid not-really-a-warning. (identifier too long for debugger hurk blah blah etc)
//#   pragma warning (disable:4786)
//    // Fix broken for() scoping in VC6
//#   define for if (0); else for
//#endif

// Calling convention poocrap.  Needed in a handful of places.
#ifdef WIN32
#   define IKA_STDCALL __stdcall
#else
#   define IKA_STDCALL
#endif

#endif
