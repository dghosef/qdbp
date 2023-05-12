#ifndef QDBP_BASIC_FNS
#define QDBP_BASIC_FNS
// The drawback of using perceus reference counting is that all FFI called
// functions must be very careful with memory usage. In particular, the
// invariant that MUST be maintained is that all args must either be dropped
// or returned to the caller. They cannot be modified other than with the drop
// fn. To simplify, we always drop all args and return newly created
// objects(with refcount=1)

#include "runtime.h"
#include <math.h>
#include <stdio.h>

qdbp_object_ptr qdbp_abort() {
  printf("aborting\n");
  exit(0);
}
qdbp_object_ptr qdbp_true() {
  return make_object(QDBP_VARIANT,
                     (union qdbp_object_data){
                         .variant = {.tag = 1, .value = empty_prototype()}});
}
qdbp_object_ptr qdbp_false() {
  return make_object(QDBP_VARIANT,
                     (union qdbp_object_data){
                         .variant = {.tag = 0, .value = empty_prototype()}});
}

#define qdbp_int_binop(name, op)                                               \
  qdbp_object_ptr name(qdbp_object_ptr a, qdbp_object_ptr b) {                 \
    assert_obj_kind(a, QDBP_INT);                                              \
    assert_obj_kind(b, QDBP_INT);                                              \
    qdbp_object_ptr result = make_object(                                      \
        QDBP_INT, (union qdbp_object_data){.i = a->data.i op b->data.i});      \
    drop(a);                                                                   \
    drop(b);                                                                   \
    return result;                                                             \
  }

#define qdbp_float_binop(name, op)                                             \
  qdbp_object_ptr name(qdbp_object_ptr a, qdbp_object_ptr b) {                 \
    assert_obj_kind(a, QDBP_FLOAT);                                            \
    assert_obj_kind(b, QDBP_FLOAT);                                            \
    qdbp_object_ptr result = make_object(                                      \
        QDBP_FLOAT, (union qdbp_object_data){.f = a->data.f op b->data.f});    \
    drop(a);                                                                   \
    drop(b);                                                                   \
    return result;                                                             \
  }

#define qdbp_intcmp_binop(name, op)                                            \
  qdbp_object_ptr name(qdbp_object_ptr a, qdbp_object_ptr b) {                 \
    assert_obj_kind(a, QDBP_INT);                                              \
    assert_obj_kind(b, QDBP_INT);                                              \
    qdbp_object_ptr result =                                                   \
        a->data.i op b->data.i ? qdbp_true() : qdbp_false();                   \
    drop(a);                                                                   \
    drop(b);                                                                   \
    return result;                                                             \
  }

#define qdbp_floatcmp_binop(name, op)                                          \
  qdbp_object_ptr name(qdbp_object_ptr a, qdbp_object_ptr b) {                 \
    assert_obj_kind(a, QDBP_FLOAT);                                            \
    assert_obj_kind(b, QDBP_FLOAT);                                            \
    qdbp_object_ptr result =                                                   \
        a->data.f op b->data.f ? qdbp_true() : qdbp_false();                   \
    drop(a);                                                                   \
    drop(b);                                                                   \
    return result;                                                             \
  }
char *empty_string() {
  char *s = (char *)qdbp_malloc(1);
  s[0] = '\0';
  return s;
}
char *c_str_concat(const char *a, const char *b) {
  int lena = strlen(a);
  int lenb = strlen(b);
  char *con = (char *)qdbp_malloc(lena + lenb + 1);
  // copy & concat (including string termination)
  memcpy(con, a, lena);
  memcpy(con + lena, b, lenb + 1);
  return con;
}
// concat_string
qdbp_object_ptr concat_string(qdbp_object_ptr a, qdbp_object_ptr b) {
  assert_obj_kind(a, QDBP_STRING);
  assert_obj_kind(b, QDBP_STRING);
  const char *a_str = a->data.s;
  const char *b_str = b->data.s;
  drop(a);
  drop(b);
  return make_object(QDBP_STRING,
                     (union qdbp_object_data){.s = c_str_concat(a_str, b_str)});
}
// qdbp_print_string_int
qdbp_object_ptr qdbp_print_string_int(qdbp_object_ptr s) {
    assert_obj_kind(s, QDBP_STRING);
    printf("%s", s->data.s);
    fflush(stdout);
    drop(s);
    return make_object(QDBP_INT, (union qdbp_object_data){.i=0});
}
// qdbp_float_to_string
qdbp_object_ptr qdbp_empty_string() {
  return make_object(QDBP_STRING,
                     (union qdbp_object_data){.s = empty_string()});
}
qdbp_object_ptr qdbp_zero_int() {
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
size_t itoa_bufsize(int64_t i) {
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
qdbp_object_ptr qdbp_int_to_string(qdbp_object_ptr i) {
  assert_obj_kind(i, QDBP_INT);
  int64_t val = i->data.i;
  drop(i);
  int bufsize = itoa_bufsize(val);
  char *s = (char *)qdbp_malloc(bufsize);
  assert(snprintf(s, bufsize, "%lld", val) == bufsize - 1);
  return make_object(QDBP_STRING, (union qdbp_object_data){.s = s});
}

qdbp_object_ptr qdbp_zero_float() {
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

qdbp_object_ptr qdbp_float_mod_float(qdbp_object_ptr a, qdbp_object_ptr b) {
  assert_obj_kind(a, QDBP_FLOAT);
  assert_obj_kind(b, QDBP_FLOAT);
  qdbp_object_ptr result = make_object(
      QDBP_FLOAT, (union qdbp_object_data){.f = fmod(a->data.f, b->data.f)});
  drop(a);
  drop(b);
  return result;
}
#endif
