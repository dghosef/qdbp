#include "runtime.h"

// Keep track of mallocs and frees
struct malloc_list_node {
  void *ptr;
  struct malloc_list_node *next;
  const char *msg;
};
static struct malloc_list_node *malloc_list = NULL;

bool malloc_list_contains(void *ptr) {
  if (DYNAMIC_TYPECHECK) {
    assert(!is_unboxed_int((qdbp_object_ptr)ptr));
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
void add_to_malloc_list(void *ptr, const char *msg) {
  if (DYNAMIC_TYPECHECK) {
    assert(!is_unboxed_int((qdbp_object_ptr)ptr));
  }
  assert(!malloc_list_contains(ptr));
  struct malloc_list_node *new_node =
      (struct malloc_list_node *)malloc(sizeof(struct malloc_list_node));
  new_node->ptr = ptr;
  new_node->next = malloc_list;
  new_node->msg = msg;
  malloc_list = new_node;
}
void remove_from_malloc_list(void *ptr) {
  if (DYNAMIC_TYPECHECK) {
    assert(!is_unboxed_int((qdbp_object_ptr)ptr));
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
void *qdbp_malloc(size_t size, const char *msg) {
  if (size == 0) {
    return NULL;
  }
  void *ptr;
  ptr = malloc(size);
  if (CHECK_MALLOC_FREE) {
    add_to_malloc_list(ptr, msg);
  }
  assert(ptr);
  return ptr;
}
void qdbp_free(void *ptr) {
  if (DYNAMIC_TYPECHECK) {
    assert(!is_unboxed_int((qdbp_object_ptr)ptr));
  }
  if (CHECK_MALLOC_FREE) {
    remove_from_malloc_list(ptr);
  }
  free(ptr);
}

#define MK_FREELIST(type, name)                                                \
  struct name##_t {                                                            \
    type objects[FREELIST_SIZE];                                               \
    size_t idx;                                                                \
  };                                                                           \
  struct name##_t name = {.idx = 0, .objects = {0}};                           \
                                                                               \
  static type pop_##name() {                                                   \
    if (name.idx == 0) {                                                       \
      assert(false);                                                           \
    }                                                                          \
    name.idx--;                                                                \
    return name.objects[name.idx];                                             \
  }                                                                            \
  static bool push_##name(type obj) {                                          \
    if (name.idx == FREELIST_SIZE) {                                           \
      return false;                                                            \
    }                                                                          \
    name.objects[name.idx] = obj;                                              \
    name.idx++;                                                                \
    return true;                                                               \
  }                                                                            \
  static void name##_remove_all_from_malloc_list() {                           \
    for (size_t i = 0; i < name.idx; i++) {                                    \
      remove_from_malloc_list(name.objects[i]);                                \
    }                                                                          \
  }
MK_FREELIST(qdbp_field_ptr, field_freelist)

void qdbp_free_field(qdbp_field_ptr field) {
  del_method(&field->method);
  if (FIELD_FREELIST && push_field_freelist(field)) {

  } else {
    qdbp_free((void *)field);
  }
}
qdbp_field_ptr qdbp_malloc_field() {
  if (FIELD_FREELIST && field_freelist.idx > 0) {
    return pop_field_freelist();
  } else {
    return (qdbp_field_ptr)qdbp_malloc(sizeof(struct qdbp_field), "field");
  }
}
void free_fields(qdbp_prototype_ptr proto) {
  struct qdbp_field *cur_field;
  struct qdbp_field *tmp;

  HASH_ITER(hh, proto->labels, cur_field, tmp) {
    HASH_DEL(proto->labels, cur_field);
    qdbp_free_field(cur_field);
  }
}

MK_FREELIST(qdbp_object_ptr, freelist)

void qdbp_free_obj(qdbp_object_ptr obj) {
  if (DYNAMIC_TYPECHECK) {
    assert(!is_unboxed_int(obj));
  }
  if (OBJ_FREELIST && push_freelist(obj)) {

  } else {
    qdbp_free((void *)obj);
  }
}

qdbp_object_ptr qdbp_malloc_obj() {
  if (OBJ_FREELIST && freelist.idx > 0) {
    return pop_freelist();
  } else {
    return (qdbp_object_ptr)qdbp_malloc(sizeof(struct qdbp_object), "field");
  }
}
MK_FREELIST(struct boxed_int *, box_freelist)
struct boxed_int *qdbp_malloc_boxed_int() {
  if (BOX_FREELIST && box_freelist.idx > 0) {
    return pop_box_freelist();
  } else {
    return qdbp_malloc(sizeof(struct boxed_int), "box");
  }
}
void qdbp_free_boxed_int(struct boxed_int *i) {
  if (BOX_FREELIST && push_box_freelist(i)) {

  } else {
    qdbp_free(i);
  }
}
void init() {
  for (int i = 0; i < FREELIST_SIZE; i++) {
    if (!CHECK_MALLOC_FREE) {
      push_freelist(qdbp_malloc(sizeof(struct qdbp_object), "obj init"));
      push_field_freelist(qdbp_malloc(sizeof(struct qdbp_field), "field init"));
    }
  }
}
void qdbp_memcpy(void *dest, const void *src, size_t size) {
  memcpy(dest, src, size);
}
void check_mem() {
  // Make sure that everything that has been malloc'd has been freed
  if (CHECK_MALLOC_FREE) {
    if (FIELD_FREELIST) {
      field_freelist_remove_all_from_malloc_list();
    }
    if (OBJ_FREELIST) {
      freelist_remove_all_from_malloc_list();
    }
    if (BOX_FREELIST) {
      box_freelist_remove_all_from_malloc_list();
    }
    struct malloc_list_node *node = malloc_list;
    while (node) {
      printf("Error: %p(%s) was malloc'd but not freed\n", node->ptr,
             node->msg);
      node = node->next;
    }
  }
}
void del_prototype(qdbp_prototype_ptr proto) {
  if (DYNAMIC_TYPECHECK) {
    assert(proto);
  }
  Word_t label = 0;
  qdbp_field_ptr *PValue;

  free_fields(proto);
}

void del_variant(qdbp_variant_ptr variant) {
  assert(variant);
  drop(variant->value, 1);
}

qdbp_object_arr make_capture_arr(size_t size) {
  qdbp_object_arr capture;
  return (qdbp_object_ptr *)qdbp_malloc(sizeof(qdbp_object_ptr) * size,
                                        "captures");
}
void free_capture_arr(qdbp_object_arr arr, size_t size) {
  if (arr) {
    qdbp_free(arr);
  }
}
void del_method(qdbp_method_ptr method) {
  assert(method);
  for (size_t i = 0; i < method->captures_size; i++) {
    drop((method->captures[i]), 1);
  }
  if (method->captures_size > 0) {
    free_capture_arr(method->captures, method->captures_size);
  }
}

void del_obj(qdbp_object_ptr obj) {

  if (DYNAMIC_TYPECHECK) {
    assert(!is_unboxed_int(obj));
  }
  switch (get_kind(obj)) {
  case QDBP_INT:
    break;
  case QDBP_FLOAT:
    break;
  case QDBP_BOXED_INT:
    del_prototype(&(obj->data.boxed_int->other_labels));
    qdbp_free(obj->data.boxed_int);
    break;
  case QDBP_STRING:
    qdbp_free(obj->data.s);
    break;
  case QDBP_PROTOTYPE:
    del_prototype(&(obj->data.prototype));
    break;
  case QDBP_VARIANT:
    del_variant(&(obj->data.variant));
    break;
  }
  qdbp_free_obj(obj);
}

void duplicate_labels(qdbp_prototype_ptr src, qdbp_prototype_ptr dest) {
  if(DYNAMIC_TYPECHECK) {
    assert(!dest->labels);
  }
  qdbp_field_ptr cur_field;
  qdbp_field_ptr tmp;
  HASH_ITER(hh, src->labels, cur_field, tmp) {
    qdbp_field_ptr new_field = qdbp_malloc_field();
    new_field->label = cur_field->label;
    new_field->method = cur_field->method;
    HASH_ADD(hh, dest->labels, label, sizeof(Word_t), new_field);
  }
}
