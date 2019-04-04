#ifndef CONFIG_HPP
#define CONFIG_HPP

#define LOGGING
#define DG_LOG_LVL DG_LOG_LVL_INFO
#define DG_USE_COLORS 1
#include "debug_goodies.h"

#include <cstdint>
typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;
typedef float r32;
typedef double r64;
typedef size_t umm;
typedef ssize_t mm; // TODO: Sometimes ssize_t is not defined. Fallback to
                    //       something like ptrdiff_t?

inline i8 operator"" _i8(unsigned long long liter) { return static_cast<i8>(liter); }
inline u8 operator"" _u8(unsigned long long liter) { return static_cast<u8>(liter); }
inline i16 operator"" _i16(unsigned long long liter) { return static_cast<i16>(liter); }
inline u16 operator"" _u16(unsigned long long liter) { return static_cast<u16>(liter); }
inline i32 operator"" _i32(unsigned long long liter) { return static_cast<i32>(liter); }
inline u32 operator"" _u32(unsigned long long liter) { return static_cast<u32>(liter); }
inline i64 operator"" _i64(unsigned long long liter) { return static_cast<i64>(liter); }
inline u64 operator"" _u64(unsigned long long liter) { return static_cast<u64>(liter); }

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
#  define forceinline __attribute__((always_inline))
#elif defined COMPILER_MSVC
#  define forceinline __forceinline
#endif

namespace intr
{
#if defined COMPILER_GCC || defined COMPILER_CLANG
    static inline bool likely(bool expr_result)
    {
        return __builtin_expect(static_cast<bool>(expr_result), true);
    }

    static inline bool unlikely(bool expr_result)
    {
        return __builtin_expect(static_cast<bool>(expr_result), false);
    }

    static inline long expect(long value, long expected_value)
    {
        return __builtin_expect(value, expected_value);
    }

    static inline int clz32(u32 val)
    {
        static_assert(sizeof(unsigned int) == 4, "");
        return __builtin_clz(val);
    }

    static inline int clz64(u64 val)
    {
        static_assert(sizeof(unsigned long long) == 8, "");
        return __builtin_clzll(val);
    }

    static inline int ctz32(u32 val)
    {
        static_assert(sizeof(unsigned int) == 4, "");
        return __builtin_ctz(val);
    }

    static inline int ctz64(u64 val)
    {
        static_assert(sizeof(unsigned long long) == 8, "");
        return __builtin_ctzll(val);
    }

    static inline int popcnt32(u32 val)
    {
        static_assert(sizeof(unsigned int) == 4, "");
        return __builtin_popcount(val);
    }

    static inline int popcnt64(u64 val)
    {
        static_assert(sizeof(unsigned long long) == 8, "");
        return __builtin_popcountll(val);
    }
#elif defined COMPILER_MSVC
    // Looks like msvc does not support it as gcc does. There is __assume macro
    // but it works a bit differently.
    static inline bool likely(bool expr_result)
    {
        return expr_result;
    }

    static inline bool unlikely(bool expr_result)
    {
        return expr_result;
    }

    static inline long expect(long value, long expected_value)
    {
        ((void)(expected_value));
        return value;
    }

    // TODO: clz and clz can be achieved with bit scan forward.
    static inline int clz32(u32 val) { static_assert("clz32 not implemented"); }
    static inline int clz64(u64 val) { static_assert("clz64 not implemented"); }
    static inline int ctz32(u32 val) { static_assert("ctz32 not implemented"); }
    static inline int ctz64(u64 val) { static_assert("ctz64 not implemented"); }
    inline inline int popcnt32(u32 val) { return static_cast<int>(__popcnt(val)); }
    inline inline int popcnt64(u64 val) { return static_cast<int>(__popcnt64(val)); }
#else
#  error "Intrinsics probably won't work for You, as the compiler is unknown."
#endif
}

// Returns the number of elemensts in c-style array.
template<typename T, mm n>
constexpr mm
array_cnt(T (&)[n])
{
    return n;
}

// Returns the number of elemensts in the cstyle-string,
// DOES NOT COUNT NULL TERMINATOR. THATS THE DIFFERENCE BETWEEN array_cnt.
template<mm n>
constexpr mm
cstr_cnt(const char (&)[n])
{
    return n - 1;
}

#include <limits>
#include <type_traits>

// TODO: Enable if integral. Different for real numbers.
template<typename FROM_TYPE,
         typename TO_TYPE,
         typename = typename std::enable_if<std::is_integral<FROM_TYPE>::value>::type,
         typename = typename std::enable_if<std::is_integral<TO_TYPE>::value>::type>
static inline constexpr TO_TYPE
safe_trunc(FROM_TYPE val)
{
    constexpr mm max_from = static_cast<mm>(std::numeric_limits<FROM_TYPE>::max());
    constexpr mm max_to = static_cast<mm>(std::numeric_limits<TO_TYPE>::max());
    if constexpr (max_from < max_to)
        return static_cast<TO_TYPE>(val);

    ASSERT(val < max_to);
    return static_cast<TO_TYPE>(val);
}

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
// NOTE: Usage example:
//       while(1)
//       {
//           DO_ONCE()
//           {
//               printf("This is printed only once\n");
//           }
//           printf("This is printed every time\n");
//       }
#define DO_ONCE_IMPL(NAME)                                              \
    auto static NAME = false;                                           \
    if(!NAME && (NAME = true) == true)

#define DO_ONCE()                                                       \
    DO_ONCE_IMPL(ANON_NAME())

// Relational operators as a macro. While waiting for operator <=> this is
// pretty cool alternative.
#define REL_OPS(TYPE_)                                                  \
    inline bool operator!=(TYPE_ const& x, TYPE_ const& y)              \
    { return !(x == y); }                                               \
    inline bool operator>(TYPE_ const& x, TYPE_ const& y)               \
    { return (y < x); }                                                 \
    inline bool operator<=(TYPE_ const& x, TYPE_ const& y)              \
    { return !(y < x); }                                                \
    inline bool operator>=(TYPE_ const& x, TYPE_ const& y)              \
    { return !(x < y); }

#endif // CONFIG_HPP
