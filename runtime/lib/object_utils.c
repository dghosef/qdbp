#include <stdio.h>
#include <string.h>

#include "runtime.h"

_qdbp_object_ptr _qdbp_make_object(_qdbp_tag_t tag,
                                   union _qdbp_object_data data) {
  _qdbp_object_ptr new_obj = _qdbp_obj_malloc();
  _qdbp_set_refcount(new_obj, 1);
  _qdbp_set_tag(new_obj, tag);
  new_obj->data = data;
  return new_obj;
}

_qdbp_object_ptr _qdbp_empty_prototype() { return NULL; }

_qdbp_object_ptr _qdbp_true() { return (_qdbp_object_ptr)(0xF0); }

_qdbp_object_ptr _qdbp_false() { return (_qdbp_object_ptr)(0xE0); }

_qdbp_object_ptr _qdbp_abort() {
  fprintf(stderr, "aborting\n");
  exit(1);
}

_qdbp_object_ptr _qdbp_match_failed() {
  _qdbp_assert(false);
  __builtin_unreachable();
}

void _qdbp_init_field(_qdbp_field_ptr field, _qdbp_label_t label,
                      struct _qdbp_object** captures, void* code,
                      uint32_t num_captures) {
  *field = (struct _qdbp_field){
      .label = label,
      .method =
          {
              .captures = captures,
              .code = code,
              .num_captures = num_captures,
          },
  };
}

bool _qdbp_is_unboxed_int(_qdbp_object_ptr obj) {
  return ((uintptr_t)obj & 1) == 1;
}

_qdbp_object_ptr _qdbp_make_unboxed_int(uintptr_t value) {
  return (_qdbp_object_ptr)((intptr_t)(value << 1) | 1);
}

uintptr_t _qdbp_get_unboxed_int(_qdbp_object_ptr obj) {
  _qdbp_assert(_qdbp_is_unboxed_int(obj));
  uintptr_t result = ((uintptr_t)obj) >> 1;
  return result;
}

bool _qdbp_is_boxed_int(_qdbp_object_ptr obj) {
  return !_qdbp_is_unboxed_int(obj) && _qdbp_get_kind(obj) == _QDBP_BOXED_INT;
}

_qdbp_object_ptr _qdbp_bool(bool value) {
  return value ? _qdbp_true() : _qdbp_false();
}

_qdbp_object_ptr _qdbp_make_string(const char* cstr, size_t length) {
  struct _qdbp_string* qdbp_str = _qdbp_qstring_malloc();
  qdbp_str->length = length;
  qdbp_str->value = _qdbp_cstring_malloc(length + 1);
  strcpy(qdbp_str->value, cstr);
  qdbp_str->prototype.label_map = NULL;
  _qdbp_object_ptr o = _qdbp_make_object(
      _QDBP_STRING, (union _qdbp_object_data){.string = qdbp_str});
  return o;
}

_qdbp_object_ptr _qdbp_make_boxed_int() {
  struct _qdbp_boxed_int* i = _qdbp_boxed_int_malloc();
  mpz_init(i->value);
  i->prototype.label_map = NULL;
  return _qdbp_make_object(_QDBP_BOXED_INT,
                           (union _qdbp_object_data){.boxed_int = i});
}

_qdbp_object_ptr _qdbp_make_boxed_int_from_cstr(const char* str) {
  struct _qdbp_boxed_int* i = _qdbp_boxed_int_malloc();
  i->prototype.label_map = NULL;
  mpz_init_set_str(i->value, str, 0);
  return _qdbp_make_object(_QDBP_BOXED_INT,
                           (union _qdbp_object_data){.boxed_int = i});
}
void _qdbp_del_fields(_qdbp_prototype_ptr proto) {
  _QDBP_HT_ITER(proto->label_map, field, tmp) {
    _qdbp_del_method(&field->method);
  }
  _qdbp_ht_del(proto->label_map);
}

void _qdbp_del_prototype(_qdbp_prototype_ptr proto) {
  _qdbp_assert(proto);
  _qdbp_del_fields(proto);
}

void _qdbp_del_variant(_qdbp_variant_ptr variant) {
  _qdbp_assert(variant);
  _qdbp_drop(variant->value, 1);
}

void _qdbp_del_method(_qdbp_method_ptr method) {
  _qdbp_assert(method);
  for (size_t i = 0; i < method->num_captures; i++) {
    _qdbp_drop((method->captures[i]), 1);
  }
  if (method->num_captures > 0) {
    _qdbp_capture_arr_free(method->captures);
  }
}

void _qdbp_del_obj(_qdbp_object_ptr obj) {
  _qdbp_assert(!_qdbp_is_unboxed_int(obj));
  switch (_qdbp_get_kind(obj)) {
    case _QDBP_BOXED_INT:
      _qdbp_del_prototype(&(obj->data.boxed_int->prototype));
      mpz_clear(obj->data.boxed_int->value);
      _qdbp_boxed_int_free(obj->data.boxed_int);
      break;
    case _QDBP_STRING:
      _qdbp_del_prototype(&(obj->data.string->prototype));
      _qdbp_cstring_free(obj->data.string->value);
      _qdbp_qstring_free(obj->data.string);
      break;
    case _QDBP_PROTOTYPE:
      _qdbp_del_prototype(&(obj->data.prototype));
      break;
    case _QDBP_VARIANT:
      _qdbp_del_variant(&(obj->data.variant));
      break;
  }
  _qdbp_obj_free(obj);
}