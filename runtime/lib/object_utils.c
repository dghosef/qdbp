#include <stdio.h>
#include <string.h>

#include "runtime.h"

_qdbp_object_ptr _qdbp_make_object(_qdbp_tag_t tag,
                                   union _qdbp_object_data data) {
  _qdbp_object_ptr new_obj = _qdbp_malloc_obj();
  _qdbp_set_refcount(new_obj, 1);
  _qdbp_set_tag(new_obj, tag);
  new_obj->data = data;
  return new_obj;
}

_qdbp_object_ptr _qdbp_empty_prototype() { return NULL; }

// TODO: instead of special casing true and false, we could
// special case all variants with no payload
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
  uintptr_t result = ((intptr_t)obj) >> 1;
  return result;
}

bool _qdbp_is_boxed_int(_qdbp_object_ptr obj) {
  return !_qdbp_is_unboxed_int(obj) && _qdbp_get_kind(obj) == _QDBP_BOXED_INT;
}

_qdbp_object_ptr _qdbp_bool(bool value) {
  return value ? _qdbp_true() : _qdbp_false();
}

_qdbp_object_ptr _qdbp_make_string(const char* cstr, size_t length) {
  struct _qdbp_string* qdbp_str = _qdbp_malloc(sizeof(struct _qdbp_string));
  qdbp_str->length = length;
  qdbp_str->value = _qdbp_malloc(length + 1);
  strcpy(qdbp_str->value, cstr);
  qdbp_str->prototype.label_map = NULL;
  _qdbp_object_ptr o = _qdbp_make_object(
      _QDBP_STRING, (union _qdbp_object_data){.string = qdbp_str});
  return o;
}