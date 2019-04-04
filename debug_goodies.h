// Define 'DEBUG' to enable:
// * BREAK()
// * PANIC(MSG, ...)
// * ASSERT(EXPR)
// * ALWAYS_ASSERT(EXPR)
// * UNREACHABLE()

// Define 'BENCHMARK' to enable:
// * PUSH_TIMER(NAME)
// * POP_TIMER()
// * RUN_BENCHMARK(NAME, REPEAT, EXPR)
// * BENCHMARK_BEFORE_MAIN(NAME, REPEAT, EXPR)

// Define 'LOGGING' to enable:
// * LOG_TRACE (Only if 'DEBUG' is defined) LOG_LVL 0+
// * LOG_DEBUG (Only if 'DEBUG' is defined) LOG_LVL 0+
// * LOG_INFO                               LOG_LVL 0+
// * LOG_WARN                               LOG_LVL 1+
// * LOG_ERROR                              LOG_LVL 2+
// * LOG_FATAL                              LOG_LVL 2+

// Other definable stuff:
// * DG_USE_DATE (To add date & time to your logs)
// * DG_FPRINTF (If you don't want to use fprintf)
// * DG_BENCHMARK_FILE (To redirect benchmarks results)
// * DG_LOG_FILE (To redirect logs to the file, not stderr)
// * DG_TIME_SCOPE_MSG(NAME, TIME) (Logic when time scope ends)
// * DG_BENCHMARK_MSG(NAME, REPEAT, BEST, WORST, AVG) (Logic after benchmark)
// * DG_DEBUG_PANIC_BEHAVIOUR(MSG, ...) (Panic logic if defined DEBUG)
// * DG_RELEASE_PANIC_BEHAVIOUR(MSG, ...) (Panic logic if not defined DEBUG)
// * DG_USE_COLORS - if not 0, output will be displayed with some ASCII escape color codes.
// * DG_LOG_LVL - This defines the verbosity level for the logging API.
//       Possible values (If not defined, default is the first one):
//         DG_LOG_LVL_ALL   - Display all messages.
//         DG_LOG_LVL_INFO  - Display only infos, warrnings and errors.
//         DG_LOG_LVL_WARN  - Display only warrnings and errors.
//         DG_LOG_LVL_ERROR - Display errors only.


// TODO: Add ability not to use chrono.

#ifndef DEBUG_GOODIES_H
#define DEBUG_GOODIES_H

// ------------------- AUXILIARY -------------------

#define DG_INT_CONCAT_IMPL(NAME1, NAME2) NAME1##NAME2
#define DG_INT_CONCAT(NAME1, NAME2) DG_INT_CONCAT_IMPL(NAME1, NAME2)

#ifdef __COUNTER__
  #define DG_INT_ANON_NAME(NAME) DG_INT_CONCAT(NAME, __COUNTER__)
#else
  #define DG_INT_ANON_NAME(NAME) DG_INT_CONCAT(NAME, __LINE__)
#endif

// ------------------- PLATFORM AND DEFAULTS -------------------

#define DG_INT_RED_COLOR_ID 31
#define DG_INT_YELLOW_COLOR_ID 33

// Cross platform debug break.
// TODO: Tests it on clang under WIN32 machine.
#if defined(__clang__)
  #include <signal.h>
  #define DG_INT_BREAK() raise(SIGTRAP)
#elif defined(__GNUC__) || defined(__GNUG__)
  #include <signal.h>
  #define DG_INT_BREAK() raise(SIGTRAP)
#elif defined(_MSC_VER)
  #define DG_INT_BREAK() __debugbreak()
#endif

// Define 'DG_FPRINTF' if you dont want to use fprintf.
#ifndef DG_FPRINTF
  #include <stdio.h>
  #define DG_FPRINTF fprintf
#endif

// Define 'DG_BENCHMARK_FILE' for benchmarks output. Default is stdout.
#ifndef DG_BENCHMARK_FILE
  #include <stdio.h>
  #define DG_BENCHMARK_FILE stdout
#endif

// Define 'DG_LOG_FILE' to set log output. Default is stderr.
#ifndef DG_LOG_FILE
  #include <stdio.h>
  #define DG_LOG_FILE stderr
#endif

// Define 'DG_USE_DATE' for time info in logs, requires chorno
#ifdef DG_USE_DATE
  #include <chrono>
  #define DG_INT_DATETIME_TO_FILE(_FILE)                                 \
      do {                                                               \
          auto in_time_t =                                               \
              std::chrono::system_clock::to_time_t(                      \
                  std::chrono::system_clock::now());                     \
          auto tm_info = localtime(&in_time_t);                          \
                                                                         \
          DG_FPRINTF(                                                    \
              _FILE,                                                     \
              "%02d-%02d-%d %02d:%02d:%02d: ",                           \
              tm_info->tm_year + 1900, tm_info->tm_mon + 1,              \
              tm_info->tm_mday, tm_info->tm_hour,                        \
              tm_info->tm_min, tm_info->tm_sec);                         \
      } while(0)
#else
  #define DG_INT_DATETIME_TO_FILE(IGNORE) ((void)0)
#endif

#ifndef DG_USE_COLORS
  #define DG_USE_COLORS 0
#endif

#define DG_INT_START_COLOR(COLOR_ID)                                     \
  do {                                                                   \
      if (COLOR_ID != -1 && DG_USE_COLORS)                               \
      {                                                                  \
          DG_FPRINTF(DG_LOG_FILE, "\033[1;%sm", #COLOR_ID);              \
      }                                                                  \
  } while(0)

#ifdef DG_USE_COLORS
  #define DG_INT_RESET_COLOR()                                           \
    do {                                                                 \
        if (DG_USE_COLORS)                                               \
        {                                                                \
            DG_FPRINTF(DG_LOG_FILE, "\033[0m");                          \
        }                                                                \
    } while(0)
#endif

#define DG_INT_LOG(MSG, LOG_TYPE, COLOR_ID, ...)                         \
    do {                                                                 \
        DG_FPRINTF(DG_LOG_FILE, "%s:%d: ", __FILE__, __LINE__);          \
        DG_INT_START_COLOR(COLOR_ID);                                    \
        DG_FPRINTF(DG_LOG_FILE, "%s:", #LOG_TYPE);                       \
        DG_INT_RESET_COLOR();                                            \
        DG_FPRINTF(DG_LOG_FILE, " ");                                    \
        DG_INT_DATETIME_TO_FILE(DG_LOG_FILE);                            \
        DG_FPRINTF(DG_LOG_FILE, MSG, ##__VA_ARGS__);                     \
        DG_FPRINTF(DG_LOG_FILE, "\n");                                   \
    } while(0)

// Default panic behaviour. If either 'DG_DEBUG_PANIC_BEHAVIOUR' or
// 'DG_RELEASE_PANIC_BEHAVIOUR' are not defined this is the default.
#define DG_INT_DEFAULT_PANIC_BEHAVIOUR(MSG, ...)                         \
    do {                                                                 \
        DG_INT_LOG(MSG, PANIC, DG_INT_RED_COLOR_ID, ##__VA_ARGS__);      \
    } while(0)

// Panic macro defaults.
// These can be redefined to do whatever user wants.
#ifndef DG_DEBUG_PANIC_BEHAVIOUR
  #define DG_DEBUG_PANIC_BEHAVIOUR(MSG, ...)                             \
      DG_INT_DEFAULT_PANIC_BEHAVIOUR(MSG, ##__VA_ARGS__)
#endif

#ifndef DG_RELEASE_PANIC_BEHAVIOUR
  #define DG_RELEASE_PANIC_BEHAVIOUR(MSG, ...)                           \
      DG_INT_DEFAULT_PANIC_BEHAVIOUR(MSG, ##__VA_ARGS__)
#endif

// Logic that takes place when single 'POP_TIMER' is done.
#ifndef DG_TIME_SCOPE_MSG
  #define DG_TIME_SCOPE_MSG(NAME, TIME)                                  \
    DG_INT_LOG("%s - %.3fs", "TIMED: ", NAME, -1, (float)TIME / 1000)
#endif

// Logic that takes place when after the benchmark is done.
#ifndef DG_BENCHMARK_MSG
  #define DG_BENCHMARK_MSG(NAME, REPEAT, BEST, WORST, AVG)               \
    DG_FPRINTF(DG_BENCHMARK_FILE,                                        \
      "%s:%d: BENCH: %s(x%d) - AVG: %.3fs, BEST: %.3fs, WORST: %.3fs\n", \
        __FILE__, __LINE__, NAME, REPEAT,                                \
        (float)AVG / 1000, (float)BEST / 1000, (float)WORST / 1000)
#endif

// ------------------- DEBUG BREAK -------------------

#define BREAK() DG_INT_BREAK()

// ------------------- ASSERTIONS -------------------

/// PANIC (string formatted).
// This is compiled with a release build and is used to give a
// error feedback to the user. In debug build, its just break.
#ifdef DEBUG
  #define PANIC(MSG, ...)                                                \
      do {                                                               \
          DG_DEBUG_PANIC_BEHAVIOUR(MSG, ##__VA_ARGS__);                  \
          BREAK();                                                       \
      } while(0)
#else
  #define PANIC(MSG, ...)                                                \
      do {                                                               \
          DG_RELEASE_PANIC_BEHAVIOUR(MSG, ##__VA_ARGS__);                \
      } while(0)
#endif

/// ASSERT (not string formatted).
// Make a debug break in debug mode, and ignore this statement
// in release mode.
#ifdef DEBUG
  #define ASSERT(EXPR)                                                   \
      do {                                                               \
          if (!(EXPR))                                                   \
          {                                                              \
              DG_INT_LOG("%s", ASSERT_FAIL, DG_INT_RED_COLOR_ID, #EXPR); \
              BREAK();                                                   \
          }                                                              \
      } while(0)
#else
  #define ASSERT(IGNORE) ((void)0)
#endif

/// RUNTIME ASSERT.
// Assertion that is evaluated at runtime. In release mode, if
// EXPR is false, macro is expanded into PANIC, in debug mode,
// this is just regular asser.
#ifdef DEBUG
  #define ALWAYS_ASSERT(EXPR)                                            \
      do {                                                               \
          ASSERT(EXPR);                                                  \
      } while(0)
#else
  #define ALWAYS_ASSERT(EXPR)                                            \
      do {                                                               \
          if (!(EXPR)) { PANIC("Assertion broken: %s", #EXPR); }       \
      } while(0)
#endif

/// UNREACHABLE.
// Marks code as unreachable. In debug mode this causes panic,
// in release marks as unreachable using __builtin_unreachable
#ifdef DEBUG
  #define UNREACHABLE() PANIC("Unreachable code reached!")
#else
  #define UNREACHABLE() __builtin_unreachable()
#endif

// ------------------- BENCHMARK -------------------

// TODO: Add version that does not depend on chrono.
#ifdef BENCHMARK
  #include <chrono>
#endif

// Auxiliary structure for time queries.
#ifdef BENCHMARK
  namespace debug {
      namespace detail {
          struct query_counter
          {
              // Identifier of the query,
              // e.g. name of the function.
              char *name;

              // Start time.
              std::chrono::time_point<std::chrono::system_clock>
              timer;
          };

          int index = 0;
          query_counter counters_stack[32];
      }
  }
#endif

// Start performance timer. Does nothing if 'BENCHMARK'
// is not defined.
#ifdef BENCHMARK
  #define PUSH_TIMER(NAME)                                               \
      do {                                                               \
          debug::detail::counters_stack[debug::detail::index].name=NAME; \
          debug::detail::counters_stack[debug::detail::index++].timer =  \
                std::chrono::system_clock::now();                        \
      } while(0)
#else
  #define PUSH_TIMER(NAME) ((void)0)
#endif

// Stop timer and run the report function.
// Does nothing if 'BENCHMARK' is not defined.
#ifdef BENCHMARK
  #define POP_TIMER()                                                    \
      do {                                                               \
          debug::detail::index--;                                        \
          std::chrono::milliseconds diff =                               \
              std::chrono::duration_cast<std::chrono::milliseconds>(     \
                  std::chrono::system_clock::now() -                     \
                  debug::detail::counters_stack[debug::detail::index]    \
                      .timer);                                           \
          DG_TIME_SCOPE_MSG(                                             \
              debug::detail::counters_stack[debug::detail::index].name,  \
              diff.count());                                             \
      } while(0)

#else
  #define POP_TIMER() ((void)0)
#endif

// Repeat 'REPEAT' times benchark and report results.
// Does nothing if benchmark is not defined.
#ifdef BENCHMARK
  #define RUN_BENCHMARK(NAME, REPEAT, EXPR)                              \
      do {                                                               \
          unsigned long long best = 0, worst = 0, sum = 0;               \
          for (int i = 0; i < REPEAT; ++i)                               \
          {                                                              \
              auto _bench_start_ = std::chrono::system_clock::now();     \
              { EXPR; }                                                  \
              std::chrono::milliseconds _bench_diff_ =                   \
                  std::chrono::duration_cast<std::chrono::milliseconds>( \
                      std::chrono::system_clock::now() - _bench_start_); \
                                                                         \
              unsigned long long _bench_res_ = _bench_diff_.count();     \
              best = _bench_res_ < best || best == 0                     \
                                   ? _bench_res_ : best;                 \
              worst = _bench_res_ > worst ? _bench_res_ : worst;         \
              sum += _bench_res_;                                        \
          }                                                              \
          float avg = sum / REPEAT;                                      \
          DG_BENCHMARK_MSG(NAME, REPEAT, best, worse, avg);              \
      } while (0);
#else
  #define RUN_BENCHMARK(IGNORE1, IGNORE2, IGNORE3) ((void)0)
#endif

#ifdef BENCHMARK
#define DG_INT_BENCHMARK_BEFORE_MAIN_AUX(NAME, BNAME, REPEAT, EXPR)      \
     struct NAME                                                         \
     {                                                                   \
         NAME()                                                          \
         {                                                               \
             unsigned long long best = 0, worst = 0, sum = 0;            \
             for (int i = 0; i < REPEAT; ++i)                            \
             {                                                           \
                 auto _bench_start_ = std::chrono::system_clock::now();  \
                 { EXPR; }                                               \
                 std::chrono::milliseconds _bench_diff_ =                \
                     std::chrono::duration_cast<                         \
                     std::chrono::milliseconds>(                         \
                         std::chrono::system_clock::now()                \
                         - _bench_start_);                               \
                                                                         \
                 unsigned long long _bench_res_ = _bench_diff_.count();  \
                 best = _bench_res_ < best || best == 0                  \
                                      ? _bench_res_ : best;              \
                 worst = _bench_res_ > worst ? _bench_res_ : worst;      \
                 sum += _bench_res_;                                     \
             }                                                           \
             float avg = sum / REPEAT;                                   \
             DG_BENCHMARK_MSG(BNAME, REPEAT, best, worst, avg);          \
         }                                                               \
     }; NAME DG_INT_ANON_NAME(___ANON_VARIABLE_NAME___)
#endif

#ifdef BENCHMARK
  #define BENCHMARK_BEFORE_MAIN(NAME, REPEAT, EXPR)                      \
      DG_INT_BENCHMARK_BEFORE_MAIN_AUX(                                  \
        DG_INT_ANON_NAME(___ANON_VARIABLE_NAME___), NAME, REPEAT, EXPR)
#else
  #define BENCHMARK_BEFORE_MAIN(NAME, REPEAT, EXPR) ;
#endif

// ------------------- LOGGING -------------------

// Define to display all logs.
#define DG_LOG_LVL_ALL (0)
#define DG_LOG_LVL_INFO (1)
#define DG_LOG_LVL_WARN (2)
#define DG_LOG_LVL_ERROR (3)

#ifndef DG_LOG_LVL
  #define DG_LOG_LVL (0)
#endif

// TODO: Summary these!
#define DG_INT_LOG_AUX(MSG, TYPE, LVL, CURR_LVL, COLOR_ID, ...)          \
    do {                                                                 \
        if ((LVL) >= (CURR_LVL)) {                                       \
            DG_INT_LOG(MSG, TYPE, COLOR_ID, ##__VA_ARGS__);              \
        }                                                                \
    } while(0)

#ifdef LOGGING
  #define LOG_TRACE(MSG, ...) DG_INT_LOG_AUX(MSG, TRACE, DG_LOG_LVL_ALL, DG_LOG_LVL, -1, ##__VA_ARGS__)
  #define LOG_DEBUG(MSG, ...) DG_INT_LOG_AUX(MSG, DEBUG, DG_LOG_LVL_ALL, DG_LOG_LVL, -1, ##__VA_ARGS__)
  #define LOG_INFO(MSG, ...) DG_INT_LOG_AUX(MSG, INFO, DG_LOG_LVL_INFO, DG_LOG_LVL, -1, ##__VA_ARGS__)
  #define LOG_WARN(MSG, ...) DG_INT_LOG_AUX(MSG, WARN, DG_LOG_LVL_WARN, DG_LOG_LVL, DG_INT_YELLOW_COLOR_ID, ##__VA_ARGS__)
  #define LOG_ERROR(MSG, ...) DG_INT_LOG_AUX(MSG, ERROR, DG_LOG_LVL_ERROR, DG_LOG_LVL, DG_INT_RED_COLOR_ID, ##__VA_ARGS__)
  #define LOG_FATAL(MSG, ...) DG_INT_LOG_AUX(MSG, FATAL, DG_LOG_LVL_ERROR, DG_LOG_LVL, DG_INT_RED_COLOR_ID, ##__VA_ARGS__)
#endif

#endif // DEBUG_GOODIES_H
