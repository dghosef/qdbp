#include <stdio.h>

#include "runtime.h"
void _qdbp_label_add(_qdbp_prototype_ptr proto, _qdbp_label_t label,
                     _qdbp_field_ptr field, size_t defaultsize) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(proto);
    assert(field);
    // check that defaultsize is a pwr of 2
    assert((defaultsize & (defaultsize - 1)) == 0);
  }
  if (!proto->labels) {
    proto->labels = _qdbp_new_ht(defaultsize);
  }
  proto->labels = _qdbp_ht_insert(proto->labels, field);
}
_qdbp_field_ptr _qdbp_label_get(_qdbp_prototype_ptr proto,
                                _qdbp_label_t label) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(proto);
    assert(proto->labels);
  }
  return _qdbp_ht_find(proto->labels, label);
}

void _qdbp_copy_captures_except(_qdbp_prototype_ptr new_prototype,
                                _qdbp_label_t except) {
  // Copy all the capture arrays except the new one
  _qdbp_field_ptr field;
  size_t tmp;
  _QDBP_HT_ITER(new_prototype->labels, field, tmp) {
    if (field->label != except) {
      _qdbp_object_arr original = field->method.captures;
      field->method.captures =
          _qdbp_make_capture_arr(field->method.captures_size);
      _qdbp_memcpy(field->method.captures, original,
                   sizeof(_qdbp_object_ptr) * field->method.captures_size);
    }
  }
}

static struct _qdbp_prototype raw_prototype_replace(
    const _qdbp_prototype_ptr src, const _qdbp_field_ptr new_field,
    _qdbp_label_t new_label) {
  size_t src_size = _qdbp_proto_size(src);
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(src_size);
    assert(new_field->label == new_label);
  }
  struct _qdbp_prototype new_prototype = {.labels = NULL};

  // FIXME: Have the overwriting done in _qdbp_duplicate_labels
  // to avoid extra work of inserting twice
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(new_prototype.labels == NULL);
  }
  _qdbp_duplicate_labels(src, &new_prototype);

  *(_qdbp_ht_find(new_prototype.labels, new_label)) = *new_field;
  _qdbp_copy_captures_except(&new_prototype, new_label);
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(new_prototype.labels);
  }
  return new_prototype;
}

static struct _qdbp_prototype raw_prototype_extend(
    const _qdbp_prototype_ptr src, const _qdbp_field_ptr new_field,
    size_t new_label, size_t defaultsize) {
  // copy src
  size_t src_size = _qdbp_proto_size(src);
  struct _qdbp_prototype new_prototype = {.labels = NULL};

  _qdbp_duplicate_labels(src, &new_prototype);
  _qdbp_label_add(&new_prototype, new_label, new_field, defaultsize);
  _qdbp_copy_captures_except(&new_prototype, new_label);
  return new_prototype;
}

_qdbp_object_arr _qdbp_get_method(_qdbp_object_ptr obj, _qdbp_label_t label,
                                  void **code_ptr /*output param*/) {
  _qdbp_assert_obj_kind(obj, QDBP_PROTOTYPE);
  struct _qdbp_method m =
      _qdbp_label_get(&(obj->data.prototype), label)->method;
  _qdbp_dup_captures(&m);
  *code_ptr = m.code;
  _qdbp_object_arr ret = m.captures;
  return ret;
}

_qdbp_object_arr _qdbp_make_captures(_qdbp_object_arr captures, size_t size) {
  if (size == 0) {
    return NULL;
  } else {
    _qdbp_object_arr out = (_qdbp_object_arr)_qdbp_malloc(
        sizeof(struct _qdbp_object *) * size, "captures");
    _qdbp_memcpy(out, captures, sizeof(struct _qdbp_object *) * size);
    return out;
  }
}

_qdbp_object_ptr _qdbp_extend(_qdbp_object_ptr obj, _qdbp_label_t label,
                              void *code, _qdbp_object_arr captures,
                              size_t captures_size, size_t defaultsize) {
  if (!obj) {
    obj = _qdbp_make_object(
        QDBP_PROTOTYPE,
        (union _qdbp_object_data){
            .prototype = {.labels = _qdbp_new_ht(defaultsize)}});
  } else if (_qdbp_is_unboxed_int(obj)) {
    return _qdbp_box(obj, label, code, captures, captures_size);
  } else if (_qdbp_is_boxed_int(obj)) {
    return _qdbp_box_extend(obj, label, code, captures, captures_size);
  }
  struct _qdbp_field f = {
      .label = label,
      .method = {.captures = _qdbp_make_captures(captures, captures_size),
                 .captures_size = captures_size,
                 .code = code}};
  _qdbp_assert_obj_kind(obj, QDBP_PROTOTYPE);
  if (!_QDBP_REUSE_ANALYSIS || !_qdbp_is_unique(obj)) {
    _qdbp_prototype_ptr prototype = &(obj->data.prototype);
    struct _qdbp_prototype new_prototype =
        raw_prototype_extend(prototype, &f, label, defaultsize);
    _qdbp_object_ptr new_obj = _qdbp_make_object(
        QDBP_PROTOTYPE, (union _qdbp_object_data){.prototype = new_prototype});
    _qdbp_dup_prototype_captures(prototype);
    _qdbp_drop(obj, 1);
    return new_obj;
  } else {
    _qdbp_label_add(&(obj->data.prototype), label, &f, defaultsize);
    return obj;
  }
}
_qdbp_object_ptr _qdbp_replace(_qdbp_object_ptr obj, _qdbp_label_t label,
                               void *code, _qdbp_object_arr captures,
                               size_t captures_size) {
  if (!obj) {
    obj = _qdbp_make_object(QDBP_PROTOTYPE, (union _qdbp_object_data){
                                                .prototype = {.labels = NULL}});
  } else if (_qdbp_is_unboxed_int(obj)) {
    obj = _qdbp_make_int_proto(_qdbp_get_unboxed_int(obj));
    _qdbp_assert_obj_kind(obj, QDBP_PROTOTYPE);
  } else if (_qdbp_is_boxed_int(obj) && label < MAX_OP) {
    int64_t i = _qdbp_get_boxed_int(obj);
    _qdbp_drop(obj, 1);
    obj = _qdbp_make_int_proto(i);
    _qdbp_assert_obj_kind(obj, QDBP_PROTOTYPE);
  } else if (_qdbp_is_boxed_int(obj) && label >= MAX_OP) {
    return _qdbp_boxed_int_replace(obj, label, code, captures, captures_size);
  }

  _qdbp_assert_obj_kind(obj, QDBP_PROTOTYPE);
  struct _qdbp_field f = {
      .label = label,
      .method = {.captures = _qdbp_make_captures(captures, captures_size),
                 .captures_size = captures_size,
                 .code = code}};
  if (!_QDBP_REUSE_ANALYSIS || !_qdbp_is_unique(obj)) {
    struct _qdbp_prototype new_prototype =
        raw_prototype_replace(&(obj->data.prototype), &f, label);
    _qdbp_dup_prototype_captures_except(&(obj->data.prototype), label);

    _qdbp_object_ptr new_obj = _qdbp_make_object(
        QDBP_PROTOTYPE, (union _qdbp_object_data){.prototype = new_prototype});

    _qdbp_drop(obj, 1);
    return new_obj;
  } else {
    // find the index to replace
    _qdbp_field_ptr field = _qdbp_label_get(&(obj->data.prototype), label);
    assert(field->label == label);
    // reuse the field
    _qdbp_del_method(&(field->method));
    field->label = f.label;
    field->method = f.method;
    return obj;
  }
}

size_t _qdbp_proto_size(_qdbp_prototype_ptr proto) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(proto);
  }
  return _qdbp_hashtable_size(proto->labels);
}
_qdbp_object_ptr _qdbp_invoke_1(_qdbp_object_ptr receiver, _qdbp_label_t label,
                                _qdbp_object_ptr arg0) {
  if (_qdbp_is_unboxed_int(arg0)) {
    return _qdbp_unboxed_unary_op(arg0, label);
  } else if (_qdbp_is_boxed_int(arg0)) {
    if (label < ADD) {
      return _qdbp_boxed_unary_op(arg0, label);
    } else {
      _qdbp_field_ptr field =
          _qdbp_label_get(&(arg0->data._qdbp_boxed_int->other_labels), label);
      _qdbp_dup_captures(&(field->method));
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
  // UB UB
  if (_qdbp_is_unboxed_int(arg0) && _qdbp_is_unboxed_int(arg1)) {
    return _qdbp_unboxed_binary_op(_qdbp_get_unboxed_int(arg0),
                                   _qdbp_get_unboxed_int(arg1), label);
  }
  // UB B
  else if (_qdbp_is_unboxed_int(arg0) && _qdbp_is_boxed_int(arg1)) {
    _qdbp_object_ptr result = _qdbp_unboxed_binary_op(
        _qdbp_get_unboxed_int(arg0), _qdbp_get_boxed_int(arg1), label);
    _qdbp_drop(arg1, 1);
    return result;
  }
  // UB BB
  else if (_qdbp_is_unboxed_int(arg0) &&
           _qdbp_get_kind(arg1) == QDBP_PROTOTYPE) {
    int64_t a = _qdbp_get_unboxed_int(arg0);
    arg1 = _qdbp_invoke_1(arg1, VAL, arg1);
    if (_QDBP_DYNAMIC_TYPECHECK) {
      assert(_qdbp_get_kind(arg1) == QDBP_INT);
    }
    int64_t b = arg1->data.i;
    _qdbp_drop(arg1, 1);
    return _qdbp_unboxed_binary_op(a, b, label);
  }
  // B UB
  // B B
  // B BB
  else if (_qdbp_is_boxed_int(arg0)) {
    if (label < MAX_OP) {
      // print label
      return _qdbp_boxed_binary_op(arg0, arg1, label);
    } else {
      // get the method from the binary op
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
  // BB UB
  // BB B

  else {
    if (_QDBP_DYNAMIC_TYPECHECK) {
      assert(_qdbp_get_kind(arg0) == QDBP_PROTOTYPE);
    }
    void *code;
    _qdbp_object_arr captures = _qdbp_get_method(receiver, label, &code);
    return ((_qdbp_object_ptr(*)(_qdbp_object_arr, _qdbp_object_ptr,
                                 _qdbp_object_ptr))code)(captures, arg0, arg1);
  }
}
