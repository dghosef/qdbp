#include <stdio.h>

#include "runtime.h"

bool _qdbp_is_unboxed_int(_qdbp_object_ptr obj) {
  return ((uintptr_t)obj & 1) == 1;
}

_qdbp_object_ptr _qdbp_make_unboxed_int(uint64_t value) {
  return (_qdbp_object_ptr)((intptr_t)(value << 1) | 1);
}

uint64_t _qdbp_get_unboxed_int(_qdbp_object_ptr obj) {
  _qdbp_assert(_qdbp_is_unboxed_int(obj));
  return ((intptr_t)obj) >> 1;
}

_qdbp_object_ptr _qdbp_unboxed_int_unary_op(_qdbp_object_ptr obj,
                                            _qdbp_label_t op) {
  _qdbp_assert(_qdbp_is_unboxed_int(obj));
  _qdbp_assert(op < MAX_OP);
  uint64_t i = _qdbp_get_unboxed_int(obj);
  switch (op) {
    case VAL:
      return _qdbp_int(i);
      break;
    case PRINT:
      printf("%lld\n", i);
      return _qdbp_empty_prototype();
      break;
    default:
      _qdbp_assert(false);
      __builtin_unreachable();
  }
}

#define MK_ARITH_SWITCH                 \
  MK_ARITH_CASE(ADD, _qdbp_checked_add) \
  MK_ARITH_CASE(SUB, _qdbp_checked_sub) \
  MK_ARITH_CASE(MUL, _qdbp_checked_mul) \
  MK_ARITH_CASE(DIV, _qdbp_checked_div) \
  MK_ARITH_CASE(MOD, _qdbp_checked_mod)

#define MK_CMP_SWITCH  \
  MK_CMP_CASE(EQ, ==)  \
  MK_CMP_CASE(NEQ, !=) \
  MK_CMP_CASE(LT, <)   \
  MK_CMP_CASE(GT, >)   \
  MK_CMP_CASE(LEQ, <=) \
  MK_CMP_CASE(GEQ, >=)

#define MK_SWITCH \
  MK_ARITH_SWITCH \
  MK_CMP_SWITCH

_qdbp_object_ptr _qdbp_unboxed_int_binary_op(uint64_t a, uint64_t b,
                                             _qdbp_label_t op) {
  switch (op) {
#define MK_ARITH_CASE(n, op) \
  case n:                    \
    return _qdbp_make_unboxed_int(op(a, b));

#define MK_CMP_CASE(n, op) \
  case n:                  \
    return _QDBP_COMPARE(op, a, b) ? _qdbp_true() : _qdbp_false();

    MK_SWITCH

#undef MK_ARITH_CASE
#undef MK_CMP_CASE
    default:
      _qdbp_assert(false);
      __builtin_unreachable();
  }
}

bool _qdbp_is_boxed_int(_qdbp_object_ptr obj) {
  return !_qdbp_is_unboxed_int(obj) && _qdbp_get_kind(obj) == QDBP_BOXED_INT;
}

uint64_t _qdbp_get_boxed_int(_qdbp_object_ptr obj) {
  _qdbp_assert(_qdbp_is_boxed_int(obj));
  return obj->data._qdbp_boxed_int->value;
}

_qdbp_object_ptr _qdbp_boxed_int_unary_op(_qdbp_object_ptr arg0,
                                          _qdbp_label_t op) {
  _qdbp_assert(_qdbp_is_boxed_int(arg0));
  uint64_t i = arg0->data._qdbp_boxed_int->value;
  _qdbp_drop(arg0, 1);
  switch (op) {
    case PRINT:
      printf("%lld\n", i);
      return _qdbp_empty_prototype();
      break;
    case VAL:
      return _qdbp_int(i);
      break;
    default:
      _qdbp_assert(false);
      __builtin_unreachable();
  }
}

static _qdbp_object_ptr boxed_binary_arith_op(_qdbp_object_ptr a,
                                              _qdbp_object_ptr b,
                                              _qdbp_label_t op) {
  _qdbp_assert(_qdbp_is_boxed_int(a));
  _qdbp_assert(op < EQ);
  uint64_t a_int = _qdbp_get_boxed_int(a);
  uint64_t b_int;
  if (_qdbp_is_unboxed_int(b)) {
    b_int = _qdbp_get_unboxed_int(b);
    _qdbp_drop(b, 1);
  } else if (_qdbp_is_boxed_int(b)) {
    b_int = b->data._qdbp_boxed_int->value;
    _qdbp_drop(b, 1);
  } else {
    _qdbp_object_ptr b_val = _qdbp_invoke_1(b, VAL, b);
    _qdbp_assert(_qdbp_get_kind(b_val) == QDBP_INT);
    b_int = b_val->data.i;
    _qdbp_drop(b_val, 1);
  }
  _qdbp_object_ptr ret;
  if (_qdbp_is_unique(a) && _QDBP_REUSE_ANALYSIS) {
    ret = a;
  } else {
    ret = _qdbp_make_boxed_int(0);
    _qdbp_copy_prototype(&a->data._qdbp_boxed_int->other_labels,
                         &ret->data._qdbp_boxed_int->other_labels);
    _qdbp_drop(a, 1);
  }
#define MK_ARITH_CASE(n, op)                             \
  case n:                                                \
    ret->data._qdbp_boxed_int->value = op(a_int, b_int); \
    break;

  switch (op) {
    MK_ARITH_SWITCH
    default:
      _qdbp_assert(false);
      __builtin_unreachable();
  }
  return ret;
#undef MK_ARITH_CASE
}

static _qdbp_object_ptr boxed_binary_cmp_op(_qdbp_object_ptr a,
                                            _qdbp_object_ptr b,
                                            _qdbp_label_t op) {
  _qdbp_assert(_qdbp_is_boxed_int(a));
  _qdbp_assert(op >= EQ && op < MAX_OP);
  uint64_t a_int = _qdbp_get_boxed_int(a);
  uint64_t b_int;
  if (_qdbp_is_unboxed_int(b)) {
    b_int = _qdbp_get_unboxed_int(b);
    _qdbp_drop(b, 1);
  } else if (_qdbp_is_boxed_int(b)) {
    b_int = b->data._qdbp_boxed_int->value;
    _qdbp_drop(b, 1);
  } else {
    _qdbp_object_ptr b_val = _qdbp_invoke_1(b, VAL, b);
    _qdbp_assert(_qdbp_get_kind(b_val) == QDBP_INT);
    b_int = b_val->data.i;
    _qdbp_drop(b_val, 1);
  }
  _qdbp_drop(a, 1);
#define MK_CMP_CASE(n, op) \
  case n:                  \
    return _QDBP_COMPARE(op, a_int, b_int) ? _qdbp_true() : _qdbp_false();
  switch (op) {
    MK_CMP_SWITCH
    default:
      _qdbp_assert(false);
      __builtin_unreachable();
  }
#undef MK_CMP_CASE
}

_qdbp_object_ptr _qdbp_boxed_int_binary_op(_qdbp_object_ptr a,
                                           _qdbp_object_ptr b,
                                           _qdbp_label_t op) {
  _qdbp_assert(_qdbp_is_boxed_int(a));
  _qdbp_assert(op < MAX_OP);
  if (op < EQ) {
    return boxed_binary_arith_op(a, b, op);
  } else {
    return boxed_binary_cmp_op(a, b, op);
  }
}

_qdbp_object_ptr _qdbp_box_unboxed_int_and_extend(_qdbp_object_ptr obj,
                                       _qdbp_label_t label, void *code,
                                       _qdbp_object_arr captures,
                                       size_t num_captures) {
  _qdbp_assert(_qdbp_is_unboxed_int(obj));
  uint64_t value = _qdbp_get_unboxed_int(obj);
  _qdbp_object_ptr ret = _qdbp_make_boxed_int(value);
  struct _qdbp_field field;
  _qdbp_init_field(&field, label, _qdbp_duplicate_captures(captures, num_captures),
                   code, num_captures);
  _qdbp_label_add(&(ret->data._qdbp_boxed_int->other_labels), label, &field, 1);
  return ret;
}

_qdbp_object_ptr _qdbp_boxed_int_extend(_qdbp_object_ptr obj,
                                        _qdbp_label_t label, void *code,
                                        _qdbp_object_arr captures,
                                        size_t num_captures) {
  _qdbp_assert(_qdbp_is_boxed_int(obj));
  struct _qdbp_field field;
  _qdbp_init_field(&field, label,
                   _qdbp_duplicate_captures(captures, num_captures), code,
                   num_captures);
  if (_qdbp_is_unique(obj) && _QDBP_REUSE_ANALYSIS) {
    _qdbp_label_add(&obj->data._qdbp_boxed_int->other_labels, label, &field, 1);
    return obj;
  } else {
    _qdbp_object_ptr ret =
        _qdbp_make_boxed_int(obj->data._qdbp_boxed_int->value);
    _qdbp_copy_prototype(&obj->data._qdbp_boxed_int->other_labels,
                         &ret->data._qdbp_boxed_int->other_labels);
    _qdbp_label_add(&ret->data._qdbp_boxed_int->other_labels, label, &field, 1);
    _qdbp_drop(obj, 1);
    return ret;
  }
}

_qdbp_object_ptr _qdbp_boxed_int_replace(_qdbp_object_ptr obj,
                                         _qdbp_label_t label, void *code,
                                         _qdbp_object_arr captures,
                                         size_t num_captures) {
  _qdbp_assert(_qdbp_is_boxed_int(obj));
  if (_qdbp_is_unique(obj) && _QDBP_REUSE_ANALYSIS) {
    _qdbp_field_ptr field = _qdbp_label_get(&(obj->data.prototype), label);
    _qdbp_del_method(&(field->method));
    field->method = (struct _qdbp_method){
        .code = code, .captures = captures, .num_captures = num_captures};
    return obj;
  } else {
    _qdbp_object_ptr ret =
        _qdbp_make_boxed_int(obj->data._qdbp_boxed_int->value);
    _qdbp_copy_prototype(&obj->data._qdbp_boxed_int->other_labels,
                         &ret->data._qdbp_boxed_int->other_labels);
    struct _qdbp_field field;
    _qdbp_init_field(&field, label, captures, code, num_captures);
    _qdbp_label_add(&ret->data._qdbp_boxed_int->other_labels, label, &field, 1);
    _qdbp_drop(obj, 1);
    return ret;
  }
}
