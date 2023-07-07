#include <stdio.h>

#include "runtime.h"

static _qdbp_object_ptr drop_and_get_capture(_qdbp_object_arr captures,
                                             _qdbp_object_ptr o) {
  (void)captures;
  _qdbp_drop(o, 1);
  return captures[0];
}

static _qdbp_object_ptr print_intproto(_qdbp_object_arr captures,
                                       _qdbp_object_ptr o) {
  (void)captures;
  _qdbp_assert_kind(o, QDBP_PROTOTYPE);
  _qdbp_object_ptr i = _qdbp_invoke_1(o, VAL, o);
  _qdbp_assert_kind(i, QDBP_INT);
  printf("%lld", i->data.i);
  _qdbp_drop(i, 1);
  return _qdbp_empty_prototype();
}

#define MK_PROTO_ARITH_OP(name, op)                                           \
  static _qdbp_object_ptr name(_qdbp_object_arr captures, _qdbp_object_ptr a, \
                               _qdbp_object_ptr b) {                          \
    (void)captures;                                                           \
    _qdbp_dup(a, 1);                                                          \
    _qdbp_object_ptr a_val = _qdbp_invoke_1(a, VAL, a);                       \
    _qdbp_object_ptr b_val = _qdbp_invoke_1(b, VAL, b);                       \
    _qdbp_assert_kind(a_val, QDBP_INT);                                       \
    _qdbp_assert_kind(b_val, QDBP_INT);                                       \
    uint64_t result = op(a_val->data.i, b_val->data.i);                       \
    _qdbp_object_arr capture = (_qdbp_object_ptr[1]){0};                      \
    capture[0] =                                                              \
        _qdbp_make_object(QDBP_INT, (union _qdbp_object_data){.i = result});  \
    _qdbp_drop(a_val, 1);                                                     \
    _qdbp_drop(b_val, 1);                                                     \
    return _qdbp_replace(a, VAL, (void *)drop_and_get_capture, capture, 1);   \
  }

#define MK_PROTO_CMP_OP(name, op)                                             \
  static _qdbp_object_ptr name(_qdbp_object_arr captures, _qdbp_object_ptr a, \
                               _qdbp_object_ptr b) {                          \
    (void)captures;                                                           \
    _qdbp_object_ptr a_val = _qdbp_invoke_1(a, VAL, a);                       \
    _qdbp_object_ptr b_val = _qdbp_invoke_1(b, VAL, b);                       \
    _qdbp_assert_kind(a_val, QDBP_INT);                                       \
    _qdbp_assert_kind(b_val, QDBP_INT);                                       \
    bool result = _QDBP_COMPARE(op, a_val->data.i, b_val->data.i);            \
    _qdbp_drop(a_val, 1);                                                     \
    _qdbp_drop(b_val, 1);                                                     \
    return result ? _qdbp_true() : _qdbp_false();                             \
  }

MK_PROTO_ARITH_OP(_qdbp_add, _qdbp_checked_add)
MK_PROTO_ARITH_OP(_qdbp_sub, _qdbp_checked_sub)
MK_PROTO_ARITH_OP(_qdbp_mul, _qdbp_checked_mul)
MK_PROTO_ARITH_OP(_qdbp_div, _qdbp_checked_div)
MK_PROTO_ARITH_OP(_qdbp_mod, _qdbp_checked_mod)
MK_PROTO_CMP_OP(_qdbp_eq, ==)
MK_PROTO_CMP_OP(_qdbp_neq, !=)
MK_PROTO_CMP_OP(_qdbp_lt, <)
MK_PROTO_CMP_OP(_qdbp_gt, >)
MK_PROTO_CMP_OP(_qdbp_gte, >=)
MK_PROTO_CMP_OP(_qdbp_lte, <=)

_qdbp_object_ptr _qdbp_int_prototype(uint64_t value) {
  _qdbp_object_ptr proto = _qdbp_empty_prototype();
  _qdbp_object_ptr capture =
      _qdbp_make_object(QDBP_INT, (union _qdbp_object_data){.i = value});
  proto = _qdbp_extend(proto, VAL, (void *)drop_and_get_capture,
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
