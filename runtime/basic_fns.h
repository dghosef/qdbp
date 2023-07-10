#ifndef QDBP_BASIC_FNS
#define QDBP_BASIC_FNS
#include <string.h>
/*
Every function that qdbp calls must follow the following rules:
- Type of return value must be `_qdbp_object_ptr`
- All arguments must have type `_qdbp_object_ptr`
- For each argument `a`, either
  - The return value has 0 references to `a` and `a` is dropped
  - The return value has `n` references to `a` and `dup` is called on `a` `n -
1` times
*/

#include <math.h>
#include <stdio.h>

#include "runtime.h"

static char *_qdbp_empty_charstar() {
  char *s = (char *)_qdbp_malloc(1);
  s[0] = '\0';
  return s;
}

static char *_qdbp_c_str_concat(const char *a, const char *b) {
  int lena = strlen(a);
  int lenb = strlen(b);
  char *con = (char *)_qdbp_malloc(lena + lenb + 1);
  // copy & concat (including string termination)
  _qdbp_memcpy(con, a, lena);
  _qdbp_memcpy(con + lena, b, lenb + 1);
  return con;
}

// concat_string
static _qdbp_object_ptr _qdbp_concat_string(_qdbp_object_ptr a,
                                            _qdbp_object_ptr b) {
  _qdbp_assert_kind(a, QDBP_STRING);
  _qdbp_assert_kind(b, QDBP_STRING);
  const char *a_str = a->data.s;
  const char *b_str = b->data.s;
  _qdbp_drop(a, 1);
  _qdbp_drop(b, 1);
  return _qdbp_make_object(
      QDBP_STRING,
      (union _qdbp_object_data){.s = _qdbp_c_str_concat(a_str, b_str)});
}

// qdbp_print_string_int
static _qdbp_object_ptr _qdbp_print_string_int(_qdbp_object_ptr s) {
  _qdbp_assert_kind(s, QDBP_STRING);
  printf("%s", s->data.s);
  fflush(stdout);
  _qdbp_drop(s, 1);
  return _qdbp_make_object(QDBP_INT, (union _qdbp_object_data){.i = _qdbp_make_bigint(0)});
}

// qdbp_float_to_string
static _qdbp_object_ptr _qdbp_empty_string() {
  return _qdbp_make_object(
      QDBP_STRING, (union _qdbp_object_data){.s = _qdbp_empty_charstar()});
}

static _qdbp_object_ptr _qdbp_zero_int() {
  return _qdbp_make_object(QDBP_INT, (union _qdbp_object_data){.i = _qdbp_make_bigint(0)});
}

static size_t _qdbp_itoa_bufsize(int64_t i) {
  size_t result = 2;  // '\0' at the end, first digit
  if (i < 0) {
    i *= -1;
    result++;  // '-' character
  }
  while (i > 9) {
    result++;
    i /= 10;
  }
  return result;
}

static _qdbp_object_ptr _qdbp_int_to_string(_qdbp_object_ptr i) {
  _qdbp_assert_kind(i, QDBP_INT);
  int64_t val = _qdbp_sign_extend(*i->data.i);
  _qdbp_drop(i, 1);
  int bufsize = _qdbp_itoa_bufsize(val);
  char *s = (char *)_qdbp_malloc(bufsize);
  assert(snprintf(s, bufsize, "%lld", val) == bufsize - 1);
  return _qdbp_make_object(QDBP_STRING, (union _qdbp_object_data){.s = s});
}

static _qdbp_object_ptr _qdbp_zero_float() {
  return _qdbp_make_object(QDBP_FLOAT, (union _qdbp_object_data){.f = 0.0});
}

#endif
