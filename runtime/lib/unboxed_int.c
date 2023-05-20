#include "runtime.h"
bool is_unboxed_int(qdbp_object_ptr obj) { return ((uintptr_t)obj & 1) == 1; }

bool is_boxed_int(qdbp_object_ptr obj) {
  return !is_unboxed_int(obj) && get_kind(obj) == QDBP_BOXED_INT;
}
qdbp_object_ptr make_unboxed_int(int64_t value) {
  return (qdbp_object_ptr)((intptr_t)(value << 1) | 1);
}

int64_t get_unboxed_int(qdbp_object_ptr obj) {
  if (DYNAMIC_TYPECHECK) {
    assert(is_unboxed_int(obj));
  }
  return ((intptr_t)obj) >> 1;
}
int64_t get_boxed_int(qdbp_object_ptr obj) {
  if (DYNAMIC_TYPECHECK) {
    assert(is_boxed_int(obj));
  }
  return obj->data.boxed_int->value;
}

int64_t unbox_int(qdbp_object_ptr obj) {
  if (DYNAMIC_TYPECHECK) {
    assert(is_boxed_int(obj));
  }
  return obj->data.boxed_int->value;
}

qdbp_object_ptr unboxed_unary_op(qdbp_object_ptr obj, label_t op) {
  if (DYNAMIC_TYPECHECK) {
    assert(is_unboxed_int(obj));
    assert(op < NUM_OP_CNT);
  }
  int64_t i = get_unboxed_int(obj);
  switch (op) {
  case VAL:
    return qdbp_int(i);
    break;
  case PRINT:
    printf("%lld\n", i);
    return empty_prototype();
    break;
  default:
    printf("unboxed_unary_op: %llu\n", op);
    assert(false);
  }
}
#define MK_ARITH_SWITCH                                                        \
  MK_ARITH_CASE(ADD, +)                                                        \
  MK_ARITH_CASE(SUB, -)                                                        \
  MK_ARITH_CASE(MUL, *)                                                        \
  MK_ARITH_CASE(DIV, /)                                                        \
  MK_ARITH_CASE(MOD, %)
#define MK_CMP_SWITCH                                                          \
  MK_CMP_CASE(EQ, ==)                                                          \
  MK_CMP_CASE(NEQ, !=)                                                         \
  MK_CMP_CASE(LT, <)                                                           \
  MK_CMP_CASE(GT, >)                                                           \
  MK_CMP_CASE(LEQ, <=)                                                         \
  MK_CMP_CASE(GEQ, >=)
#define MK_SWITCH                                                              \
  MK_ARITH_SWITCH                                                              \
  MK_CMP_SWITCH

qdbp_object_ptr unboxed_binary_op(int64_t a, int64_t b, label_t op) {
  switch (op) {
#define MK_ARITH_CASE(n, op)                                                   \
  case n:                                                                      \
    return make_unboxed_int(a op b);
#define MK_CMP_CASE(n, op)                                                     \
  case n:                                                                      \
    return a op b ? qdbp_true() : qdbp_false();
    MK_SWITCH
#undef MK_ARITH_CASE
#undef MK_CMP_CASE
  default:
    assert(false);
  }
}

static qdbp_object_ptr boxed_binary_arith_op(qdbp_object_ptr a,
                                             qdbp_object_ptr b, label_t op) {
  if (DYNAMIC_TYPECHECK) {
    assert(is_boxed_int(a));
    assert(op < EQ);
  }
  int64_t a_int = get_boxed_int(a);
  int64_t b_int;
  if (is_unboxed_int(b)) {
    b_int = get_unboxed_int(b);
    drop(b, 1);
  } else if (is_boxed_int(b)) {
    b_int = b->data.boxed_int->value;
    drop(b, 1);
  } else {
    qdbp_object_ptr b_val = invoke_1(b, VAL, b);
    if (DYNAMIC_TYPECHECK) {
      assert(get_kind(b_val) == QDBP_INT);
    }
    b_int = b_val->data.i;
    drop(b_val, 1);
  }
  qdbp_object_ptr ret;
  if (is_unique(a) && REUSE_ANALYSIS) {
    ret = a;
  } else {
    ret = qdbp_malloc_obj();
    set_refcount(ret, 1);
    set_tag(ret, QDBP_BOXED_INT);
    ret->data.boxed_int = qdbp_malloc_boxed_int();
    ret->data.boxed_int->other_labels.labels = NULL;
    Word_t label = 0;
    qdbp_field_ptr *PValue;
    JLF(PValue, a->data.boxed_int->other_labels.labels, label);
    while (PValue != NULL) {
      qdbp_field_ptr new_field_ptr = qdbp_malloc_field();
      label_add(&(ret->data.boxed_int->other_labels), label, new_field_ptr);
      *new_field_ptr = **PValue;
      JLN(PValue, a->data.boxed_int->other_labels.labels, label);
    }
    drop(a, 1);
  }
#define MK_ARITH_CASE(n, op)                                                   \
  case n:                                                                      \
    ret->data.boxed_int->value = a_int op b_int;                               \
    break;

  switch (op) {
    MK_ARITH_SWITCH
  default:
    printf("boxed_binary_arith_op: %llu\n", op);
    assert(false);
  }
  // Ensure that all math is done in 63 bits
  ret->data.boxed_int->value <<= 1;
  ret->data.boxed_int->value >>= 1;
  return ret;
#undef MK_ARITH_CASE
}

static qdbp_object_ptr boxed_binary_cmp_op(qdbp_object_ptr a, qdbp_object_ptr b,
                                           label_t op) {
  if (DYNAMIC_TYPECHECK) {
    assert(is_boxed_int(a));
    assert(op >= EQ && op < NUM_OP_CNT);
  }
  int64_t a_int = get_boxed_int(a);
  int64_t b_int;
  if (is_unboxed_int(b)) {
    b_int = get_unboxed_int(b);
  drop(b, 1);
  } else if (is_boxed_int(b)) {
    b_int = b->data.boxed_int->value;
  drop(b, 1);
  } else {
    qdbp_object_ptr b_val = invoke_1(b, VAL, b);
    if (DYNAMIC_TYPECHECK) {
      assert(get_kind(b_val) == QDBP_INT);
    }
    b_int = b_val->data.i;
    drop(b_val, 1);
  }
  drop(a, 1);
#define MK_CMP_CASE(n, op)                                                     \
  case n:                                                                      \
    return a_int op b_int ? qdbp_true() : qdbp_false();
  switch (op) {
    MK_CMP_SWITCH
  default:
    assert(false);
  }
#undef MK_CMP_CASE
}
qdbp_object_ptr boxed_binary_op(qdbp_object_ptr a, qdbp_object_ptr b,
                                label_t op) {
  if (DYNAMIC_TYPECHECK) {
    assert(is_boxed_int(a));
    assert(op < NUM_OP_CNT);
  }
  if (op < EQ) {
    return boxed_binary_arith_op(a, b, op);
  } else {
    return boxed_binary_cmp_op(a, b, op);
  }
}

qdbp_object_ptr box(qdbp_object_ptr obj, label_t label, void *code,
                    qdbp_object_arr captures, size_t captures_size) {
  if (DYNAMIC_TYPECHECK) {
    assert(is_unboxed_int(obj));
  }
  int64_t value = get_unboxed_int(obj);
  qdbp_object_ptr ret = qdbp_malloc_obj();
  set_refcount(ret, 1);
  set_tag(ret, QDBP_BOXED_INT);
  ret->data.boxed_int = qdbp_malloc_boxed_int();
  ret->data.boxed_int->value = value;
  ret->data.boxed_int->other_labels.labels = NULL;
  qdbp_field_ptr field = qdbp_malloc_field();
  field->method.code = code;
  field->method.captures = make_captures(captures, captures_size);
  field->method.captures_size = captures_size;
  label_add(&(ret->data.boxed_int->other_labels), label, field);
  return ret;
}

qdbp_object_ptr box_extend(qdbp_object_ptr obj, label_t label, void *code,
                           qdbp_object_arr captures, size_t captures_size) {
  if (DYNAMIC_TYPECHECK) {
    assert(is_boxed_int(obj));
  }
  if (is_unique(obj) && REUSE_ANALYSIS) {
    qdbp_field_ptr field = qdbp_malloc_field();
    field->method.code = code;
    field->method.captures = make_captures(captures, captures_size);
    field->method.captures_size = captures_size;
    label_add(obj->data.boxed_int->other_labels.labels, label, field);
    return obj;
  } else {
    qdbp_object_ptr ret = qdbp_malloc_obj();
    set_refcount(ret, 1);
    set_tag(ret, QDBP_BOXED_INT);
    ret->data.boxed_int = qdbp_malloc_boxed_int();
    ret->data.boxed_int->value = obj->data.boxed_int->value;
    ret->data.boxed_int->other_labels.labels = NULL;
    Word_t idx = 0;
    qdbp_field_ptr *PValue;
    JLF(PValue, obj->data.boxed_int->other_labels.labels, idx);
    while (PValue != NULL) {
      qdbp_field_ptr new_field_ptr = qdbp_malloc_field();
      label_add(ret->data.boxed_int->other_labels.labels, idx, new_field_ptr);
      *new_field_ptr = **PValue;
      JLN(PValue, obj->data.boxed_int->other_labels.labels, idx);
    }
    qdbp_field_ptr field = qdbp_malloc_field();
    field->method.code = code;
    field->method.captures = make_captures(captures, captures_size);
    field->method.captures_size = captures_size;
    label_add(ret->data.boxed_int->other_labels.labels, label, field);
    drop(obj, 1);
    return ret;
  }
}

qdbp_object_ptr boxed_int_replace(qdbp_object_ptr obj, label_t label,
                                  void *code, qdbp_object_arr captures,
                                  size_t captures_size) {
  if (DYNAMIC_TYPECHECK) {
    assert(is_boxed_int(obj));
  }
  if (is_unique(obj) && REUSE_ANALYSIS) {
    qdbp_field_ptr field = label_get(&(obj->data.prototype), label);
    del_method(&(field->method));
    field->method = (struct qdbp_method){
        .code = code, .captures = captures, .captures_size = captures_size};
    return obj;
  } else {
    qdbp_object_ptr ret = qdbp_malloc_obj();
    set_refcount(ret, 1);
    set_tag(ret, QDBP_BOXED_INT);
    ret->data.boxed_int = qdbp_malloc_boxed_int();
    ret->data.boxed_int->value = obj->data.boxed_int->value;
    ret->data.boxed_int->other_labels.labels = NULL;
    Word_t idx = 0;
    qdbp_field_ptr *PValue;
    JLF(PValue, obj->data.boxed_int->other_labels.labels, idx);
    while (PValue != NULL) {
      qdbp_field_ptr new_field_ptr = qdbp_malloc_field();
      label_add(ret->data.boxed_int->other_labels.labels, idx, new_field_ptr);
      *new_field_ptr = **PValue;
      JLN(PValue, obj->data.boxed_int->other_labels.labels, idx);
    }
    qdbp_field_ptr field = qdbp_malloc_field();
    field->method.code = code;
    field->method.captures = captures;
    field->method.captures_size = captures_size;
    label_add(ret->data.boxed_int->other_labels.labels, label, field);
    drop(obj, 1);
    return ret;
  }
}

qdbp_object_ptr boxed_unary_op(qdbp_object_ptr arg0, label_t label) {
  if (DYNAMIC_TYPECHECK) {
    assert(is_boxed_int(arg0));
  }
  int64_t i = arg0->data.boxed_int->value;
  drop(arg0, 1);
  switch (label) {
  case PRINT:
    printf("%lld\n", i);
    return empty_prototype();
    break;
  case VAL:
    return qdbp_int(i);
    break;
  default:
    assert(false);
  }
}
