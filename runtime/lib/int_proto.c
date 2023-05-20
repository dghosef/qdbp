#include "runtime.h"
static qdbp_object_ptr get_capture(qdbp_object_arr captures,
                                   qdbp_object_ptr o) {
  drop(o, 1);
  return captures[0];
}

static qdbp_object_ptr print_intproto(qdbp_object_arr captures,
                                      qdbp_object_ptr o) {
  assert_obj_kind(o, QDBP_PROTOTYPE);
  o = invoke_1(o, VAL, o);
  assert_obj_kind(o, QDBP_INT);
  printf("%lld", o->data.i);
  drop(o, 1);
  return empty_prototype();
}

#define MK_PROTO_ARITH_OP(name, op)                                            \
  static qdbp_object_ptr name(qdbp_object_arr captures, qdbp_object_ptr a,     \
                              qdbp_object_ptr b) {                             \
    obj_dup(a, 1);                                                             \
    qdbp_object_ptr a_val = invoke_1(a, VAL, a);                               \
    qdbp_object_ptr b_val = invoke_1(b, VAL, b);                               \
    assert_obj_kind(a_val, QDBP_INT);                                          \
    assert_obj_kind(b_val, QDBP_INT);                                          \
    int64_t result = ((a_val->data.i op b_val->data.i) << 1) >> 1;             \
    qdbp_object_arr capture = (qdbp_object_ptr[1]){0};                         \
    capture[0] = make_object(QDBP_INT, (union qdbp_object_data){.i = result}); \
    drop(a_val, 1);                                                            \
    drop(b_val, 1);                                                            \
    return replace(a, VAL, (void *)get_capture, capture, 1);                   \
  }

#define MK_PROTO_CMP_OP(name, op)                                              \
  static qdbp_object_ptr name(qdbp_object_arr captures, qdbp_object_ptr a,     \
                              qdbp_object_ptr b) {                             \
    qdbp_object_ptr a_val = invoke_1(a, VAL, a);                               \
    qdbp_object_ptr b_val = invoke_1(b, VAL, b);                               \
    assert_obj_kind(a_val, QDBP_INT);                                          \
    assert_obj_kind(b_val, QDBP_INT);                                          \
    bool result = a_val->data.i op b_val->data.i;                              \
    drop(a_val, 1); drop(b_val, 1); return result ? qdbp_true() : qdbp_false();  \
  }

MK_PROTO_ARITH_OP(qdbp_add, +)
MK_PROTO_ARITH_OP(qdbp_sub, -)
MK_PROTO_ARITH_OP(qdbp_mul, *)
MK_PROTO_ARITH_OP(qdbp_div, /)
MK_PROTO_ARITH_OP(qdbp_mod, %)
MK_PROTO_CMP_OP(qdbp_eq, ==)
MK_PROTO_CMP_OP(qdbp_neq, !=)
MK_PROTO_CMP_OP(qdbp_lt, <)
MK_PROTO_CMP_OP(qdbp_gt, >)
MK_PROTO_CMP_OP(qdbp_gte, >=)
MK_PROTO_CMP_OP(qdbp_lte, <=)

qdbp_object_ptr make_int_proto(int64_t value) {
  qdbp_object_ptr proto = empty_prototype();
  qdbp_object_ptr capture =
      make_object(QDBP_INT, (union qdbp_object_data){.i = value});
  proto =
      extend(proto, VAL, (void *)get_capture, (qdbp_object_ptr[1]){capture}, 1);
  proto = extend(proto, ADD, (void *)qdbp_add, NULL, 0);
  proto = extend(proto, SUB, (void *)qdbp_sub, NULL, 0);
  proto = extend(proto, MUL, (void *)qdbp_mul, NULL, 0);
  proto = extend(proto, DIV, (void *)qdbp_div, NULL, 0);
  proto = extend(proto, MOD, (void *)qdbp_mod, NULL, 0);
  proto = extend(proto, EQ, (void *)qdbp_eq, NULL, 0);
  proto = extend(proto, NEQ, (void *)qdbp_neq, NULL, 0);
  proto = extend(proto, LT, (void *)qdbp_lt, NULL, 0);
  proto = extend(proto, GT, (void *)qdbp_gt, NULL, 0);
  proto = extend(proto, GEQ, (void *)qdbp_gte, NULL, 0);
  proto = extend(proto, LEQ, (void *)qdbp_lte, NULL, 0);
  proto = extend(proto, PRINT, (void *)print_intproto, NULL, 0);
  return proto;
}
