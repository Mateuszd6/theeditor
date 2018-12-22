#ifndef CONFIG_HPP
#define CONFIG_HPP

#define LOGGING
#define DG_USE_COLORS 1
#include "debug_goodies.h"

#include <cstdint>
typedef int8_t int8;
typedef uint8_t uint8;
typedef int16_t int16;
typedef uint16_t uint16;
typedef int32_t int32;
typedef uint32_t uint32;
typedef int64_t int64;
typedef uint64_t uint64;
typedef float real32;
typedef double real64;
typedef size_t umm;
typedef ssize_t mm; // TODO: Sometimes ssize_t is not defined. Fallback to
                    // something like ptrdiff_t?

#define s_cast static_cast
#define r_cast reinterpret_cast
#define c_cast const_cast
#define dyn_cast dynamic_cast
#define dur_cast duration_cast

// Figure out the compiler.
#if (defined(__GNUC__) && !defined(__clang__))
#  define COMPILER_GCC 1
#elif defined(__clang__)
#  define COMPILER_CLANG 1
#elif defined (_MSC_VER)
#  define COMPILER_MSVC
#else
#  define COMPILER_UNKNOW 1
#endif

// Some commonly used intrinsics:

#if defined COMPILER_GCC || defined COMPILER_CLANG
#  define INTR_LIKELY(EXPR) (__builtin_expect(s_cast<bool>(EXPR), true))
#  define INTR_UNLIKELY(EXPR) (__builtin_expect(s_cast<bool>(EXPR), false))
#else // Looks like msvc does not support it.
#  define INTR_LIKELY(EXPR) (EXPR)
#  define INTR_UNLIKELY(EXPR) (EXPR)
#endif

#if defined COMPILER_GCC || defined COMPILER_CLANG
#  define forceinline __attribute__((always_inline))
#elif defined COMPILER_MSVC
#  define forceinline __forceinline
#endif

#if defined COMPILER_GCC || defined COMPILER_CLANG
inline int clz(int val) { return __builtin_clz(val); }
inline int clz(long val) { return __builtin_clzl(val); }
inline int clz(unsigned long long val) { return __builtin_clzll(val); }
inline int ctz(int val) { return __builtin_ctz(val); }
inline int ctz(long val) { return __builtin_ctzl(val); }
inline int ctz(unsigned long long val) { return __builtin_ctzll(val); }
inline int popcnt(int val) { return __builtin_popcount(val); }
inline int popcnt(long val) { return __builtin_popcountl(val); }
inline int popcnt(unsigned long long val) { return __builtin_popcountll(val); }
#elif defined COMPILER_MSVC
inline int popcnt(int32 val) { return s_cast<int>(__popcnt(val)); }
inline int popcnt(int64 val) { return s_cast<int>(__popcnt64(val)); }
#else
// TODO: Define some basics for moar compilerz.
#endif

template<typename T, mm n >
mm array_cnt( T (&)[n] ) { return n; }

#include <chrono>
namespace chrono = std::chrono;

#if __cplusplus >= 201402L
// Supress the clang warrning about namespace alias in the header.
#  if COMPILER_CLANG
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Weverything"
#  endif
using namespace std::chrono_literals;
#  if COMPILER_CLANG
#    pragma clang diagnostic pop
#  endif
#endif

#endif //CONFIG_HPP
