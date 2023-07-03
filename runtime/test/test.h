#ifndef QDBP_TEST_H
#define QDBP_TEST_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct _qdbptest_suite_results {
  size_t passed;
  size_t failed;
};

static void _qdbptest_log(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
}

#define TEST_SUITE(name)                                                   \
  struct _qdbptest_suite_results _qdbptest_suite_##name(void) {            \
    const char* _qdbptest_suite_name = #name;                              \
    const char* _qdbptest_case_name = "";                                  \
    struct _qdbptest_suite_results _qdbptest_suite_results = {.passed = 0, \
                                                              .failed = 0};

#define TEST_SUITE_END()          \
  return _qdbptest_suite_results; \
  }

#define TEST_CASE(name)                                           \
  for (bool _qdbptest_bool = (_qdbptest_case_name = #name, true); \
       _qdbptest_bool; _qdbptest_bool = false, _qdbptest_case_name = "")

#define DECLARE_SUITE(name) \
  extern struct _qdbptest_suite_results _qdbptest_suite_##name(void);

// TODO: Report case pass/fail instead?
#define RUN_SUITE(name)                                        \
  do {                                                         \
    struct _qdbptest_suite_results _qdbptest_suite_results =   \
        _qdbptest_suite_##name();                              \
    _qdbptest_log("Suite %s: %zu checks passed, %zu checks failed\n", #name, \
                  _qdbptest_suite_results.passed,              \
                  _qdbptest_suite_results.failed);             \
  } while (0)

#define DECLARE_AND_RUN_SUITE(name) \
  DECLARE_SUITE(name)               \
  RUN_SUITE(name)

#define _QDBPTEST_LOG_FAIL_NO_NEWLINE()                               \
  _qdbptest_log("Test \"%s/%s\" failed: %s:%d", _qdbptest_suite_name, \
                _qdbptest_case_name, __FILE__, __LINE__);             \
  _qdbptest_suite_results.failed++;

#define _QDBPTEST_LOG_FAIL()       \
  _QDBPTEST_LOG_FAIL_NO_NEWLINE(); \
  _qdbptest_log("\n");

#define _QDBPTEST_EXIT_SUITE(exit) \
  if (exit) {                      \
    break;                         \
  }

#define _QDBPTEST_TEST(cond, exitonfailure) \
  do {                                      \
    if (!(cond)) {                          \
      _QDBPTEST_LOG_FAIL();                 \
      _QDBPTEST_EXIT_SUITE(exitonfailure);  \
    } else {                                \
      _qdbptest_suite_results.passed++;     \
    }                                       \
  } while (0)

#define _QDBPTEST_TEST_MSG(cond, exitonfailure, msg) \
  do {                                               \
    if (!(cond)) {                                   \
      _QDBPTEST_LOG_FAIL_MSG();                      \
      _QDBPTEST_EXIT_SUITE(exitonfailure);           \
    } else {                                         \
      _qdbptest_suite_results.passed++;              \
    }                                                \
  } while (0)

// TODO: Print more descriptive error messages
#define QDBP_TEST_INT_CMP_OP(op, a, b, exitonfailure, cast, fmt)        \
  do {                                                                  \
    int _qdbptest_a = (a);                                              \
    int _qdbptest_b = (b);                                              \
    if (!(_qdbptest_a op _qdbptest_b)) {                                \
      _QDBPTEST_LOG_FAIL();                                             \
      _qdbptest_log("  " #fmt " %s " #fmt "\n", (cast)_qdbptest_a, #op, \
                    (cast)_qdbptest_b);                                 \
      _QDBPTEST_EXIT_SUITE(exitonfailure);                              \
    } else {                                                            \
      _qdbptest_suite_results.passed++;                                 \
    }                                                                   \
  } while (0)

#define QDBP_TEST_STRING_EQ(a, b, exitonfailure)                       \
  do {                                                                 \
    const char* _qdbptest_a = (a);                                     \
    const char* _qdbptest_b = (b);                                     \
    if (strcmp(_qdbptest_a, _qdbptest_b) != 0) {                       \
      _QDBPTEST_LOG_FAIL();                                            \
      _qdbptest_log("  \"%s\" == \"%s\"\n", _qdbptest_a, _qdbptest_b); \
      _QDBPTEST_EXIT_SUITE(exitonfailure);                             \
    } else {                                                           \
      _qdbptest_suite_results.passed++;                                \
    }                                                                  \
  } while (0)

#define CHECK(cond) _QDBPTEST_TEST(cond, false)
#define CHECK_STRING_EQ(a, b) QDBP_TEST_STRING_EQ(a, b, false)
#define CHECK_INT_CMP(op, a, b) \
  QDBP_TEST_INT_CMP_OP(op, a, b, false, int64_t, % ld)
#define CHECK_UINT_CMP(op, a, b) \
  QDBP_TEST_INT_CMP_OP(op, a, b, false, uint64_t, % lu)

#endif