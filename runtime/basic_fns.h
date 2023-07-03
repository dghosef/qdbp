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

// FIXME: Safe arithmetic

#include <math.h>
#include <stdio.h>

#include "runtime.h"

static _qdbp_object_ptr _qdbp_abort() {
  printf("aborting\n");
  exit(0);
}

#define _qdbp_int_binop(name, op)                                            \
  static _qdbp_object_ptr name(_qdbp_object_ptr a, _qdbp_object_ptr b) {     \
    _qdbp_assert_obj_kind(a, QDBP_INT);                                            \
    _qdbp_assert_obj_kind(b, QDBP_INT);                                            \
    if (_qdbp_is_unique(a)) {                                                \
      a->data.i = a->data.i op b->data.i;                                    \
      _qdbp_drop(b, 1);                                                      \
      return a;                                                              \
    } else if (_qdbp_is_unique(b)) {                                         \
      b->data.i = a->data.i op b->data.i;                                    \
      _qdbp_drop(a, 1);                                                      \
      return b;                                                              \
    } else {                                                                 \
      _qdbp_object_ptr result = _qdbp_make_object(                           \
          QDBP_INT, (union _qdbp_object_data){.i = a->data.i op b->data.i}); \
      _qdbp_drop(a, 1);                                                      \
      _qdbp_drop(b, 1);                                                      \
      return result;                                                         \
    }                                                                        \
  }

#define _qdbp_float_binop(name, op)                                            \
  static _qdbp_object_ptr name(_qdbp_object_ptr a, _qdbp_object_ptr b) {       \
    _qdbp_assert_obj_kind(a, QDBP_FLOAT);                                            \
    _qdbp_assert_obj_kind(b, QDBP_FLOAT);                                            \
    if (_qdbp_is_unique(a)) {                                                  \
      a->data.f = a->data.f op b->data.f;                                      \
      _qdbp_drop(b, 1);                                                        \
      return a;                                                                \
    } else if (_qdbp_is_unique(b)) {                                           \
      b->data.f = a->data.f op b->data.f;                                      \
      _qdbp_drop(a, 1);                                                        \
      return b;                                                                \
    } else {                                                                   \
      _qdbp_object_ptr result = _qdbp_make_object(                             \
          QDBP_FLOAT, (union _qdbp_object_data){.f = a->data.f op b->data.f}); \
      _qdbp_drop(a, 1);                                                        \
      _qdbp_drop(b, 1);                                                        \
      return result;                                                           \
    }                                                                          \
  }

#define _qdbp_intcmp_binop(name, op)                                     \
  static _qdbp_object_ptr name(_qdbp_object_ptr a, _qdbp_object_ptr b) { \
    _qdbp_assert_obj_kind(a, QDBP_INT);                                        \
    _qdbp_assert_obj_kind(b, QDBP_INT);                                        \
    _qdbp_object_ptr result =                                            \
        a->data.i op b->data.i ? _qdbp_true() : _qdbp_false();           \
    _qdbp_drop(a, 1);                                                    \
    _qdbp_drop(b, 1);                                                    \
    return result;                                                       \
  }

#define _qdbp_floatcmp_binop(name, op)                                   \
  static _qdbp_object_ptr name(_qdbp_object_ptr a, _qdbp_object_ptr b) { \
    _qdbp_assert_obj_kind(a, QDBP_FLOAT);                                      \
    _qdbp_assert_obj_kind(b, QDBP_FLOAT);                                      \
    _qdbp_object_ptr result =                                            \
        a->data.f op b->data.f ? _qdbp_true() : _qdbp_false();           \
    _qdbp_drop(a, 1);                                                    \
    _qdbp_drop(b, 1);                                                    \
    return result;                                                       \
  }
static char *_qdbp_empty_charstar() {
  char *s = (char *)_qdbp_malloc(1, "empty_string");
  s[0] = '\0';
  return s;
}
static char *_qdbp_c_str_concat(const char *a, const char *b) {
  int lena = strlen(a);
  int lenb = strlen(b);
  char *con = (char *)_qdbp_malloc(lena + lenb + 1, "c_str_concat");
  // copy & concat (including string termination)
  memcpy(con, a, lena);
  memcpy(con + lena, b, lenb + 1);
  return con;
}
// concat_string
static _qdbp_object_ptr _qdbp_concat_string(_qdbp_object_ptr a,
                                            _qdbp_object_ptr b) {
  _qdbp_assert_obj_kind(a, QDBP_STRING);
  _qdbp_assert_obj_kind(b, QDBP_STRING);
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
  _qdbp_assert_obj_kind(s, QDBP_STRING);
  printf("%s", s->data.s);
  fflush(stdout);
  _qdbp_drop(s, 1);
  return _qdbp_make_object(QDBP_INT, (union _qdbp_object_data){.i = 0});
}
// qdbp_float_to_string
static _qdbp_object_ptr _qdbp_empty_string() {
  return _qdbp_make_object(
      QDBP_STRING, (union _qdbp_object_data){.s = _qdbp_empty_charstar()});
}
static _qdbp_object_ptr _qdbp_zero_int() {
  return _qdbp_make_object(QDBP_INT, (union _qdbp_object_data){.i = 0});
}

// the autoformatter is weird
_qdbp_int_binop(_qdbp_int_add_int, +);
_qdbp_int_binop(_qdbp_int_sub_int, -);
_qdbp_int_binop(_qdbp_int_mul_int, *);
_qdbp_int_binop(_qdbp_int_div_int, /);
_qdbp_int_binop(_qdbp_int_mod_int, %);
_qdbp_intcmp_binop(_qdbp_int_lt_bool, <);
_qdbp_intcmp_binop(_qdbp_int_le_bool, <=);
_qdbp_intcmp_binop(_qdbp_int_gt_bool, >);
_qdbp_intcmp_binop(_qdbp_int_ge_bool, >=);
_qdbp_intcmp_binop(_qdbp_int_eq_bool, ==);
_qdbp_intcmp_binop(_qdbp_int_ne_bool, !=);

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
  _qdbp_assert_obj_kind(i, QDBP_INT);
  int64_t val = i->data.i;
  _qdbp_drop(i, 1);
  int bufsize = _qdbp_itoa_bufsize(val);
  char *s = (char *)_qdbp_malloc(bufsize, "qdbp_int_to_string");
  assert(snprintf(s, bufsize, "%lld", val) == bufsize - 1);
  return _qdbp_make_object(QDBP_STRING, (union _qdbp_object_data){.s = s});
}

static _qdbp_object_ptr _qdbp_zero_float() {
  return _qdbp_make_object(QDBP_FLOAT, (union _qdbp_object_data){.f = 0.0});
}
_qdbp_float_binop(_qdbp_float_add_float, +);
_qdbp_float_binop(_qdbp_float_sub_float, -);
_qdbp_float_binop(_qdbp_float_mul_float, *);
_qdbp_float_binop(_qdbp_float_div_float, /);
_qdbp_floatcmp_binop(_qdbp_float_lt_bool, <);
_qdbp_floatcmp_binop(_qdbp_float_le_bool, <=);
_qdbp_floatcmp_binop(_qdbp_float_gt_bool, >);
_qdbp_floatcmp_binop(_qdbp_float_ge_bool,
                     >=) void ____() /* fix formatting issues*/;

static _qdbp_object_ptr _qdbp_float_mod_float(_qdbp_object_ptr a,
                                              _qdbp_object_ptr b) {
  _qdbp_assert_obj_kind(a, QDBP_FLOAT);
  _qdbp_assert_obj_kind(b, QDBP_FLOAT);
  _qdbp_object_ptr result = _qdbp_make_object(
      QDBP_FLOAT, (union _qdbp_object_data){.f = fmod(a->data.f, b->data.f)});
  _qdbp_drop(a, 1);
  _qdbp_drop(b, 1);
  return result;
}
#endif
