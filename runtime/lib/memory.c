#include <stdio.h>
#include <string.h>

#include "runtime.h"
#define MK_FREELIST(type, name)                              \
  struct _qdbp_##name##_t {                                  \
    type objects[_QDBP_FREELIST_SIZE];                       \
    size_t idx;                                              \
  };                                                         \
  struct _qdbp_##name##_t name = {.idx = 0, .objects = {0}}; \
                                                             \
  static type _qdbp_pop_##name() {                           \
    _qdbp_assert(name.idx != 0);                             \
    name.idx--;                                              \
    return name.objects[name.idx];                           \
  }                                                          \
  static bool _qdbp_push_##name(type obj) {                  \
    if (name.idx == _QDBP_FREELIST_SIZE) {                   \
      return false;                                          \
    }                                                        \
    name.objects[name.idx] = obj;                            \
    name.idx++;                                              \
    return true;                                             \
  }                                                          \
  static void _qdbp_##name##_free_all() {                    \
    for (size_t i = 0; i < name.idx; i++) {                  \
      _qdbp_free(name.objects[i]);                           \
    }                                                        \
  }

MK_FREELIST(_qdbp_object_ptr, obj_freelist)
MK_FREELIST(struct _qdbp_boxed_int*, box_freelist)

void* _qdbp_malloc(size_t size) {
  void* ptr = malloc(size);
  return ptr;
}

void _qdbp_free(void* ptr) {
  _qdbp_assert(!_qdbp_is_unboxed_int((_qdbp_object_ptr)ptr));
  free(ptr);
}

void _qdbp_memcpy(void* dest, const void* src, size_t size) {
  memcpy(dest, src, size);
}

void _qdbp_cleanup() {
  // Make sure that everything that has been malloc'd has been freed
  if (_QDBP_OBJ_FREELIST) {
    _qdbp_obj_freelist_free_all();
  }
  if (_QDBP_BOX_FREELIST) {
    _qdbp_box_freelist_free_all();
  }
}

void _qdbp_init() {
  for (int i = 0; i < _QDBP_FREELIST_SIZE; i++) {
    if (_QDBP_OBJ_FREELIST) {
      _qdbp_push_obj_freelist(_qdbp_malloc(sizeof(struct _qdbp_object)));
    }
    if (_QDBP_BOX_FREELIST) {
      _qdbp_push_box_freelist(_qdbp_malloc(sizeof(struct _qdbp_boxed_int)));
    }
  }
}

_qdbp_object_ptr _qdbp_make_boxed_int() {
  struct _qdbp_boxed_int* i;
  if (_QDBP_BOX_FREELIST && box_freelist.idx > 0) {
    i = _qdbp_pop_box_freelist();
  } else {
    i = _qdbp_malloc(sizeof(struct _qdbp_boxed_int));
  }
  mpz_init(i->value);
  i->prototype.label_map = NULL;
  return _qdbp_make_object(_QDBP_BOXED_INT,
                           (union _qdbp_object_data){.boxed_int = i});
}

_qdbp_object_ptr _qdbp_make_boxed_int_from_cstr(const char* str) {
  struct _qdbp_boxed_int* i;
  if (_QDBP_BOX_FREELIST && box_freelist.idx > 0) {
    i = _qdbp_pop_box_freelist();
  } else {
    i = _qdbp_malloc(sizeof(struct _qdbp_boxed_int));
  }
  i->prototype.label_map = NULL;
  mpz_init_set_str(i->value, str, 0);
  return _qdbp_make_object(_QDBP_BOXED_INT,
                           (union _qdbp_object_data){.boxed_int = i});
}

_qdbp_object_ptr _qdbp_malloc_obj() {
  if (_QDBP_OBJ_FREELIST && obj_freelist.idx > 0) {
    return _qdbp_pop_obj_freelist();
  } else {
    return _qdbp_malloc(sizeof(struct _qdbp_object));
  }
}

/* I think the problem is null capture array in find opt and here */
_qdbp_object_arr _qdbp_malloc_capture_arr(size_t size) {
  return _qdbp_malloc(sizeof(_qdbp_object_ptr) * size);
}

void _qdbp_free_boxed_int(struct _qdbp_boxed_int* i) {
  if (_QDBP_BOX_FREELIST && _qdbp_push_box_freelist(i)) {
  } else {
    _qdbp_free(i);
  }
}

void _qdbp_free_obj(_qdbp_object_ptr obj) {
  _qdbp_assert(!_qdbp_is_unboxed_int(obj));
  if (_QDBP_OBJ_FREELIST && _qdbp_push_obj_freelist(obj)) {
  } else {
    _qdbp_free((void*)obj);
  }
}

void _qdbp_free_capture_arr(_qdbp_object_arr arr) { _qdbp_free(arr); }

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
    _qdbp_free_capture_arr(method->captures);
  }
}

void _qdbp_del_obj(_qdbp_object_ptr obj) {
  _qdbp_assert(!_qdbp_is_unboxed_int(obj));
  switch (_qdbp_get_kind(obj)) {
    case _QDBP_BOXED_INT:
      _qdbp_del_prototype(&(obj->data.boxed_int->prototype));
      mpz_clear(obj->data.boxed_int->value);
      _qdbp_free(obj->data.boxed_int);
      break;
    case _QDBP_STRING:
      _qdbp_del_prototype(&(obj->data.string->prototype));
      _qdbp_free(obj->data.string->value);
      _qdbp_free(obj->data.string);
      break;
    case _QDBP_PROTOTYPE:
      _qdbp_del_prototype(&(obj->data.prototype));
      break;
    case _QDBP_VARIANT:
      _qdbp_del_variant(&(obj->data.variant));
      break;
  }
  _qdbp_free_obj(obj);
}
