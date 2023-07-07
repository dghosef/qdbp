#include <stdio.h>

#include "runtime.h"

size_t _qdbp_prototype_size(_qdbp_prototype_ptr proto) {
  _qdbp_assert(proto);
  return _qdbp_ht_size(proto->labels);
}

void _qdbp_label_add(_qdbp_prototype_ptr proto, _qdbp_label_t label,
                     _qdbp_field_ptr field, size_t default_capacity) {
  _qdbp_assert(proto);
  _qdbp_assert(field);
  // check that default_capacity is a pwr of 2
  _qdbp_assert((default_capacity & (default_capacity - 1)) == 0);
  if (!proto->labels) {
    proto->labels = _qdbp_ht_new(default_capacity);
  }
  proto->labels = _qdbp_ht_insert(proto->labels, field);
}

_qdbp_field_ptr _qdbp_label_get(_qdbp_prototype_ptr proto,
                                _qdbp_label_t label) {
  _qdbp_assert(proto);
  _qdbp_assert(proto->labels);
  return _qdbp_ht_find(proto->labels, label);
}

void _qdbp_copy_prototype(_qdbp_prototype_ptr src, _qdbp_prototype_ptr dest) {
  _qdbp_assert(!dest->labels);
  dest->labels = _qdbp_ht_duplicate(src->labels);
}

void _qdbp_make_fresh_captures_except(_qdbp_prototype_ptr new_prototype,
                                      _qdbp_label_t except) {
  // Copy all the capture arrays except the new one
  _qdbp_field_ptr field;
  size_t tmp;
  _QDBP_HT_ITER(new_prototype->labels, field, tmp) {
    if (field->label != except) {
      _qdbp_object_arr original = field->method.captures;
      field->method.captures =
          _qdbp_malloc_capture_arr(field->method.num_captures);
      _qdbp_memcpy(field->method.captures, original,
                   sizeof(_qdbp_object_ptr) * field->method.num_captures);
    }
  }
}

_qdbp_object_arr _qdbp_duplicate_captures(_qdbp_object_arr captures,
                                          size_t size) {
  if (size == 0) {
    return NULL;
  } else {
    _qdbp_object_arr out = _qdbp_malloc_capture_arr(size);
    _qdbp_memcpy(out, captures, sizeof(struct _qdbp_object *) * size);
    return out;
  }
}

_qdbp_object_arr _qdbp_get_method(_qdbp_object_ptr obj, _qdbp_label_t label,
                                  void **code /*output param*/) {
  _qdbp_assert_kind(obj, QDBP_PROTOTYPE);
  struct _qdbp_method m =
      _qdbp_label_get(&(obj->data.prototype), label)->method;
  _qdbp_dup_method_captures(&m);
  *code = m.code;
  _qdbp_object_arr ret = m.captures;
  return ret;
}

static struct _qdbp_prototype prototype_copy_and_extend(
    const _qdbp_prototype_ptr src, const _qdbp_field_ptr new_field,
    size_t new_label, size_t default_capacity) {
  // copy src
  size_t src_size = _qdbp_prototype_size(src);
  struct _qdbp_prototype new_prototype = {.labels = NULL};

  _qdbp_copy_prototype(src, &new_prototype);
  _qdbp_label_add(&new_prototype, new_label, new_field, default_capacity);
  _qdbp_make_fresh_captures_except(&new_prototype, new_label);
  return new_prototype;
}

_qdbp_object_ptr _qdbp_extend(_qdbp_object_ptr obj, _qdbp_label_t label,
                              void *code, _qdbp_object_arr captures,
                              size_t num_captures, size_t default_capacity) {
  if (!obj) {
    // Special case: `obj` is the empty prototype
    obj = _qdbp_make_object(
        QDBP_PROTOTYPE,
        (union _qdbp_object_data){
            .prototype = {.labels = _qdbp_ht_new(default_capacity)}});
    // Fallthrough to after special case handling
  } else if (_qdbp_is_unboxed_int(obj)) {
    // Special case: `obj` is an unboxed int
    return _qdbp_box_unboxed_int_and_extend(obj, label, code, captures,
                                            num_captures);
  } else if (_qdbp_is_boxed_int(obj)) {
    // Special case: `obj` is a boxed int
    return _qdbp_boxed_int_extend(obj, label, code, captures, num_captures);
  }
  struct _qdbp_field new_field = {
      .label = label,
      .method = {.captures = _qdbp_duplicate_captures(captures, num_captures),
                 .num_captures = num_captures,
                 .code = code}};
  _qdbp_assert_kind(obj, QDBP_PROTOTYPE);
  if (!_QDBP_REUSE_ANALYSIS || !_qdbp_is_unique(obj)) {
    _qdbp_prototype_ptr prototype = &(obj->data.prototype);
    struct _qdbp_prototype new_prototype =
        prototype_copy_and_extend(prototype, &new_field, label, default_capacity);
    _qdbp_object_ptr new_obj = _qdbp_make_object(
        QDBP_PROTOTYPE, (union _qdbp_object_data){.prototype = new_prototype});
    _qdbp_dup_prototype_captures(prototype);
    _qdbp_drop(obj, 1);
    return new_obj;
  } else {
    _qdbp_label_add(&(obj->data.prototype), label, &new_field, default_capacity);
    return obj;
  }
}

static struct _qdbp_prototype prototype_copy_and_replace(
    const _qdbp_prototype_ptr src, const _qdbp_field_ptr new_field,
    _qdbp_label_t new_label) {
  size_t src_size = _qdbp_prototype_size(src);
  _qdbp_assert(src_size);
  _qdbp_assert(new_field->label == new_label);
  struct _qdbp_prototype new_prototype = {.labels = NULL};

  _qdbp_assert(new_prototype.labels == NULL);
  _qdbp_copy_prototype(src, &new_prototype);

  *(_qdbp_ht_find(new_prototype.labels, new_label)) = *new_field;
  _qdbp_make_fresh_captures_except(&new_prototype, new_label);
  _qdbp_assert(new_prototype.labels);
  return new_prototype;
}

_qdbp_object_ptr _qdbp_replace(_qdbp_object_ptr obj, _qdbp_label_t label,
                               void *code, _qdbp_object_arr captures,
                               size_t num_captures) {
  if (!obj) {
    // Special case: `obj` is the empty prototype
    obj = _qdbp_make_object(QDBP_PROTOTYPE, (union _qdbp_object_data){
                                                .prototype = {.labels = NULL}});
    // Fallthrough to after special case handling
  } else if (_qdbp_is_unboxed_int(obj)) {
    // Special case: `obj` is an unboxed int
    obj = _qdbp_int_prototype(_qdbp_get_unboxed_int(obj));
    _qdbp_assert_kind(obj, QDBP_PROTOTYPE);
    // Fallthrough to after special case handling
  } else if (_qdbp_is_boxed_int(obj) && label < MAX_OP) {
    // Special case: `obj` is a boxed int and we're replacing an builtin math
    // method
    uint64_t i = _qdbp_get_boxed_int(obj);
    _qdbp_drop(obj, 1);
    obj = _qdbp_int_prototype(i);
    _qdbp_assert_kind(obj, QDBP_PROTOTYPE);
    // Fallthrough to after special case handling
  } else if (_qdbp_is_boxed_int(obj) && label >= MAX_OP) {
    // Special case: `obj` is a boxed int and we're replacing a user-defined
    // method
    return _qdbp_boxed_int_replace(obj, label, code, captures, num_captures);
  }

  _qdbp_assert_kind(obj, QDBP_PROTOTYPE);
  struct _qdbp_field new_field = {
      .label = label,
      .method = {.captures = _qdbp_duplicate_captures(captures, num_captures),
                 .num_captures = num_captures,
                 .code = code}};
  if (!_QDBP_REUSE_ANALYSIS || !_qdbp_is_unique(obj)) {
    struct _qdbp_prototype new_prototype =
        prototype_copy_and_replace(&(obj->data.prototype), &new_field, label);
    _qdbp_dup_prototype_captures_except(&(obj->data.prototype), label);

    _qdbp_object_ptr new_obj = _qdbp_make_object(
        QDBP_PROTOTYPE, (union _qdbp_object_data){.prototype = new_prototype});

    _qdbp_drop(obj, 1);
    return new_obj;
  } else {
    // find the index to replace
    _qdbp_field_ptr field = _qdbp_label_get(&(obj->data.prototype), label);
    _qdbp_assert(field->label == label);
    // reuse the field
    _qdbp_del_method(&(field->method));
    field->label = new_field.label;
    field->method = new_field.method;
    return obj;
  }
}

_qdbp_object_ptr _qdbp_invoke_1(_qdbp_object_ptr receiver, _qdbp_label_t label,
                                _qdbp_object_ptr arg0) {
  if (_qdbp_is_unboxed_int(arg0)) {
    return _qdbp_unboxed_int_unary_op(arg0, label);
  } else if (_qdbp_is_boxed_int(arg0)) {
    if (label < ADD) {
      // Built-in math method
      return _qdbp_boxed_int_unary_op(arg0, label);
    } else {
      // User-defined method
      _qdbp_field_ptr field =
          _qdbp_label_get(&(arg0->data._qdbp_boxed_int->other_labels), label);
      _qdbp_dup_method_captures(&(field->method));
      return ((_qdbp_object_ptr(*)(_qdbp_object_arr,
                                   _qdbp_object_ptr))field->method.code)(
          field->method.captures, arg0);
    }
  } else {
    void *code;
    _qdbp_object_arr captures = _qdbp_get_method(receiver, label, &code);
    return ((_qdbp_object_ptr(*)(_qdbp_object_arr, _qdbp_object_ptr))code)(
        captures, arg0);
  }
}

_qdbp_object_ptr _qdbp_invoke_2(_qdbp_object_ptr receiver, _qdbp_label_t label,
                                _qdbp_object_ptr arg0, _qdbp_object_ptr arg1) {
  // arg0: unboxed int, arg1: unboxed int
  if (_qdbp_is_unboxed_int(arg0) && _qdbp_is_unboxed_int(arg1)) {
    return _qdbp_unboxed_int_binary_op(_qdbp_get_unboxed_int(arg0),
                                       _qdbp_get_unboxed_int(arg1), label);
  }
  // arg0: unboxed int, arg1: boxed int
  else if (_qdbp_is_unboxed_int(arg0) && _qdbp_is_boxed_int(arg1)) {
    _qdbp_object_ptr result = _qdbp_unboxed_int_binary_op(
        _qdbp_get_unboxed_int(arg0), _qdbp_get_boxed_int(arg1), label);
    _qdbp_drop(arg1, 1);
    return result;
  }
  // arg0: unboxed int, arg1: regular prototype
  else if (_qdbp_is_unboxed_int(arg0) &&
           _qdbp_get_kind(arg1) == QDBP_PROTOTYPE) {
    uint64_t a = _qdbp_get_unboxed_int(arg0);
    arg1 = _qdbp_invoke_1(arg1, VAL, arg1);
    _qdbp_assert(_qdbp_get_kind(arg1) == QDBP_INT);
    uint64_t b = arg1->data.i;
    _qdbp_drop(arg1, 1);
    return _qdbp_unboxed_int_binary_op(a, b, label);
  }
  // arg0: boxed int, arg1: unboxed int
  // arg0: boxed int, arg1: boxed int
  // arg0: boxed int, arg1: regular prototype
  else if (_qdbp_is_boxed_int(arg0)) {
    if (label < MAX_OP) {
      // built-in math method
      return _qdbp_boxed_int_binary_op(arg0, arg1, label);
    } else {
      _qdbp_field_ptr field =
          _qdbp_label_get(&arg0->data._qdbp_boxed_int->other_labels, label);
      _qdbp_object_ptr (*code)(_qdbp_object_arr, _qdbp_object_ptr,
                               _qdbp_object_ptr) =
          ((_qdbp_object_ptr(*)(_qdbp_object_arr, _qdbp_object_ptr,
                                _qdbp_object_ptr))field->method.code);
      _qdbp_object_arr captures = field->method.captures;
      return code(captures, arg0, arg1);
    }
  }
  // arg0: regular prototype, arg1: unboxed int
  // arg0: regular prototype, arg1: boxed int
  // arg0: regular prototype, arg1: regular prototype
  else {
    _qdbp_assert(_qdbp_get_kind(arg0) == QDBP_PROTOTYPE);
    void *code;
    _qdbp_object_arr captures = _qdbp_get_method(receiver, label, &code);
    return ((_qdbp_object_ptr(*)(_qdbp_object_arr, _qdbp_object_ptr,
                                 _qdbp_object_ptr))code)(captures, arg0, arg1);
  }
}
