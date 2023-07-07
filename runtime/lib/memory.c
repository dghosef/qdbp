#include <stdio.h>
#include <string.h>

#include "runtime.h"
#include "uthash.h"

struct malloc_entry {
  void* ptr;
  const char* msg;
  UT_hash_handle hh;
};
static struct malloc_entry* active_mallocs = NULL;

static void add_to_malloc_list(void* ptr, const char* msg) {
  struct malloc_entry* m;
  HASH_FIND_PTR(active_mallocs, &ptr, m);
  if (m == NULL) {
    m = malloc(sizeof(struct malloc_entry));
    m->msg = msg;
    m->ptr = ptr;
    HASH_ADD_PTR(active_mallocs, ptr, m);
  } else {
    printf("Internal error - Double malloc: %s\n", msg);
    assert(false);
  }
}

static void remove_from_malloc_list(void* ptr) {
  struct malloc_entry* m;
  HASH_FIND_PTR(active_mallocs, &ptr, m);
  if (m) {
    HASH_DEL(active_mallocs, m);
    free(m);
  } else {
    printf("Freeing unallocated memory\n");
    assert(false);
  }
}

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
  static void _qdbp_##name##_free_all() { \
    for (size_t i = 0; i < name.idx; i++) {                  \
      _qdbp_free(name.objects[i]);                           \
    }                                                        \
  }

MK_FREELIST(_qdbp_object_ptr, obj_freelist)
MK_FREELIST(struct _qdbp_boxed_int*, box_freelist)

void* _qdbp_malloc_internal(size_t size, const char* msg) {
  void* ptr = malloc(size);
  if (_QDBP_MEMORY_SANITIZE) {
    add_to_malloc_list(ptr, msg);
  }
  assert(ptr);
  return ptr;
}

void _qdbp_free(void* ptr) {
  _qdbp_assert(!_qdbp_is_unboxed_int((_qdbp_object_ptr)ptr));
  if (_QDBP_MEMORY_SANITIZE) {
    remove_from_malloc_list(ptr);
  }
  free(ptr);
}

void _qdbp_memcpy(void* dest, const void* src, size_t size) {
  memcpy(dest, src, size);
}

bool _qdbp_check_mem() {
  // Make sure that everything that has been malloc'd has been freed
  if (_QDBP_MEMORY_SANITIZE) {
    if (_QDBP_OBJ_FREELIST) {
      _qdbp_obj_freelist_free_all();
    }
    if (_QDBP_BOX_FREELIST) {
      _qdbp_box_freelist_free_all();
    }
    struct malloc_entry* cur;
    struct malloc_entry* tmp;
    bool failed = false;
    HASH_ITER(hh, active_mallocs, cur, tmp) {
      failed = true;
      fprintf(stderr, "Memory leak at %p: %s\n", cur->ptr, cur->msg);
      HASH_DEL(active_mallocs, cur);
      free(cur);
    }
    _qdbp_assert(active_mallocs == NULL);
    return !failed;
  }
  return true;
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

struct _qdbp_boxed_int* _qdbp_malloc_boxed_int() {
  if (_QDBP_BOX_FREELIST && box_freelist.idx > 0) {
    return _qdbp_pop_box_freelist();
  } else {
    return _qdbp_malloc(sizeof(struct _qdbp_boxed_int));
  }
}

_qdbp_object_ptr _qdbp_malloc_obj() {
  if (_QDBP_OBJ_FREELIST && obj_freelist.idx > 0) {
    return _qdbp_pop_obj_freelist();
  } else {
    return _qdbp_malloc(sizeof(struct _qdbp_object));
  }
}

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

void _qdbp_free_capture_arr(_qdbp_object_arr arr, size_t size) {
  _qdbp_free(arr);
}

void _qdbp_del_fields(_qdbp_prototype_ptr proto) {
  struct _qdbp_field* cur_field;
  _qdbp_field_ptr field;
  size_t tmp;
  _QDBP_HT_ITER(proto->labels, field, tmp) { _qdbp_del_method(&field->method); }
  _qdbp_ht_del(proto->labels);
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
    _qdbp_free_capture_arr(method->captures, method->num_captures);
  }
}

void _qdbp_del_obj(_qdbp_object_ptr obj) {
  _qdbp_assert(!_qdbp_is_unboxed_int(obj));
  switch (_qdbp_get_kind(obj)) {
    case QDBP_INT:
      break;
    case QDBP_FLOAT:
      break;
    case QDBP_BOXED_INT:
      _qdbp_del_prototype(&(obj->data._qdbp_boxed_int->other_labels));
      _qdbp_free(obj->data._qdbp_boxed_int);
      break;
    case QDBP_STRING:
      _qdbp_free(obj->data.s);
      break;
    case QDBP_PROTOTYPE:
      _qdbp_del_prototype(&(obj->data.prototype));
      break;
    case QDBP_VARIANT:
      _qdbp_del_variant(&(obj->data.variant));
      break;
  }
  _qdbp_free_obj(obj);
}
