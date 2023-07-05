#include <stdio.h>

#include "runtime.h"

static _qdbp_object_ptr get_capture(_qdbp_object_arr captures,
                                    _qdbp_object_ptr o) {
  _qdbp_drop(o, 1);
  return captures[0];
}

static _qdbp_object_ptr print_intproto(_qdbp_object_arr captures,
                                       _qdbp_object_ptr o) {
  _qdbp_assert_obj_kind(o, QDBP_PROTOTYPE);
  o = _qdbp_invoke_1(o, VAL, o);
  _qdbp_assert_obj_kind(o, QDBP_INT);
  printf("%lld", o->data.i);
  _qdbp_drop(o, 1);
  return _qdbp_empty_prototype();
}

#define MK_PROTO_ARITH_OP(name, op)                                           \
  static _qdbp_object_ptr name(_qdbp_object_arr captures, _qdbp_object_ptr a, \
                               _qdbp_object_ptr b) {                          \
    _qdbp_obj_dup(a, 1);                                                      \
    _qdbp_object_ptr a_val = _qdbp_invoke_1(a, VAL, a);                       \
    _qdbp_object_ptr b_val = _qdbp_invoke_1(b, VAL, b);                       \
    _qdbp_assert_obj_kind(a_val, QDBP_INT);                                   \
    _qdbp_assert_obj_kind(b_val, QDBP_INT);                                   \
    int64_t result = ((a_val->data.i op b_val->data.i) << 1) >> 1;            \
    _qdbp_object_arr capture = (_qdbp_object_ptr[1]){0};                      \
    capture[0] =                                                              \
        _qdbp_make_object(QDBP_INT, (union _qdbp_object_data){.i = result});  \
    _qdbp_drop(a_val, 1);                                                     \
    _qdbp_drop(b_val, 1);                                                     \
    return _qdbp_replace(a, VAL, (void *)get_capture, capture, 1);            \
  }

#define MK_PROTO_CMP_OP(name, op)                                             \
  static _qdbp_object_ptr name(_qdbp_object_arr captures, _qdbp_object_ptr a, \
                               _qdbp_object_ptr b) {                          \
    _qdbp_object_ptr a_val = _qdbp_invoke_1(a, VAL, a);                       \
    _qdbp_object_ptr b_val = _qdbp_invoke_1(b, VAL, b);                       \
    _qdbp_assert_obj_kind(a_val, QDBP_INT);                                   \
    _qdbp_assert_obj_kind(b_val, QDBP_INT);                                   \
    bool result = a_val->data.i op b_val->data.i;                             \
    _qdbp_drop(a_val, 1);                                                     \
    _qdbp_drop(b_val, 1);                                                     \
    return result ? _qdbp_true() : _qdbp_false();                             \
  }

MK_PROTO_ARITH_OP(_qdbp_add, +)
MK_PROTO_ARITH_OP(_qdbp_sub, -)
MK_PROTO_ARITH_OP(_qdbp_mul, *)
MK_PROTO_ARITH_OP(_qdbp_div, /)
MK_PROTO_ARITH_OP(_qdbp_mod, %)
MK_PROTO_CMP_OP(_qdbp_eq, ==)
MK_PROTO_CMP_OP(_qdbp_neq, !=)
MK_PROTO_CMP_OP(_qdbp_lt, <)
MK_PROTO_CMP_OP(_qdbp_gt, >)
MK_PROTO_CMP_OP(_qdbp_gte, >=)
MK_PROTO_CMP_OP(_qdbp_lte, <=)

_qdbp_object_ptr _qdbp_make_int_proto(int64_t value) {
  _qdbp_object_ptr proto = _qdbp_empty_prototype();
  _qdbp_object_ptr capture =
      _qdbp_make_object(QDBP_INT, (union _qdbp_object_data){.i = value});
  proto = _qdbp_extend(proto, VAL, (void *)get_capture,
                       (_qdbp_object_ptr[1]){capture}, 1, 16);
  proto = _qdbp_extend(proto, ADD, (void *)_qdbp_add, NULL, 0, 16);
  proto = _qdbp_extend(proto, SUB, (void *)_qdbp_sub, NULL, 0, 16);
  proto = _qdbp_extend(proto, MUL, (void *)_qdbp_mul, NULL, 0, 16);
  proto = _qdbp_extend(proto, DIV, (void *)_qdbp_div, NULL, 0, 16);
  proto = _qdbp_extend(proto, MOD, (void *)_qdbp_mod, NULL, 0, 16);
  proto = _qdbp_extend(proto, EQ, (void *)_qdbp_eq, NULL, 0, 16);
  proto = _qdbp_extend(proto, NEQ, (void *)_qdbp_neq, NULL, 0, 16);
  proto = _qdbp_extend(proto, LT, (void *)_qdbp_lt, NULL, 0, 16);
  proto = _qdbp_extend(proto, GT, (void *)_qdbp_gt, NULL, 0, 16);
  proto = _qdbp_extend(proto, GEQ, (void *)_qdbp_gte, NULL, 0, 16);
  proto = _qdbp_extend(proto, LEQ, (void *)_qdbp_lte, NULL, 0, 16);
  proto = _qdbp_extend(proto, PRINT, (void *)print_intproto, NULL, 0, 16);
  return proto;
}
