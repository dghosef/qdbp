#include <stdio.h>

#include "runtime.h"

void _qdbp_incref(_qdbp_object_ptr obj, _qdbp_refcount_t amount) {
  if (!_QDBP_REFCOUNT) {
    return;
  }
  if (_QDBP_ASSERTS_ENABLED &&
      (_qdbp_is_unboxed_int(obj) || obj == _qdbp_true() ||
       obj == _qdbp_false() || !obj)) {
    _qdbp_assert(false);
  }
  obj->metadata.rc += amount;
}

void _qdbp_decref(_qdbp_object_ptr obj, _qdbp_refcount_t amount) {
  if (!_QDBP_REFCOUNT) {
    return;
  }
  if (_QDBP_ASSERTS_ENABLED &&
      (_qdbp_is_unboxed_int(obj) || obj == _qdbp_true() ||
       obj == _qdbp_false() || !obj)) {
    _qdbp_assert(false);
  }
  obj->metadata.rc -= amount;
}

void _qdbp_set_refcount(_qdbp_object_ptr obj, _qdbp_refcount_t refcount) {
  if (!_QDBP_REFCOUNT) {
    return;
  }
  if (_QDBP_ASSERTS_ENABLED &&
      (_qdbp_is_unboxed_int(obj) || obj == _qdbp_true() ||
       obj == _qdbp_false() || !obj)) {
    _qdbp_assert(false);
  }
  obj->metadata.rc = refcount;
}

_qdbp_refcount_t _qdbp_get_refcount(_qdbp_object_ptr obj) {
  if (!_QDBP_REFCOUNT) {
    return 100;
  }
  if (_QDBP_ASSERTS_ENABLED &&
      (_qdbp_is_unboxed_int(obj) || obj == _qdbp_true() ||
       obj == _qdbp_false() || !obj)) {
    _qdbp_assert(false);
  }
  return obj->metadata.rc;
}

void _qdbp_drop(_qdbp_object_ptr obj, _qdbp_refcount_t cnt) {
  if (!_QDBP_REFCOUNT) {
    return;
  }
  if (!obj || _qdbp_is_unboxed_int(obj) || obj == _qdbp_true() ||
      obj == _qdbp_false()) {
    return;
  } else {
    _qdbp_assert_refcount(obj);
    _qdbp_assert(_qdbp_get_refcount(obj) >= cnt);
    _qdbp_decref(obj, cnt);
    if (_qdbp_get_refcount(obj) == 0) {
      _qdbp_del_obj(obj);
      return;
    }
    return;
  }
}

void _qdbp_obj_dup(_qdbp_object_ptr obj, _qdbp_refcount_t cnt) {
  if (!_QDBP_REFCOUNT) {
    return;
  }
  if (!obj || _qdbp_is_unboxed_int(obj) || obj == _qdbp_true() ||
      obj == _qdbp_false()) {
    return;
  } else {
    _qdbp_assert_refcount(obj);
    _qdbp_incref(obj, cnt);
  }
}

void _qdbp_dup_captures(_qdbp_method_ptr method) {
  if (!_QDBP_REFCOUNT) {
    return;
  }
  for (size_t i = 0; i < method->captures_size; i++) {
    _qdbp_obj_dup((method->captures[i]), 1);
  }
}

void _qdbp_dup_prototype_captures(_qdbp_prototype_ptr proto) {
  if (!_QDBP_REFCOUNT) {
    return;
  }
  _qdbp_field_ptr field;
  size_t tmp;
  _QDBP_HT_ITER(proto->labels, field, tmp) {
    _qdbp_dup_captures(&(field->method));
  }
}

void _qdbp_dup_prototype_captures_except(_qdbp_prototype_ptr proto,
                                         _qdbp_label_t except) {
  if (!_QDBP_REFCOUNT) {
    return;
  }
  _qdbp_field_ptr field;
  size_t tmp;
  _QDBP_HT_ITER(proto->labels, field, tmp) {
    if (field->label != except) {
      _qdbp_dup_captures(&(field->method));
    }
  }
}

bool _qdbp_is_unique(_qdbp_object_ptr obj) {
  if (!_QDBP_REFCOUNT) {
    return false;
  }
  if (_qdbp_is_unboxed_int(obj) || obj == _qdbp_true() ||
      obj == _qdbp_false() || !obj) {
    return true;
  } else {
    _qdbp_assert(obj);
    _qdbp_assert(_qdbp_get_refcount(obj) >= 0);
    return _qdbp_get_refcount(obj) == 1;
  }
}
