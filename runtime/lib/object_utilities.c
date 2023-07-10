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

_qdbp_bigint_t* _qdbp_make_bigint(uint64_t v) {
  _qdbp_bigint_t *i = _qdbp_malloc(sizeof(_qdbp_bigint_t));
  *i = v;
  return i;
}

_qdbp_object_ptr _qdbp_make_boxed_int(uint64_t i) {
  _qdbp_object_ptr ret = _qdbp_make_object(
      QDBP_BOXED_INT, (union _qdbp_object_data)_qdbp_malloc_boxed_int());
  ret->data._qdbp_boxed_int->value = i;
  ret->data._qdbp_boxed_int->other_labels.label_map = NULL;
  return ret;
}

_qdbp_object_ptr _qdbp_empty_prototype() { return NULL; }

_qdbp_object_ptr _qdbp_true() { return (_qdbp_object_ptr)(0xF0); }

_qdbp_object_ptr _qdbp_false() { return (_qdbp_object_ptr)(0xE0); }

_qdbp_object_ptr _qdbp_int(uint64_t i) {
  _qdbp_object_ptr new_obj = _qdbp_make_object(
      QDBP_INT, (union _qdbp_object_data){.i = _qdbp_make_bigint(i)});
  return new_obj;
}

_qdbp_object_ptr _qdbp_string(const char *src) {
  char *dest = (char *)_qdbp_malloc(strlen(src) + 1);
  strcpy(dest, src);
  _qdbp_object_ptr new_obj =
      _qdbp_make_object(QDBP_STRING, (union _qdbp_object_data){.s = dest});
  return new_obj;
}

_qdbp_object_ptr _qdbp_float(double f) {
  _qdbp_object_ptr new_obj =
      _qdbp_make_object(QDBP_FLOAT, (union _qdbp_object_data){.f = f});
  return new_obj;
}

_qdbp_object_ptr _qdbp_abort() {
  fprintf(stderr, "aborting\n");
  exit(0);
}

_qdbp_object_ptr _qdbp_match_failed() {
  _qdbp_assert(false);
  __builtin_unreachable();
}

void _qdbp_init_field(_qdbp_field_ptr field, _qdbp_label_t label,
                      struct _qdbp_object **captures, void *code,
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