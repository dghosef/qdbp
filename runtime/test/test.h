#ifndef QDBP_TEST_H
#define QDBP_TEST_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "rnd.h"

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

#define TEST_SUITE(name)                                                    \
  struct _qdbptest_suite_results _qdbptest_suite_##name(void) {             \
    rnd_pcg_t _qdbptest_pcg;                                                \
    rnd_pcg_seed(&_qdbptest_pcg, 0u);                                       \
    const char* _qdbptest_suite_name = #name;                               \
    bool _qdbptest_continue = true;                                         \
    const char* _qdbptest_case_name = "";                                   \
    struct _qdbptest_suite_results _qdbptest_suite_results = {.passed = 0,  \
                                                              .failed = 0}; \
    bool _qdbptest_success = true;

#define TEST_SUITE_END()          \
  return _qdbptest_suite_results; \
  }

#define _QDBP_TEST_CASE_INTERNAL(name, memchk)                        \
  for (bool _qdbptest_bool = /*setup*/                                \
       (_qdbptest_success = true, _qdbptest_continue = true,          \
       _qdbptest_case_name = #name, true);                            \
       _qdbptest_bool; /*teardown*/ (memchk), _qdbptest_bool = false, \
            (_qdbptest_success ? _qdbptest_suite_results.passed++     \
                               : _qdbptest_suite_results.failed++),   \
            _qdbptest_case_name = "", _qdbptest_continue = true)

#define TEST_CASE(name) _QDBP_TEST_CASE_INTERNAL(name, (void)0)
#define TEST_CASE_NOLEAK(name) \
  _QDBP_TEST_CASE_INTERNAL(    \
      name, (_qdbptest_success = _qdbptest_success && _qdbp_check_mem()))

#define DECLARE_SUITE(name) \
  extern struct _qdbptest_suite_results _qdbptest_suite_##name(void);

#define RUN_SUITE(name)                                                \
  do {                                                                 \
    struct _qdbptest_suite_results _qdbptest_suite_results =           \
        _qdbptest_suite_##name();                                      \
    _qdbptest_log("Suite: \"%s\" - Passed: %zu, Failed: %zu\n", #name, \
                  _qdbptest_suite_results.passed,                      \
                  _qdbptest_suite_results.failed);                     \
  } while (0)

#define DECLARE_AND_RUN_SUITE(name) \
  DECLARE_SUITE(name)               \
  RUN_SUITE(name)

#define _QDBPTEST_LOG_FAIL_NO_NEWLINE()                                \
  _qdbptest_success = false;                                           \
  _qdbptest_log("Check \"%s/%s\" failed: %s:%d", _qdbptest_suite_name, \
                _qdbptest_case_name, __FILE__, __LINE__);

#define _QDBPTEST_LOG_FAIL()       \
  _QDBPTEST_LOG_FAIL_NO_NEWLINE(); \
  _qdbptest_log("\n");

#define _QDBPTEST_EXIT_SUITE(exit) \
  if (exit) {                      \
    _qdbptest_continue = false;    \
  }

#define _QDBPTEST_TEST(cond, exitonfailure) \
  do {                                      \
    if (_qdbptest_continue && !(cond)) {    \
      _QDBPTEST_LOG_FAIL();                 \
      _QDBPTEST_EXIT_SUITE(exitonfailure);  \
    }                                       \
  } while (0)

#define _QDBPTEST_TEST_MSG(cond, exitonfailure, msg) \
  do {                                               \
    if (_qdbptest_continue && !(cond)) {             \
      _QDBPTEST_LOG_FAIL_MSG();                      \
      _QDBPTEST_EXIT_SUITE(exitonfailure);           \
    }                                                \
  } while (0)

#define QDBP_TEST_INT_CMP_OP(op, a, b, exitonfailure, cast, fmt)          \
  do {                                                                    \
    if (_qdbptest_continue) {                                             \
      int _qdbptest_a = (a);                                              \
      int _qdbptest_b = (b);                                              \
      if (!(_qdbptest_a op _qdbptest_b)) {                                \
        _QDBPTEST_LOG_FAIL();                                             \
        _qdbptest_log("  " #fmt " %s " #fmt "\n", (cast)_qdbptest_a, #op, \
                      (cast)_qdbptest_b);                                 \
        _QDBPTEST_EXIT_SUITE(exitonfailure);                              \
      }                                                                   \
    }                                                                     \
  } while (0)

#define QDBP_TEST_STRING_EQ(a, b, exitonfailure)                         \
  do {                                                                   \
    if (_qdbptest_continue) {                                            \
      const char* _qdbptest_a = (a);                                     \
      const char* _qdbptest_b = (b);                                     \
      if (strcmp(_qdbptest_a, _qdbptest_b) != 0) {                       \
        _QDBPTEST_LOG_FAIL();                                            \
        _qdbptest_log("  \"%s\" == \"%s\"\n", _qdbptest_a, _qdbptest_b); \
        _QDBPTEST_EXIT_SUITE(exitonfailure);                             \
      }                                                                  \
    }                                                                    \
  } while (0)

#define QDBP_TEST_ABORT(stmt, exitonfailure)                           \
  do {                                                                 \
    int pid = fork();                                                  \
    if (pid == 0) {                                                    \
      stmt;                                                            \
      exit(0);                                                         \
    } else {                                                           \
      int status;                                                      \
      waitpid(pid, &status, 0);                                        \
      if (WEXITSTATUS(status) == 0) {                                  \
        _QDBPTEST_LOG_FAIL();                                          \
        _qdbptest_log(                                                 \
            "Expected statement to abort with nonzero exit status\n"); \
        _QDBPTEST_EXIT_SUITE(exitonfailure);                           \
      }                                                                \
    }                                                                  \
  } while (0)

#define randomU32() (rnd_pcg_next(&_qdbptest_pcg))
#define randomU64() (((uint64_t)randomU32() << 32) | randomU32())

#define CHECK(cond) _QDBPTEST_TEST(cond, false)
#define CHECK_STRING_EQ(a, b) QDBP_TEST_STRING_EQ(a, b, false)
#define CHECK_INT_CMP(op, a, b) \
  QDBP_TEST_INT_CMP_OP(op, a, b, false, int64_t, % ld)
#define CHECK_UINT_CMP(op, a, b) \
  QDBP_TEST_INT_CMP_OP(op, a, b, false, uint64_t, % lu)
#define CHECK_ABORT(stmt) QDBP_TEST_ABORT(stmt, false)

#define REQUIRE(cond) _QDBPTEST_TEST(cond, true)
#define REQUIRE_STRING_EQ(a, b) QDBP_TEST_STRING_EQ(a, b, true)
#define REQUIRE_INT_CMP(op, a, b) \
  QDBP_TEST_INT_CMP_OP(op, a, b, true, int64_t, % ld)
#define REQUIRE_UINT_CMP(op, a, b) \
  QDBP_TEST_INT_CMP_OP(op, a, b, true, uint64_t, % lu)
#define REQUIRE_ABORT(stmt) QDBP_TEST_ABORT(stmt, true)

#endif