///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef EMGUITYPES_H 
#define EMGUITYPES_H  

#include <stddef.h>

typedef unsigned int uint;

#if defined(_WIN32)

#if defined(__GNUC__)
#include <stdint.h>
#else

typedef unsigned char uint8_t;
typedef signed char int8_t;

typedef unsigned short uint16_t;
typedef signed short int16_t;

typedef unsigned int uint32_t;
typedef signed int int32_t;

typedef unsigned __int64 uint64_t;
typedef signed __int64 int64_t;

typedef unsigned char bool;
typedef wchar_t text_t;

#ifndef true
#define true	1
#endif

#ifndef false
#define false	0
#endif

#pragma warning(disable: 4100 4127 4201 4706)
#endif

#define EMGUI_LIKELY(exp) exp 
#define EMGUI_UNLIKELY(exp) exp 
#define EMGUI_INLINE __forceinline
#define EMGUI_RESTRICT __restrict
#define EMGUI_ALIGN(x) __declspec(align(x))
#define EMGUI_UNCACHEDAC_PTR(x)	x
#define EMGUI_UNCACHED_PTR(x) x
#define EMGUI_CACHED_PTR(x) x 
#define EMGUI_ALIGNOF(t) __alignof(t)
#define EMGUI_BREAK __debugbreak()

#if defined(_WIN64)
#define EMGUI_X64 
#endif

#elif defined(EMGUI_UNIX) || defined(EMGUI_MACOSX)

#include <stdint.h>

#define EMGUI_LIKELY(exp) __builtin_expect(exp, 1) 
#define EMGUI_UNLIKELY(exp) __builtin_expect(exp, 0) 
#define EMGUI_INLINE inline
#define EMGUI_RESTRICT __restrict
#define EMGUI_ALIGN(x) __attribute__((aligned(x)))
#define EMGUI_UNCACHEDAC_PTR(x)	x
#define EMGUI_UNCACHED_PTR(x) x
#define EMGUI_CACHED_PTR(x) x 
#define EMGUI_ALIGNOF(t) __alignof__(t)
#define EMGUI_BREAK ((*(volatile uint32_t *)(0)) = 0x666)

#define _T(v) v

typedef char text_t;

#include <stdbool.h>

#ifndef true
#define true	1
#endif

#ifndef false
#define false	0
#endif

#if defined(__LP64__)
#define EMGUI_X64
#endif

#else
#error Platform not supported
#endif

#define sizeof_array(array) (int)(sizeof(array) / sizeof(array[0]))

#endif
