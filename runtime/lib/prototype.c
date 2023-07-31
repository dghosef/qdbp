#include <stdio.h>

#include "runtime.h"

size_t _qdbp_prototype_size(_qdbp_prototype_ptr proto) {
  _qdbp_assert(proto);
  return _qdbp_ht_size(proto->label_map);
}

void _qdbp_label_add(_qdbp_prototype_ptr proto, _qdbp_field_ptr field,
                     size_t default_capacity) {
  _qdbp_assert(proto);
  _qdbp_assert(field);
  // check that default_capacity is a pwr of 2
  _qdbp_assert((default_capacity & (default_capacity - 1)) == 0);
  if (!proto->label_map) {
    proto->label_map = _qdbp_ht_new(default_capacity);
  }
  proto->label_map = _qdbp_ht_insert(proto->label_map, field);
}

_qdbp_field_ptr _qdbp_label_get(_qdbp_prototype_ptr proto,
                                _qdbp_label_t label) {
  return _qdbp_ht_find(proto->label_map, label);
}

_qdbp_field_ptr _qdbp_label_get_opt(_qdbp_prototype_ptr proto,
                                           _qdbp_label_t label) {
  return _qdbp_ht_find_opt(proto->label_map, label);
}

void _qdbp_copy_prototype(_qdbp_prototype_ptr src, _qdbp_prototype_ptr dest) {
  _qdbp_assert(!dest->label_map);
  dest->label_map = _qdbp_ht_duplicate(src->label_map);
}

void _qdbp_make_fresh_captures_except(_qdbp_prototype_ptr new_prototype,
                                      _qdbp_label_t except) {
  // Copy all the capture arrays except the new one
  _qdbp_field_ptr field;
  size_t tmp;
  _QDBP_HT_ITER(new_prototype->label_map, field, tmp) {
    if (field->label != except) {
      _qdbp_object_arr original = field->method.captures;
      field->method.captures =
          _qdbp_malloc_capture_arr(field->method.num_captures);
      _qdbp_memcpy(field->method.captures, original,
                   sizeof(_qdbp_object_ptr) * field->method.num_captures);
    }
  }
}

_qdbp_object_arr _qdbp_copy_captures(_qdbp_object_arr captures,
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
  _qdbp_prototype_ptr proto;
  if (_qdbp_get_kind(obj) == _QDBP_PROTOTYPE) {
    proto = &(obj->data.prototype);
  } else {
    _qdbp_assert_kind(obj, _QDBP_BOXED_INT);
    proto = &(obj->data.boxed_int->prototype);
  }
  struct _qdbp_method m = _qdbp_label_get(proto, label)->method;
  _qdbp_dup_method_captures(&m);
  *code = m.code;
  _qdbp_object_arr ret = m.captures;
  return ret;
}

_qdbp_object_arr _qdbp_get_method_opt(_qdbp_object_ptr obj, _qdbp_label_t label,
                                      void **code /*output param*/) {
  _qdbp_prototype_ptr proto;
  if(_qdbp_is_unboxed_int(obj)) {
    return NULL;
  }
  else if (_qdbp_get_kind(obj) == _QDBP_PROTOTYPE) {
    proto = &(obj->data.prototype);
  } else if (_qdbp_get_kind(obj) == _QDBP_BOXED_INT) {
    proto = &(obj->data.boxed_int->prototype);
  } else {
    _qdbp_assert(false);
    __builtin_unreachable();
  }
  _qdbp_field_ptr field = _qdbp_label_get_opt(proto, label);
  if (!field) {
    return NULL;
  }
  struct _qdbp_method m = field->method;
  _qdbp_dup_method_captures(&m);
  *code = m.code;
  _qdbp_object_arr ret = m.captures;
  return ret;
}

static struct _qdbp_prototype prototype_copy_and_extend(
    const _qdbp_prototype_ptr src, const _qdbp_field_ptr new_field,
    size_t new_label, size_t default_capacity) {
  // copy src
  struct _qdbp_prototype new_prototype = {.label_map = NULL};

  _qdbp_copy_prototype(src, &new_prototype);
  _qdbp_label_add(&new_prototype, new_field, default_capacity);
  _qdbp_make_fresh_captures_except(&new_prototype, new_label);
  return new_prototype;
}

static _qdbp_object_ptr copy_obj_and_update_proto(
    _qdbp_object_ptr obj, struct _qdbp_prototype new_prototype) {
  _qdbp_object_ptr new_obj;
  if (_qdbp_get_kind(obj) == _QDBP_PROTOTYPE) {
    new_obj = _qdbp_make_object(
        _QDBP_PROTOTYPE, (union _qdbp_object_data){.prototype = new_prototype});
  } else {
    new_obj = _qdbp_make_boxed_int();
    new_obj->data.boxed_int->prototype = new_prototype;
    mpz_set(new_obj->data.boxed_int->value, obj->data.boxed_int->value);
  }
  return new_obj;
}

_qdbp_object_ptr _qdbp_extend(_qdbp_object_ptr obj, _qdbp_label_t label,
                              void *code, _qdbp_object_arr captures,
                              size_t num_captures, size_t default_capacity) {
  if (!obj) {
    // Special case: `obj` is the empty prototype
    obj = _qdbp_make_object(
        _QDBP_PROTOTYPE,
        (union _qdbp_object_data){
            .prototype = {.label_map = _qdbp_ht_new(default_capacity)}});
    // Fallthrough to after special case handling
  }
  struct _qdbp_field new_field = {
      .label = label,
      .method = {.captures = _qdbp_copy_captures(captures, num_captures),
                 .num_captures = num_captures,
                 .code = code}};
  _qdbp_prototype_ptr original_prototype;
  if (_qdbp_get_kind(obj) == _QDBP_PROTOTYPE) {
    original_prototype = &(obj->data.prototype);
  } else {
    _qdbp_assert_kind(obj, _QDBP_BOXED_INT);
    original_prototype = &(obj->data.boxed_int->prototype);
  }
  if (!_QDBP_REUSE_ANALYSIS || !_qdbp_is_unique(obj)) {
    _qdbp_prototype_ptr prototype = original_prototype;
    struct _qdbp_prototype new_prototype = prototype_copy_and_extend(
        prototype, &new_field, label, default_capacity);
    _qdbp_object_ptr new_obj = copy_obj_and_update_proto(obj, new_prototype);
    _qdbp_dup_prototype_captures(prototype);
    _qdbp_drop(obj, 1);
    return new_obj;
  } else {
    _qdbp_label_add(original_prototype, &new_field, default_capacity);
    return obj;
  }
}

static struct _qdbp_prototype prototype_copy_and_replace(
    const _qdbp_prototype_ptr src, const _qdbp_field_ptr new_field,
    _qdbp_label_t new_label) {
  size_t src_size = _qdbp_prototype_size(src);
  _qdbp_assert(src_size > 0);
  _qdbp_assert(new_field->label == new_label);
  struct _qdbp_prototype new_prototype = {.label_map = NULL};

  _qdbp_assert(new_prototype.label_map == NULL);
  _qdbp_copy_prototype(src, &new_prototype);

  *(_qdbp_ht_find(new_prototype.label_map, new_label)) = *new_field;
  _qdbp_make_fresh_captures_except(&new_prototype, new_label);
  _qdbp_assert(new_prototype.label_map);
  return new_prototype;
}

_qdbp_object_ptr _qdbp_replace(_qdbp_object_ptr obj, _qdbp_label_t label,
                               void *code, _qdbp_object_arr captures,
                               size_t num_captures) {
  if (!obj) {
    // Special case: `obj` is the empty prototype
    obj = _qdbp_make_object(
        _QDBP_PROTOTYPE,
        (union _qdbp_object_data){.prototype = {.label_map = NULL}});
    // Fallthrough to after special case handling
  }

  struct _qdbp_field new_field = {
      .label = label,
      .method = {.captures = _qdbp_copy_captures(captures, num_captures),
                 .num_captures = num_captures,
                 .code = code}};
  _qdbp_prototype_ptr original_prototype;
  if (_qdbp_get_kind(obj) == _QDBP_PROTOTYPE) {
    original_prototype = &(obj->data.prototype);
  } else {
    _qdbp_assert_kind(obj, _QDBP_BOXED_INT);
    original_prototype = &(obj->data.boxed_int->prototype);
  }
  if (!_QDBP_REUSE_ANALYSIS || !_qdbp_is_unique(obj)) {
    struct _qdbp_prototype new_prototype =
        prototype_copy_and_replace(original_prototype, &new_field, label);
    _qdbp_dup_prototype_captures_except(original_prototype, label);

    _qdbp_object_ptr new_obj = copy_obj_and_update_proto(obj, new_prototype);
    _qdbp_drop(obj, 1);
    return new_obj;
  } else {
    // find the field to replace
    _qdbp_field_ptr field = _qdbp_label_get(original_prototype, label);
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
  void *code;
  _qdbp_object_arr captures = _qdbp_get_method(receiver, label, &code);
  return ((_qdbp_object_ptr(*)(_qdbp_object_arr, _qdbp_object_ptr))code)(
      captures, arg0);
}

_qdbp_object_ptr _qdbp_invoke_2(_qdbp_object_ptr receiver, _qdbp_label_t label,
                                _qdbp_object_ptr arg0, _qdbp_object_ptr arg1) {
  void *code;
  _qdbp_object_arr captures = _qdbp_get_method_opt(receiver, label, &code);
  if (!captures) {
    return _qdbp_int_binary_op(arg0, arg1, label);
  } else {
    return ((_qdbp_object_ptr(*)(_qdbp_object_arr, _qdbp_object_ptr,
                                 _qdbp_object_ptr))code)(captures, arg0, arg1);
  }
}
