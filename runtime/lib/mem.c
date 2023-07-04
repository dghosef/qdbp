#include <stdio.h>
#include <string.h>

#include "runtime.h"

// Keep track of mallocs and frees
struct malloc_list_node {
  void *ptr;
  struct malloc_list_node *next;
  const char *msg;
};
static struct malloc_list_node *malloc_list = NULL;

static bool malloc_list_contains(void *ptr) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(!_qdbp_is_unboxed_int((_qdbp_object_ptr)ptr));
  }
  struct malloc_list_node *node = malloc_list;
  while (node) {
    if (node->ptr == ptr) {
      return true;
    }
    node = node->next;
  }
  return false;
}
static void add_to_malloc_list(void *ptr, const char *msg) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(!_qdbp_is_unboxed_int((_qdbp_object_ptr)ptr));
  }
  assert(!malloc_list_contains(ptr));
  struct malloc_list_node *new_node =
      (struct malloc_list_node *)malloc(sizeof(struct malloc_list_node));
  new_node->ptr = ptr;
  new_node->next = malloc_list;
  new_node->msg = msg;
  malloc_list = new_node;
}
static void remove_from_malloc_list(void *ptr) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(!_qdbp_is_unboxed_int((_qdbp_object_ptr)ptr));
  }
  assert(malloc_list_contains(ptr));
  struct malloc_list_node *node = malloc_list;
  struct malloc_list_node *prev = NULL;
  while (node) {
    if (node->ptr == ptr) {
      if (prev) {
        prev->next = node->next;
      } else {
        malloc_list = node->next;
      }
      free(node);
      return;
    }
    prev = node;
    node = node->next;
  }
  assert(false);
}
// TODO: Macroify?
void *_qdbp_malloc(size_t size, const char *msg) {
  if (size == 0) {
    return NULL;
  }
  void *ptr;
  ptr = malloc(size);
  if (_QDBP_CHECK_MALLOC_FREE) {
    add_to_malloc_list(ptr, msg);
  }
  assert(ptr);
  return ptr;
}
void _qdbp_free(void *ptr) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(!_qdbp_is_unboxed_int((_qdbp_object_ptr)ptr));
  }
  if (_QDBP_CHECK_MALLOC_FREE) {
    remove_from_malloc_list(ptr);
  }
  free(ptr);
}

#define MK_FREELIST(type, name)                              \
  struct _qdbp_##name##_t {                                   \
    type objects[_QDBP_FREELIST_SIZE];                       \
    size_t idx;                                              \
  };                                                         \
  struct _qdbp_##name##_t name = {.idx = 0, .objects = {0}}; \
                                                             \
  static type _qdbp_pop_##name() {                           \
    if (name.idx == 0) {                                     \
      assert(false);                                         \
    }                                                        \
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
  static void _qdbp_##name##_remove_all_from_malloc_list() { \
    for (size_t i = 0; i < name.idx; i++) {                  \
      remove_from_malloc_list(name.objects[i]);              \
    }                                                        \
  }
void _qdbp_free_fields(_qdbp_prototype_ptr proto) {
  struct _qdbp_field *cur_field;

  _qdbp_field_ptr field;
  size_t tmp;
  _QDBP_HT_ITER(proto->labels, field, tmp) { _qdbp_del_method(&field->method); }
  _qdbp_del_ht(proto->labels);
}

MK_FREELIST(_qdbp_object_ptr, freelist)

void _qdbp_free_obj(_qdbp_object_ptr obj) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(!_qdbp_is_unboxed_int(obj));
  }
  if (_QDBP_OBJ_FREELIST && _qdbp_push_freelist(obj)) {
  } else {
    _qdbp_free((void *)obj);
  }
}

_qdbp_object_ptr _qdbp_malloc_obj() {
  if (_QDBP_OBJ_FREELIST && freelist.idx > 0) {
    return _qdbp_pop_freelist();
  } else {
    return (_qdbp_object_ptr)_qdbp_malloc(sizeof(struct _qdbp_object), "field");
  }
}
MK_FREELIST(struct _qdbp_boxed_int *, box_freelist)
struct _qdbp_boxed_int *_qdbp_malloc_boxed_int() {
  if (_QDBP_BOX_FREELIST && box_freelist.idx > 0) {
    return _qdbp_pop_box_freelist();
  } else {
    return _qdbp_malloc(sizeof(struct _qdbp_boxed_int), "_qdbp_box");
  }
}
void _qdbp_free_boxed_int(struct _qdbp_boxed_int *i) {
  if (_QDBP_BOX_FREELIST && _qdbp_push_box_freelist(i)) {
  } else {
    _qdbp_free(i);
  }
}
void _qdbp_init() {
  for (int i = 0; i < _QDBP_FREELIST_SIZE; i++) {
    if (!_QDBP_CHECK_MALLOC_FREE) {
      _qdbp_push_freelist(
          _qdbp_malloc(sizeof(struct _qdbp_object), "obj _qdbp_init"));
    }
  }
}
void _qdbp_memcpy(void *dest, const void *src, size_t size) {
  memcpy(dest, src, size);
}
void _qdbp_check_mem() {
  // Make sure that everything that has been malloc'd has been freed
  if (_QDBP_CHECK_MALLOC_FREE) {
    if (_QDBP_OBJ_FREELIST) {
      _qdbp_freelist_remove_all_from_malloc_list();
    }
    if (_QDBP_BOX_FREELIST) {
      _qdbp_box_freelist_remove_all_from_malloc_list();
    }
    struct malloc_list_node *node = malloc_list;
    while (node) {
      printf("Error: %p(%s) was malloc'd but not freed\n", node->ptr,
             node->msg);
      node = node->next;
    }
  }
}
void _qdbp_del_prototype(_qdbp_prototype_ptr proto) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(proto);
  }
  _qdbp_field_ptr *PValue;

  _qdbp_free_fields(proto);
}

void _qdbp_del_variant(_qdbp_variant_ptr variant) {
  assert(variant);
  _qdbp_drop(variant->value, 1);
}

_qdbp_object_arr _qdbp_make_capture_arr(size_t size) {
  _qdbp_object_arr capture;
  return (_qdbp_object_ptr *)_qdbp_malloc(sizeof(_qdbp_object_ptr) * size,
                                          "captures");
}
void _qdbp_free_capture_arr(_qdbp_object_arr arr, size_t size) {
  if (arr) {
    _qdbp_free(arr);
  }
}
void _qdbp_del_method(_qdbp_method_ptr method) {
  assert(method);
  for (size_t i = 0; i < method->captures_size; i++) {
    _qdbp_drop((method->captures[i]), 1);
  }
  if (method->captures_size > 0) {
    _qdbp_free_capture_arr(method->captures, method->captures_size);
  }
}

void _qdbp_del_obj(_qdbp_object_ptr obj) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(!_qdbp_is_unboxed_int(obj));
  }
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

void _qdbp_duplicate_labels(_qdbp_prototype_ptr src, _qdbp_prototype_ptr dest) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(!dest->labels);
  }
  dest->labels = _qdbp_ht_duplicate(src->labels);
}
