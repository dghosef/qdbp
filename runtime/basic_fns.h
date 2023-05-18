#ifndef QDBP_BASIC_FNS
#define QDBP_BASIC_FNS
/*
Every function that qdbp calls must follow the following rules:
- Type of return value must be `qdbp_object_ptr`
- All arguments must have type `qdbp_object_ptr`
- For each argument `a`, either
  - The return value has 0 references to `a` and `a` is dropped
  - The return value has `n` references to `a` and `dup` is called on `a` `n -
1` times
*/

// FIXME: Safe arithmetic

#include "runtime.h"
#include <math.h>
#include <stdio.h>

static qdbp_object_ptr qdbp_abort() {
  printf("aborting\n");
  exit(0);
}

#define qdbp_int_binop(name, op)                                               \
  static qdbp_object_ptr name(qdbp_object_ptr a, qdbp_object_ptr b) {                 \
    assert_obj_kind(a, QDBP_INT);                                              \
    assert_obj_kind(b, QDBP_INT);                                              \
    if (is_unique(a)) {                                                        \
      a->data.i = a->data.i op b->data.i;                                      \
      drop(b, 1);                                                              \
      return a;                                                                \
    } else if (is_unique(b)) {                                                 \
      b->data.i = a->data.i op b->data.i;                                      \
      drop(a, 1);                                                              \
      return b;                                                                \
    } else {                                                                   \
      qdbp_object_ptr result = make_object(                                    \
          QDBP_INT, (union qdbp_object_data){.i = a->data.i op b->data.i});    \
      drop(a, 1);                                                              \
      drop(b, 1);                                                              \
      return result;                                                           \
    }                                                                          \
  }

#define qdbp_float_binop(name, op)                                             \
  static qdbp_object_ptr name(qdbp_object_ptr a, qdbp_object_ptr b) {                 \
    assert_obj_kind(a, QDBP_FLOAT);                                            \
    assert_obj_kind(b, QDBP_FLOAT);                                            \
    if (is_unique(a)) {                                                        \
      a->data.f = a->data.f op b->data.f;                                      \
      drop(b, 1);                                                              \
      return a;                                                                \
    } else if (is_unique(b)) {                                                 \
      b->data.f = a->data.f op b->data.f;                                      \
      drop(a, 1);                                                              \
      return b;                                                                \
    } else {                                                                   \
      qdbp_object_ptr result = make_object(                                    \
          QDBP_FLOAT, (union qdbp_object_data){.f = a->data.f op b->data.f});  \
      drop(a, 1);                                                              \
      drop(b, 1);                                                              \
      return result;                                                           \
    }                                                                          \
  }

#define qdbp_intcmp_binop(name, op)                                            \
  static qdbp_object_ptr name(qdbp_object_ptr a, qdbp_object_ptr b) {                 \
    assert_obj_kind(a, QDBP_INT);                                              \
    assert_obj_kind(b, QDBP_INT);                                              \
    qdbp_object_ptr result =                                                   \
        a->data.i op b->data.i ? qdbp_true() : qdbp_false();                   \
    drop(a, 1);                                                                \
    drop(b, 1);                                                                \
    return result;                                                             \
  }

#define qdbp_floatcmp_binop(name, op)                                          \
  static qdbp_object_ptr name(qdbp_object_ptr a, qdbp_object_ptr b) {                 \
    assert_obj_kind(a, QDBP_FLOAT);                                            \
    assert_obj_kind(b, QDBP_FLOAT);                                            \
    qdbp_object_ptr result =                                                   \
        a->data.f op b->data.f ? qdbp_true() : qdbp_false();                   \
    drop(a, 1);                                                                \
    drop(b, 1);                                                                \
    return result;                                                             \
  }
static char *empty_string() {
  char *s = (char *)qdbp_malloc(1);
  s[0] = '\0';
  return s;
}
static char *c_str_concat(const char *a, const char *b) {
  int lena = strlen(a);
  int lenb = strlen(b);
  char *con = (char *)qdbp_malloc(lena + lenb + 1);
  // copy & concat (including string termination)
  memcpy(con, a, lena);
  memcpy(con + lena, b, lenb + 1);
  return con;
}
// concat_string
static qdbp_object_ptr concat_string(qdbp_object_ptr a, qdbp_object_ptr b) {
  assert_obj_kind(a, QDBP_STRING);
  assert_obj_kind(b, QDBP_STRING);
  const char *a_str = a->data.s;
  const char *b_str = b->data.s;
  drop(a, 1);
  drop(b, 1);
  return make_object(QDBP_STRING,
                     (union qdbp_object_data){.s = c_str_concat(a_str, b_str)});
}
// qdbp_print_string_int
static qdbp_object_ptr qdbp_print_string_int(qdbp_object_ptr s) {
  assert_obj_kind(s, QDBP_STRING);
  printf("%s", s->data.s);
  fflush(stdout);
  drop(s, 1);
  return make_object(QDBP_INT, (union qdbp_object_data){.i = 0});
}
// qdbp_float_to_string
static qdbp_object_ptr qdbp_empty_string() {
  return make_object(QDBP_STRING,
                     (union qdbp_object_data){.s = empty_string()});
}
static qdbp_object_ptr qdbp_zero_int() {
  return make_object(QDBP_INT, (union qdbp_object_data){.i = 0});
}

// the autoformatter is weird
qdbp_int_binop(qdbp_int_add_int, +) qdbp_int_binop(qdbp_int_sub_int, -)
    qdbp_int_binop(qdbp_int_mul_int, *) qdbp_int_binop(qdbp_int_div_int, /)
        qdbp_int_binop(qdbp_int_mod_int, %)
            qdbp_intcmp_binop(qdbp_int_lt_bool, <)
                qdbp_intcmp_binop(qdbp_int_le_bool, <=)
                    qdbp_intcmp_binop(qdbp_int_gt_bool, >)
                        qdbp_intcmp_binop(qdbp_int_ge_bool, >=)
                            qdbp_intcmp_binop(qdbp_int_eq_bool, ==)
                                qdbp_intcmp_binop(
                                    qdbp_int_ne_bool,
                                    !=) void ____() /* fix formatting issues*/;
static size_t itoa_bufsize(int64_t i) {
  size_t result = 2; // '\0' at the end, first digit
  if (i < 0) {
    i *= -1;
    result++; // '-' character
  }
  while (i > 9) {
    result++;
    i /= 10;
  }
  return result;
}
static qdbp_object_ptr qdbp_int_to_string(qdbp_object_ptr i) {
  assert_obj_kind(i, QDBP_INT);
  int64_t val = i->data.i;
  drop(i, 1);
  int bufsize = itoa_bufsize(val);
  char *s = (char *)qdbp_malloc(bufsize);
  assert(snprintf(s, bufsize, "%lld", val) == bufsize - 1);
  return make_object(QDBP_STRING, (union qdbp_object_data){.s = s});
}

static qdbp_object_ptr qdbp_zero_float() {
  return make_object(QDBP_FLOAT, (union qdbp_object_data){.f = 0.0});
}
qdbp_float_binop(qdbp_float_add_float, +) qdbp_float_binop(qdbp_float_sub_float,
                                                           -)
    qdbp_float_binop(qdbp_float_mul_float, *)
        qdbp_float_binop(qdbp_float_div_float,
                         /) qdbp_floatcmp_binop(qdbp_float_lt_bool, <)
            qdbp_floatcmp_binop(qdbp_float_le_bool,
                                <=) qdbp_floatcmp_binop(qdbp_float_gt_bool, >)
                qdbp_floatcmp_binop(qdbp_float_ge_bool,
                                    >=) void ____() /* fix formatting issues*/;

static qdbp_object_ptr qdbp_float_mod_float(qdbp_object_ptr a, qdbp_object_ptr b) {
  assert_obj_kind(a, QDBP_FLOAT);
  assert_obj_kind(b, QDBP_FLOAT);
  qdbp_object_ptr result = make_object(
      QDBP_FLOAT, (union qdbp_object_data){.f = fmod(a->data.f, b->data.f)});
  drop(a, 1);
  drop(b, 1);
  return result;
}
#endif
