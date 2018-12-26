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

inline int8 operator"" _i8(unsigned long long liter) { return static_cast<int8>(liter); }
inline uint8 operator"" _u8(unsigned long long liter) { return static_cast<uint8>(liter); }
inline int16 operator"" _i16(unsigned long long liter) { return static_cast<int16>(liter); }
inline uint16 operator"" _u16(unsigned long long liter) { return static_cast<uint16>(liter); }
inline int32 operator"" _i32(unsigned long long liter) { return static_cast<int32>(liter); }
inline uint32 operator"" _u32(unsigned long long liter) { return static_cast<uint32>(liter); }
inline int64 operator"" _i64(unsigned long long liter) { return static_cast<int64>(liter); }
inline uint64 operator"" _u64(unsigned long long liter) { return static_cast<uint64>(liter); }

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
#  define COMPILER_MSVC 1
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

// Annonymus variable in c++?
#define CONCAT_IMPL(NAME1, NAME2) NAME1##NAME2
#define CONCAT(NAME1, NAME2) CONCAT_IMPL(NAME1, NAME2)
#ifdef __COUNTER__
#  define ANON_NAME_IMPL(NAME) CONCAT(NAME, __COUNTER__)
#else
#  define ANON_NAME_IMPL(NAME) CONCAT(NAME, __LINE__)
#endif
#define ANON_NAME() ANON_NAME_IMPL(ANONYMUS_VAR__)

// A horriebly bad macro encapsulating local static initailization.
#define DO_ONCE_IMPL(NAME)                                              \
    auto static NAME = false;                                           \
    if(!NAME && (NAME = true) == true)
#define DO_ONCE()                                                       \
    DO_ONCE_IMPL(ANON_NAME())

// Relational operators as a macro
#define REL_OPS(TYPE_)                                                  \
    inline bool operator!=(TYPE_ const& x, TYPE_ const& y)              \
    { return !(x == y); }                                               \
    inline bool operator>(TYPE_ const& x, TYPE_ const& y)               \
    { return y < x; }                                                   \
    inline bool operator<=(TYPE_ const& x, TYPE_ const& y)              \
    { return !(y < x); }                                                \
    inline bool operator>=(TYPE_ const& x, TYPE_ const& y)              \
    { return !(x < y); }

#endif //CONFIG_HPP
