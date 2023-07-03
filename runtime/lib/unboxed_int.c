#include <stdio.h>

#include "runtime.h"
bool _qdbp_is_unboxed_int(_qdbp_object_ptr obj) {
  return ((uintptr_t)obj & 1) == 1;
}

bool _qdbp_is_boxed_int(_qdbp_object_ptr obj) {
  return !_qdbp_is_unboxed_int(obj) && _qdbp_get_kind(obj) == QDBP_BOXED_INT;
}
_qdbp_object_ptr _qdbp_make_unboxed_int(int64_t value) {
  return (_qdbp_object_ptr)((intptr_t)(value << 1) | 1);
}

int64_t _qdbp_get_unboxed_int(_qdbp_object_ptr obj) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(_qdbp_is_unboxed_int(obj));
  }
  return ((intptr_t)obj) >> 1;
}
int64_t _qdbp_get_boxed_int(_qdbp_object_ptr obj) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(_qdbp_is_boxed_int(obj));
  }
  return obj->data._qdbp_boxed_int->value;
}

static int64_t unbox_int(_qdbp_object_ptr obj) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(_qdbp_is_boxed_int(obj));
  }
  return obj->data._qdbp_boxed_int->value;
}

_qdbp_object_ptr _qdbp_unboxed_unary_op(_qdbp_object_ptr obj,
                                        _qdbp_label_t op) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(_qdbp_is_unboxed_int(obj));
    assert(op < MAX_OP);
  }
  int64_t i = _qdbp_get_unboxed_int(obj);
  switch (op) {
    case VAL:
      return _qdbp_int(i);
      break;
    case PRINT:
      printf("%lld\n", i);
      return _qdbp_empty_prototype();
      break;
    default:
      printf("_qdbp_unboxed_unary_op: %u\n", op);
      assert(false);
      __builtin_unreachable();
  }
}
#define MK_ARITH_SWITCH \
  MK_ARITH_CASE(ADD, +) \
  MK_ARITH_CASE(SUB, -) \
  MK_ARITH_CASE(MUL, *) \
  MK_ARITH_CASE(DIV, /) \
  MK_ARITH_CASE(MOD, %)
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

_qdbp_object_ptr _qdbp_unboxed_binary_op(int64_t a, int64_t b,
                                         _qdbp_label_t op) {
  switch (op) {
#define MK_ARITH_CASE(n, op) \
  case n:                    \
    return _qdbp_make_unboxed_int(a op b);
#define MK_CMP_CASE(n, op) \
  case n:                  \
    return a op b ? _qdbp_true() : _qdbp_false();
    MK_SWITCH
#undef MK_ARITH_CASE
#undef MK_CMP_CASE
    default:
      assert(false);
      __builtin_unreachable();
  }
}

static _qdbp_object_ptr boxed_binary_arith_op(_qdbp_object_ptr a,
                                              _qdbp_object_ptr b,
                                              _qdbp_label_t op) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(_qdbp_is_boxed_int(a));
    assert(op < EQ);
  }
  int64_t a_int = _qdbp_get_boxed_int(a);
  int64_t b_int;
  if (_qdbp_is_unboxed_int(b)) {
    b_int = _qdbp_get_unboxed_int(b);
    _qdbp_drop(b, 1);
  } else if (_qdbp_is_boxed_int(b)) {
    b_int = b->data._qdbp_boxed_int->value;
    _qdbp_drop(b, 1);
  } else {
    _qdbp_object_ptr b_val = _qdbp_invoke_1(b, VAL, b);
    if (_QDBP_DYNAMIC_TYPECHECK) {
      assert(_qdbp_get_kind(b_val) == QDBP_INT);
    }
    b_int = b_val->data.i;
    _qdbp_drop(b_val, 1);
  }
  _qdbp_object_ptr ret;
  if (_qdbp_is_unique(a) && _QDBP_REUSE_ANALYSIS) {
    ret = a;
  } else {
    ret = _qdbp_malloc_obj();
    _qdbp_set_refcount(ret, 1);
    _qdbp_set_tag(ret, QDBP_BOXED_INT);
    ret->data._qdbp_boxed_int = _qdbp_malloc_boxed_int();
    ret->data._qdbp_boxed_int->other_labels.labels = NULL;
    _qdbp_field_ptr *PValue;
    _qdbp_duplicate_labels(&a->data._qdbp_boxed_int->other_labels,
                           &ret->data._qdbp_boxed_int->other_labels);
    _qdbp_drop(a, 1);
  }
#define MK_ARITH_CASE(n, op)                           \
  case n:                                              \
    ret->data._qdbp_boxed_int->value = a_int op b_int; \
    break;

  switch (op) {
    MK_ARITH_SWITCH
    default:
      printf("boxed_binary_arith_op: %u\n", op);
      assert(false);
      __builtin_unreachable();
  }
  // Ensure that all math is done in 63 bits
  ret->data._qdbp_boxed_int->value <<= 1;
  ret->data._qdbp_boxed_int->value >>= 1;
  return ret;
#undef MK_ARITH_CASE
}

static _qdbp_object_ptr boxed_binary_cmp_op(_qdbp_object_ptr a,
                                            _qdbp_object_ptr b,
                                            _qdbp_label_t op) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(_qdbp_is_boxed_int(a));
    assert(op >= EQ && op < MAX_OP);
  }
  int64_t a_int = _qdbp_get_boxed_int(a);
  int64_t b_int;
  if (_qdbp_is_unboxed_int(b)) {
    b_int = _qdbp_get_unboxed_int(b);
    _qdbp_drop(b, 1);
  } else if (_qdbp_is_boxed_int(b)) {
    b_int = b->data._qdbp_boxed_int->value;
    _qdbp_drop(b, 1);
  } else {
    _qdbp_object_ptr b_val = _qdbp_invoke_1(b, VAL, b);
    if (_QDBP_DYNAMIC_TYPECHECK) {
      assert(_qdbp_get_kind(b_val) == QDBP_INT);
    }
    b_int = b_val->data.i;
    _qdbp_drop(b_val, 1);
  }
  _qdbp_drop(a, 1);
#define MK_CMP_CASE(n, op) \
  case n:                  \
    return a_int op b_int ? _qdbp_true() : _qdbp_false();
  switch (op) {
    MK_CMP_SWITCH
    default:
      assert(false);
      __builtin_unreachable();
  }
#undef MK_CMP_CASE
}
_qdbp_object_ptr _qdbp_boxed_binary_op(_qdbp_object_ptr a, _qdbp_object_ptr b,
                                       _qdbp_label_t op) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(_qdbp_is_boxed_int(a));
    assert(op < MAX_OP);
  }
  if (op < EQ) {
    return boxed_binary_arith_op(a, b, op);
  } else {
    return boxed_binary_cmp_op(a, b, op);
  }
}

_qdbp_object_ptr _qdbp_box(_qdbp_object_ptr obj, _qdbp_label_t label,
                           void *code, _qdbp_object_arr captures,
                           size_t captures_size) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(_qdbp_is_unboxed_int(obj));
  }
  int64_t value = _qdbp_get_unboxed_int(obj);
  _qdbp_object_ptr ret = _qdbp_malloc_obj();
  _qdbp_set_refcount(ret, 1);
  _qdbp_set_tag(ret, QDBP_BOXED_INT);
  ret->data._qdbp_boxed_int = _qdbp_malloc_boxed_int();
  ret->data._qdbp_boxed_int->value = value;
  ret->data._qdbp_boxed_int->other_labels.labels = NULL;
  struct _qdbp_field field = {0};
  field.method.code = code;
  field.method.captures = _qdbp_make_captures(captures, captures_size);
  field.method.captures_size = captures_size;
  field.label = label;
  _qdbp_label_add(&(ret->data._qdbp_boxed_int->other_labels), label, &field, 1);
  return ret;
}

_qdbp_object_ptr _qdbp_box_extend(_qdbp_object_ptr obj, _qdbp_label_t label,
                                  void *code, _qdbp_object_arr captures,
                                  size_t captures_size) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(_qdbp_is_boxed_int(obj));
  }
  if (_qdbp_is_unique(obj) && _QDBP_REUSE_ANALYSIS) {
    struct _qdbp_field field;
    field.method.code = code;
    field.method.captures = _qdbp_make_captures(captures, captures_size);
    field.method.captures_size = captures_size;
    field.label = label;
    _qdbp_label_add(&obj->data._qdbp_boxed_int->other_labels, label, &field, 1);
    return obj;
  } else {
    _qdbp_object_ptr ret = _qdbp_malloc_obj();
    _qdbp_set_refcount(ret, 1);
    _qdbp_set_tag(ret, QDBP_BOXED_INT);
    ret->data._qdbp_boxed_int = _qdbp_malloc_boxed_int();
    ret->data._qdbp_boxed_int->value = obj->data._qdbp_boxed_int->value;
    ret->data._qdbp_boxed_int->other_labels.labels = NULL;
    _qdbp_duplicate_labels(&obj->data._qdbp_boxed_int->other_labels,
                           &ret->data._qdbp_boxed_int->other_labels);
    struct _qdbp_field field;
    field.method.code = code;
    field.method.captures = _qdbp_make_captures(captures, captures_size);
    field.method.captures_size = captures_size;
    field.label = label;
    _qdbp_label_add(&ret->data._qdbp_boxed_int->other_labels, label, &field, 1);
    _qdbp_drop(obj, 1);
    return ret;
  }
}

_qdbp_object_ptr _qdbp_boxed_int_replace(_qdbp_object_ptr obj,
                                         _qdbp_label_t label, void *code,
                                         _qdbp_object_arr captures,
                                         size_t captures_size) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(_qdbp_is_boxed_int(obj));
  }
  if (_qdbp_is_unique(obj) && _QDBP_REUSE_ANALYSIS) {
    _qdbp_field_ptr field = _qdbp_label_get(&(obj->data.prototype), label);
    _qdbp_del_method(&(field->method));
    field->method = (struct _qdbp_method){
        .code = code, .captures = captures, .captures_size = captures_size};
    return obj;
  } else {
    _qdbp_object_ptr ret = _qdbp_malloc_obj();
    _qdbp_set_refcount(ret, 1);
    _qdbp_set_tag(ret, QDBP_BOXED_INT);
    ret->data._qdbp_boxed_int = _qdbp_malloc_boxed_int();
    ret->data._qdbp_boxed_int->value = obj->data._qdbp_boxed_int->value;
    ret->data._qdbp_boxed_int->other_labels.labels = NULL;
    _qdbp_duplicate_labels(&obj->data._qdbp_boxed_int->other_labels,
                           &ret->data._qdbp_boxed_int->other_labels);
    struct _qdbp_field field;
    field.method.code = code;
    field.method.captures = captures;
    field.method.captures_size = captures_size;
    field.label = label;
    _qdbp_label_add(&ret->data._qdbp_boxed_int->other_labels, label, &field, 1);
    _qdbp_drop(obj, 1);
    return ret;
  }
}

_qdbp_object_ptr _qdbp_boxed_unary_op(_qdbp_object_ptr arg0,
                                      _qdbp_label_t label) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(_qdbp_is_boxed_int(arg0));
  }
  int64_t i = arg0->data._qdbp_boxed_int->value;
  _qdbp_drop(arg0, 1);
  switch (label) {
    case PRINT:
      printf("%lld\n", i);
      return _qdbp_empty_prototype();
      break;
    case VAL:
      return _qdbp_int(i);
      break;
    default:
      assert(false);
      __builtin_unreachable();
  }
}
