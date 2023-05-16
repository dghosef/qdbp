// This file gets included first in every qdbp progrram
// FIXME: Have record literals include label names

#ifndef QDBP_RUNTIME_H
#define QDBP_RUNTIME_H
#include "bump.h"
#include "mempool.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// config
// You could also add fsanitize=undefined
// Features
// Not all are mutually exclusive
static const size_t PROTO_HEADER_THRESHOLD = 2;
static const bool REFCOUNT = true;
static const bool BUMP_ALLOCATOR = true;
static const bool BINARY_SEARCH_ENABLED = true;
static const size_t BINARY_SEARCH_THRESHOLD = 30;
static const bool REUSE_PROTO_REPLACE = true;
static const bool OBJ_FREELIST = true;
static const bool FIELD_FREELIST = true;
static const bool CAPTURE_FREELIST = true;
static const bool PROTO_HEADER_FREELIST = true;
#define FREELIST_SIZE 100

// Change the # of times `action` is called for a different # of freelists
// IMPORTANT: Keep NUM_FREELISTS and FORALL_FREELISTS synced
static const size_t NUM_FREELISTS = 20;
#define FORALL_FREELISTS(action)                                               \
  action(1) action(2) action(3) action(4) action(5) action(6) action(7)        \
      action(8) action(9) action(10) action(11) action(12) action(13)          \
          action(14) action(15) action(16) action(17) action(18) action(19)    \
              action(20)

// Dynamic checks
static const bool CHECK_MALLOC_FREE = false; // very slow
static const bool VERIFY_REFCOUNTS = false;
static const bool DYNAMIC_TYPECHECK = false;
static const bool CHECK_DUPLICATE_LABELS = false;
static const bool CHECK_PROTOTYPE_ORDERED = false;

/*
    ====================================================
    Types
    ====================================================
*/
// FIXME: All identifiers should start with __qdbp
// FIXME: Clean all the adding to the list up
// FIXME: Wrap all object accesses in, for example, get_prototype that does
// assertion
// Have make string, make int, etc fns
// FIXME: Check all accesses are asserted
typedef uint64_t label_t;
typedef uint64_t tag_t;
typedef int64_t refcount_t;
struct qdbp_object;
struct qdbp_method {
  struct qdbp_object **captures;
  size_t captures_size;
  void *code; // We don't know how many args so stay as a void* for now
};
struct qdbp_field {
  label_t label;
  struct qdbp_method method;
};

struct qdbp_prototype_header {
  refcount_t rc;
  size_t labels[LABEL_CNT];
};
struct qdbp_prototype {
  size_t size;
  struct qdbp_prototype_header *header;
  struct qdbp_field *fields;
};

struct qdbp_variant {
  tag_t tag;
  struct qdbp_object *value;
};

union qdbp_object_data {
  struct qdbp_prototype prototype;
  int64_t i;
  double f;
  char *s;
  struct qdbp_variant variant;
};

enum qdbp_object_kind {
  QDBP_INT,
  QDBP_FLOAT,
  QDBP_STRING,
  QDBP_PROTOTYPE,
  QDBP_VARIANT
};

struct qdbp_object {
  refcount_t refcount;
  enum qdbp_object_kind kind;
  union qdbp_object_data data;
};
typedef struct qdbp_object *qdbp_object_ptr;
typedef struct qdbp_object **qdbp_object_arr;
typedef struct qdbp_variant *qdbp_variant_ptr;
typedef struct qdbp_prototype *qdbp_prototype_ptr;
typedef struct qdbp_method *qdbp_method_ptr;
typedef struct qdbp_field *qdbp_field_ptr;
typedef struct qdbp_prototype_header *qdbp_proto_header_ptr;
/*
====================================================
Basic Utilities
====================================================
*/
void print_object(qdbp_object_ptr obj) {
  switch (obj->kind) {
  case QDBP_INT:
    printf("int %lld\n", obj->data.i);
    break;
  case QDBP_FLOAT:
    printf("float %f\n", obj->data.f);
    break;
  case QDBP_STRING:
    printf("str %s\n", obj->data.s);
    break;
  case QDBP_PROTOTYPE:
    for (size_t i = 0; i < obj->data.prototype.size; i++) {
      printf("field %lld\n", obj->data.prototype.fields[i].label);
    }
    break;
  case QDBP_VARIANT:
    printf("variant %lld\n", obj->data.variant.tag);
    break;
  }
}
#define assert_refcount(obj)                                                   \
  do {                                                                         \
    if (VERIFY_REFCOUNTS) {                                                    \
      assert((obj));                                                           \
      if ((obj)->refcount <= 0) {                                              \
        printf("refcount of %lld\n", (obj)->refcount);                         \
        print_object(obj);                                                     \
        assert(false);                                                         \
      };                                                                       \
      if ((obj)->kind == QDBP_PROTOTYPE &&                                     \
          (obj)->data.prototype.size >= PROTO_HEADER_THRESHOLD &&              \
          (obj)->data.prototype.header->rc <= 0) {                             \
        printf("refcount of %lld\n", ((obj)->data.prototype.header->rc));      \
        print_object(obj);                                                     \
        assert(false);                                                         \
      };                                                                       \
    }                                                                          \
  } while (0);
#define assert_obj_kind(obj, k)                                                \
  do {                                                                         \
    assert_refcount(obj);                                                      \
    if (DYNAMIC_TYPECHECK) {                                                   \
      assert((obj)->kind == k);                                                \
    }                                                                          \
  } while (0);
void qdbp_memcpy(void *dest, const void *src, size_t size) {
  memcpy(dest, src, size);
}
// Keep track of mallocs and frees
struct malloc_list_node {
  void *ptr;
  struct malloc_list_node *next;
};
struct malloc_list_node *malloc_list = NULL;
bool malloc_list_contains(void *ptr) {
  struct malloc_list_node *node = malloc_list;
  while (node) {
    if (node->ptr == ptr) {
      return true;
    }
    node = node->next;
  }
  return false;
}
void add_to_malloc_list(void *ptr) {
  assert(!malloc_list_contains(ptr));
  struct malloc_list_node *new_node =
      (struct malloc_list_node *)malloc(sizeof(struct malloc_list_node));
  new_node->ptr = ptr;
  new_node->next = malloc_list;
  malloc_list = new_node;
}
void remove_from_malloc_list(void *ptr) {
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
void *qdbp_malloc(size_t size) {
  if (size == 0) {
    return NULL;
  }
  void *ptr;
  if (BUMP_ALLOCATOR) {
    ptr = bump_alloc(size);
  } else {
    ptr = malloc(size);
  }
  if (CHECK_MALLOC_FREE) {
    add_to_malloc_list(ptr);
  }
  assert(ptr);
  return ptr;
}
void qdbp_free(void *ptr) {
  if (CHECK_MALLOC_FREE) {
    remove_from_malloc_list(ptr);
  }
  if (!BUMP_ALLOCATOR) {
    free(ptr);
  }
}

#define MK_FREELIST(type, name)                                                \
  struct name##_t {                                                            \
    type objects[FREELIST_SIZE];                                               \
    size_t idx;                                                                \
  };                                                                           \
  struct name##_t name = {.idx = 0, .objects = {0}};                           \
                                                                               \
  type pop_##name() {                                                          \
    if (name.idx == 0) {                                                       \
      assert(false);                                                           \
    }                                                                          \
    name.idx--;                                                                \
    return name.objects[name.idx];                                             \
  }                                                                            \
  bool push_##name(type obj) {                                                 \
    if (name.idx == FREELIST_SIZE) {                                           \
      return false;                                                            \
    }                                                                          \
    name.objects[name.idx] = obj;                                              \
    name.idx++;                                                                \
    return true;                                                               \
  }                                                                            \
  void name##_remove_all_from_malloc_list() {                                  \
    for (size_t i = 0; i < name.idx; i++) {                                    \
      remove_from_malloc_list(name.objects[i]);                                \
    }                                                                          \
  }
MK_FREELIST(qdbp_proto_header_ptr, proto_header_freelist)
qdbp_proto_header_ptr mk_proto_header() {
  qdbp_proto_header_ptr ptr;
  if (PROTO_HEADER_FREELIST && proto_header_freelist.idx > 0) {
    ptr = pop_proto_header_freelist();
  } else {
    ptr = qdbp_malloc(sizeof(struct qdbp_prototype_header));
  }
  ptr->rc = 1;
  for (size_t i = 0; i < LABEL_CNT; i++) {
    ptr->labels[i] = LABEL_CNT + 1;
  }
  return ptr;
}
MK_FREELIST(qdbp_object_ptr, freelist)

void qdbp_free_obj(qdbp_object_ptr obj) {
  if (OBJ_FREELIST && push_freelist(obj)) {

  } else {
    qdbp_free((void *)obj);
  }
}
mp_pool_t obj_pool;

qdbp_object_ptr qdbp_malloc_obj() {
  if (OBJ_FREELIST && freelist.idx > 0) {
    return pop_freelist();
  } else {
    return (qdbp_object_ptr)qdbp_malloc(sizeof(struct qdbp_object));
  }
}

// Keep track of objects for testing
struct object_list_node {
  qdbp_object_ptr obj;
  struct object_list_node *next;
};
struct object_list_node *object_list = NULL;
qdbp_object_ptr make_object(enum qdbp_object_kind kind,
                            union qdbp_object_data data) {
  qdbp_object_ptr new_obj = qdbp_malloc_obj();
  // add new_obj to object_list. Only if freeing is off otherwise
  // object might be freed before we get to it
  if (BUMP_ALLOCATOR && VERIFY_REFCOUNTS) {
    struct object_list_node *new_obj_list =
        (struct object_list_node *)malloc(sizeof(struct object_list_node));
    new_obj_list->obj = new_obj;
    new_obj_list->next = object_list;
    object_list = new_obj_list;
  }
  new_obj->refcount = 1;
  new_obj->kind = kind;
  new_obj->data = data;
  return new_obj;
}

qdbp_object_ptr empty_prototype() {
  qdbp_object_ptr obj = make_object(
      QDBP_PROTOTYPE, (union qdbp_object_data){
                          .prototype = {.size = 0,
                                        .fields = NULL,
                                        .header = PROTO_HEADER_THRESHOLD <= 0
                                                      ? mk_proto_header()
                                                      : NULL}});
  return obj;
}
qdbp_object_ptr qdbp_true() {
  return make_object(QDBP_VARIANT,
                     (union qdbp_object_data){
                         .variant = {.tag = 1, .value = empty_prototype()}});
}
qdbp_object_ptr qdbp_false() {
  return make_object(QDBP_VARIANT,
                     (union qdbp_object_data){
                         .variant = {.tag = 0, .value = empty_prototype()}});
}
/*
    ====================================================
    Reference Counting Functions
    ====================================================
*/
bool is_unique(qdbp_object_ptr obj) {
  assert(obj);
  assert(obj->refcount >= 0);
  return REFCOUNT && obj->refcount == 1;
}
void decref(qdbp_object_ptr obj) {
  assert_refcount(obj);
  obj->refcount--;
}

void del_method(qdbp_method_ptr method);
void del_prototype(qdbp_prototype_ptr proto);
void del_variant(qdbp_variant_ptr variant);
void del_obj(qdbp_object_ptr obj);
bool drop(qdbp_object_ptr obj, refcount_t cnt);

// FIXME: Remove code duplication

#define MAKE_CASES FORALL_FREELISTS(MAKE_CASE)

#define MK_FIELD_FREELIST(n) MK_FREELIST(qdbp_field_ptr, field_freelist##n)
FORALL_FREELISTS(MK_FIELD_FREELIST)

#define MK_CAPTURE_FREELIST(n) MK_FREELIST(qdbp_object_arr, capture_freelist##n)
FORALL_FREELISTS(MK_CAPTURE_FREELIST)

bool pop_field_freelist(size_t size, qdbp_field_ptr *field) {

#define MAKE_CASE(n)                                                           \
  case n:                                                                      \
    if (field_freelist##n.idx > 0) {                                           \
      *field = pop_field_freelist##n();                                        \
      return true;                                                             \
    }                                                                          \
    break;

  switch (size) {
    MAKE_CASES
#undef MAKE_CASE
  default:
    return false;
    break;
  }
  return false;
}

bool push_field_freelist(size_t size, qdbp_field_ptr field) {
  if (!field) {
    return false;
  }
#define MAKE_CASE(n)                                                           \
  case n:                                                                      \
    return push_field_freelist##n(field);                                      \
    break;
  switch (size) {
    MAKE_CASES
#undef MAKE_CASE
  default:
    return false;
    break;
  }
}

qdbp_field_ptr make_field_arr(size_t size) {
  qdbp_field_ptr field;
  if (FIELD_FREELIST && pop_field_freelist(size, &field)) {
    return field;
  } else {
    return (qdbp_field_ptr)qdbp_malloc(sizeof(struct qdbp_field) * size);
  }
}
void free_field_arr(qdbp_field_ptr arr, size_t size) {
  if (arr) {
    if (FIELD_FREELIST && push_field_freelist(size, arr)) {
      return;
    } else {
      qdbp_free(arr);
    }
  }
}

bool pop_capture_freelist(size_t size, qdbp_object_arr *capture) {
#define MAKE_CASE(n)                                                           \
  case n:                                                                      \
    if (capture_freelist##n.idx > 0) {                                         \
      *capture = pop_capture_freelist##n();                                    \
      return true;                                                             \
    }                                                                          \
    break;
  switch (size) {
    MAKE_CASES
#undef MAKE_CASE
  default:
    return false;
    break;
  }
  return false;
}

bool push_capture_freelist(size_t size, qdbp_object_arr capture) {
  if (!capture) {
    return false;
  }
#define MAKE_CASE(n)                                                           \
  case n:                                                                      \
    return push_capture_freelist##n(capture);                                  \
    break;
  switch (size) {
    MAKE_CASES
#undef MAKE_CASE
  default:
    return false;
    break;
  }
}
void del_proto_header(qdbp_proto_header_ptr header) {
  if (PROTO_HEADER_FREELIST && !push_proto_header_freelist(header)) {
    qdbp_free(header);
  }
}
void drop_proto_header(const qdbp_prototype_ptr proto) {
  if (proto->size >= PROTO_HEADER_THRESHOLD) {
    if (VERIFY_REFCOUNTS) {
      assert(proto->header->rc > 0);
    }
    proto->header->rc--;
    if (proto->header->rc == 0) {
      del_proto_header(proto->header);
    }
  }
}
void del_prototype(qdbp_prototype_ptr proto) {
  assert(proto);
  for (size_t i = 0; i < proto->size; i++) {
    assert(proto->fields);
    del_method(&(proto->fields[i].method));
  }
  if (proto->size > 0) {
    free_field_arr(proto->fields, proto->size);
  }
  drop_proto_header(proto);
}

void del_variant(qdbp_variant_ptr variant) {
  assert(variant);
  drop(variant->value, 1);
}

qdbp_object_arr make_capture_arr(size_t size) {
  qdbp_object_arr capture;
  if (CAPTURE_FREELIST && pop_capture_freelist(size, &capture)) {
    return capture;
  } else {
    return (qdbp_object_ptr *)qdbp_malloc(sizeof(qdbp_object_ptr) * size);
  }
}
void free_capture_arr(qdbp_object_arr arr, size_t size) {
  if (arr) {
    if (CAPTURE_FREELIST && push_capture_freelist(size, arr)) {
      return;
    } else {
      qdbp_free(arr);
    }
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

  switch (obj->kind) {
  case QDBP_INT:
    break;
  case QDBP_FLOAT:
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

bool drop(qdbp_object_ptr obj, refcount_t cnt) {
  assert_refcount(obj);
  if (VERIFY_REFCOUNTS) {
    assert(obj->refcount >= cnt);
  }
  if (REFCOUNT) {
    obj->refcount -= cnt;
    if (obj->refcount == 0) {
      del_obj(obj);
      return true;
    }
    return false;
  } else {
    return false;
  }
}

void obj_dup(qdbp_object_ptr obj, refcount_t cnt) {
  assert_refcount(obj);
  if (REFCOUNT) {
    obj->refcount += cnt;
  }
}

qdbp_proto_header_ptr dup_header(const qdbp_prototype_ptr proto) {
  if (proto->size >= PROTO_HEADER_THRESHOLD) {
    if (VERIFY_REFCOUNTS) {
      assert(proto->header->rc > 0);
    }
    proto->header->rc++;
    return proto->header;
  } else {
    assert(proto->header == NULL);
    return NULL;
  }
}
void dup_captures(qdbp_method_ptr method) {
  for (size_t i = 0; i < method->captures_size; i++) {
    obj_dup((method->captures[i]), 1);
  }
}

void dup_prototype_captures(qdbp_prototype_ptr proto) {
  for (size_t i = 0; i < proto->size; i++) {
    dup_captures(&(proto->fields[i].method));
  }
}
void dup_prototype_captures_except(qdbp_prototype_ptr proto, label_t except) {
  for (size_t i = 0; i < proto->size; i++) {
    if (proto->fields[i].label != except) {
      dup_captures(&(proto->fields[i].method));
    }
  }
}
/*
    ====================================================
    Prototype Utilities
    ====================================================
*/

size_t linear_prototype_get(const qdbp_prototype_ptr proto, label_t label) {
  for (size_t i = 0; i < proto->size; i++) {
    if (proto->fields[i].label == label) {
      return i;
    }
  }
  printf("Label %llu not found in prototype\n", label);
  for (size_t i = 0; i < proto->size; i++) {
    printf("Label %llu found\n", proto->fields[i].label);
  }
  assert(false);
  __builtin_unreachable();
}
size_t binary_prototype_get(const qdbp_prototype_ptr proto, label_t label) {
  if (CHECK_PROTOTYPE_ORDERED) {
    for (size_t i = 1; i < proto->size; i++) {
      assert(proto->fields[i].label > proto->fields[i - 1].label);
    }
  }
  size_t low = 0;
  size_t high = proto->size - 1;
  while (low <= high) {
    size_t mid = (low + high) / 2;
    if (proto->fields[mid].label < label) {
      low = mid + 1;
    } else if (proto->fields[mid].label > label) {
      high = mid - 1;
    } else {
      return mid;
    }
  }
  printf("Label %llu not found in prototype\n", label);
  for (size_t i = 0; i < proto->size; i++) {
    printf("Label %llu found\n", proto->fields[i].label);
  }
  assert(false);
  __builtin_unreachable();
}
size_t prototype_get(const qdbp_prototype_ptr proto, label_t label) {
  if (DYNAMIC_TYPECHECK) {
    assert(label < LABEL_CNT);
  }
  if (proto->size == 1) {
    return 0;
  } else if (proto->size < PROTO_HEADER_THRESHOLD) {
    return linear_prototype_get(proto, label);
  } else {
    size_t result = proto->header->labels[label];
    if (DYNAMIC_TYPECHECK && result > LABEL_CNT + 1) {
      assert(false);
    }
    return result;
  }
}

void copy_captures_except(qdbp_prototype_ptr new_prototype, label_t except) {
  // Copy all the capture arrays except the new one
  for (size_t i = 0; i < new_prototype->size; i++) {
    if (new_prototype->fields[i].label != except) {
      qdbp_object_arr original = new_prototype->fields[i].method.captures;
      new_prototype->fields[i].method.captures =
          make_capture_arr(new_prototype->fields[i].method.captures_size);
      qdbp_memcpy(new_prototype->fields[i].method.captures, original,
                  sizeof(qdbp_object_ptr) *
                      new_prototype->fields[i].method.captures_size);
    }
  }
}

struct qdbp_prototype raw_prototype_replace(const qdbp_prototype_ptr src,
                                            const qdbp_field_ptr new_field) {
  assert(src->size > 0);
  // copy src
  qdbp_field_ptr new_fields = make_field_arr(src->size);
  qdbp_memcpy(new_fields, src->fields, sizeof(struct qdbp_field) * src->size);
  struct qdbp_prototype new_prototype = {
      .size = src->size, .fields = new_fields, .header = dup_header(src)};
  // overwrite the field
  new_prototype.fields[prototype_get(&new_prototype, new_field->label)] =
      *new_field;
  copy_captures_except(&new_prototype, new_field->label);
  return new_prototype;
}

struct qdbp_prototype raw_prototype_extend(const qdbp_prototype_ptr src,
                                           const qdbp_field_ptr new_field) {
  // copy src
  if (DYNAMIC_TYPECHECK) {
    assert(src->size <= LABEL_CNT);
  }
  qdbp_field_ptr new_fields = make_field_arr((src->size + 1));
  qdbp_proto_header_ptr header;
  if (src->size + 1 < PROTO_HEADER_THRESHOLD) {
    header = NULL;
  } else if (src->size + 1 == PROTO_HEADER_THRESHOLD) {
    header = mk_proto_header();
    for (size_t i = 0; i < src->size; i++) {
      header->labels[src->fields[i].label] = i;
    }
    header->labels[new_field->label] = src->size;
  } else {
    if (src->header->labels[new_field->label] == src->size) {
      header = dup_header(src);

    } else if (src->header->labels[new_field->label] > LABEL_CNT) {
      header = dup_header(src);
      header->labels[new_field->label] = src->size;
    } else {
      header = mk_proto_header();
      for (size_t i = 0; i < src->size; i++) {
        header->labels[src->fields[i].label] = i;
      }
      header->labels[new_field->label] = src->size;
    }
  }
  struct qdbp_prototype new_prototype = {
      .size = src->size + 1, .fields = new_fields, .header = header};
  // Add the new field. Make sure it is in order!!
  if (CHECK_DUPLICATE_LABELS) {
    for (size_t i = 0; i < src->size; i++) {
      assert(src->fields[i].label != new_field->label);
    }
  }
  size_t src_pos = 0;
  size_t dest_pos = 0;
  qdbp_memcpy(new_fields, src->fields, src->size * sizeof(struct qdbp_field));
  new_fields[src->size] = *new_field;
  // Copy all the capture arrays
  copy_captures_except(&new_prototype, new_field->label);
  return new_prototype;
}

/*
    ==========================================================
    Object Utilities. See Fig 7 of the perceus refcount paper
    ==========================================================
    The basic theory is that every function "owns" each of its args
    When the function returns, either the arg is returned(or an object that
   points to it is returned) or the arg is dropped.
*/

/* Prototype Invoke
`invoke prototype label args`
  - lookup the required method
  - dup the captures
  - get the code ptr
  - drop the prototype
  - call the code ptr
*/
qdbp_object_arr get_method(qdbp_object_ptr obj, label_t label,
                           void **code_ptr /*output param*/) {
  assert_obj_kind(obj, QDBP_PROTOTYPE);
  struct qdbp_prototype prototype = (obj->data.prototype);
  size_t idx = prototype_get(&prototype, label);
  struct qdbp_method m = prototype.fields[idx].method;
  dup_captures(&m);
  *code_ptr = m.code;
  qdbp_object_arr ret = m.captures;
  return ret;
}

/* Prototype Extension
`extend prototype label method`
  - Do the prototype extension
  - Set the refcount to 1
  - Dup all the captures
  - Drop the old prototype
*/
qdbp_object_arr make_captures(qdbp_object_arr captures, size_t size) {
  if (size == 0) {
    return NULL;
  } else {
    qdbp_object_arr out =
        (qdbp_object_arr)qdbp_malloc(sizeof(struct qdbp_object *) * size);
    qdbp_memcpy(out, captures, sizeof(struct qdbp_object *) * size);
    return out;
  }
}

qdbp_object_ptr extend(qdbp_object_ptr obj, label_t label, void *code,
                       qdbp_object_arr captures, size_t captures_size) {
  struct qdbp_field f = {
      .label = label,
      .method = {.captures = make_captures(captures, captures_size),
                 .captures_size = captures_size,
                 .code = code}};
  assert_obj_kind(obj, QDBP_PROTOTYPE);
  qdbp_prototype_ptr prototype = &(obj->data.prototype);
  struct qdbp_prototype new_prototype = raw_prototype_extend(prototype, &f);
  qdbp_object_ptr new_obj = make_object(
      QDBP_PROTOTYPE, (union qdbp_object_data){.prototype = new_prototype});
  dup_prototype_captures(prototype);
  drop(obj, 1);
  return new_obj;
}
/* Prototype Replacement
Identical to above except we don't dup captures of new method
`replace prototype label method`
  - Duplicate and replace the prototype
  - dup all the shared captures between the old and new prototype
  - drop prototype
*/
qdbp_object_ptr replace(qdbp_object_ptr obj, label_t label, void *code,
                        qdbp_object_arr captures, size_t captures_size) {
  assert_obj_kind(obj, QDBP_PROTOTYPE);
  struct qdbp_field f = {
      .label = label,
      .method = {.captures = make_captures(captures, captures_size),
                 .captures_size = captures_size,
                 .code = code}};
  if (!REUSE_PROTO_REPLACE || !is_unique(obj)) {
    struct qdbp_prototype new_prototype =
        raw_prototype_replace(&(obj->data.prototype), &f);
    dup_prototype_captures_except(&(obj->data.prototype), f.label);

    qdbp_object_ptr new_obj = make_object(
        QDBP_PROTOTYPE, (union qdbp_object_data){.prototype = new_prototype});

    drop(obj, 1);
    return new_obj;
  } else {
    // find the index to replace
    size_t i = prototype_get(&(obj->data.prototype), label);
    // reuse the field
    del_method(&(obj->data.prototype.fields[i].method));
    obj->data.prototype.fields[i] = f;
    return obj;
  }
}
/* Variant Creation
`variant label value`
  - create a new variant w/ refcount 1 in the heap
*/
qdbp_object_ptr variant_create(tag_t tag, qdbp_object_ptr value) {
  assert_refcount(value);
  qdbp_object_ptr new_obj = make_object(
      QDBP_VARIANT,
      (union qdbp_object_data){.variant = {.tag = tag, .value = value}});
  return new_obj;
}
/* Variant Pattern Matching
  - dup the variant payload
  - drop the variant
  - execute the variant stuff
*/
void decompose_variant(qdbp_object_ptr obj, tag_t *tag,
                       qdbp_object_ptr *payload) {
  assert_obj_kind(obj, QDBP_VARIANT);
  qdbp_object_ptr value = obj->data.variant.value;
  *tag = obj->data.variant.tag;
  // return value, tag
  *payload = value;
}

// Macros/Fns for the various kinds of expressions
#define DROP(v, cnt, expr) (drop(v, cnt), (expr))
#define DUP(v, cnt, expr) (obj_dup(v, cnt), (expr))
#define LET(lhs, rhs, in)                                                      \
  ((lhs = (rhs)), (in)) // assume lhs has been declared already
#define MATCH(tag1, tag2, arg, ifmatch, ifnomatch)                             \
  ((tag1) == (tag2) ? (LET(arg, payload, ifmatch)) : (ifnomatch))
qdbp_object_ptr match_failed() {
  assert(false);
  __builtin_unreachable();
}

qdbp_object_ptr qdbp_int(int64_t i) {
  qdbp_object_ptr new_obj =
      make_object(QDBP_INT, (union qdbp_object_data){.i = i});
  return new_obj;
}

qdbp_object_ptr qdbp_string(const char *src) {
  char *dest = (char *)qdbp_malloc(strlen(src) + 1);
  strcpy(dest, src);
  qdbp_object_ptr new_obj =
      make_object(QDBP_STRING, (union qdbp_object_data){.s = dest});
  return new_obj;
}

qdbp_object_ptr qdbp_float(double f) {
  qdbp_object_ptr new_obj =
      make_object(QDBP_FLOAT, (union qdbp_object_data){.f = f});
  return new_obj;
}
void check_mem() {
  // We can't do this if freeing is on cuz the objects might already be freed
  if (BUMP_ALLOCATOR && VERIFY_REFCOUNTS) {
    struct object_list_node *node = object_list;
    while (node) {
      refcount_t refcount = node->obj->refcount;
      if (refcount > 0) {
        printf("Error: refcount of %p is %lld\n", (void *)node->obj, refcount);
      }
      node = node->next;
    }
  } else {
    assert(!object_list);
  }
  // Make sure that everything that has been malloc'd has been freed
  if (CHECK_MALLOC_FREE) {
    if (OBJ_FREELIST) {
      freelist_remove_all_from_malloc_list();
    }
    if (FIELD_FREELIST) {

#define RM_FIELD_FROM_MALLOC_LIST(n)                                           \
  field_freelist##n##_remove_all_from_malloc_list();
      FORALL_FREELISTS(RM_FIELD_FROM_MALLOC_LIST)
    }
    if (CAPTURE_FREELIST) {
#define RM_CAPTURE_FROM_MALLOC_LIST(n)                                         \
  capture_freelist##n##_remove_all_from_malloc_list();
      FORALL_FREELISTS(RM_CAPTURE_FROM_MALLOC_LIST)
    }
    if (PROTO_HEADER_FREELIST) {
      proto_header_freelist_remove_all_from_malloc_list();
    }
    struct malloc_list_node *node = malloc_list;
    while (node) {
      printf("Error: %p was malloc'd but not freed\n", node->ptr);
      node = node->next;
    }
  }
}
void init() {
  size_t total_freelist_mem = 0;
  total_freelist_mem += sizeof(struct freelist_t);
  total_freelist_mem += sizeof(struct qdbp_object) * FREELIST_SIZE;
  for (size_t i = 0; i < NUM_FREELISTS; i++) {
    total_freelist_mem += sizeof(struct qdbp_field) * FREELIST_SIZE;
    total_freelist_mem += sizeof(struct field_freelist1_t);
    total_freelist_mem += sizeof(qdbp_object_ptr) * FREELIST_SIZE;
    total_freelist_mem += sizeof(struct capture_freelist1_t);
    total_freelist_mem += sizeof(struct qdbp_prototype_header) * FREELIST_SIZE;
    total_freelist_mem += sizeof(struct proto_header_freelist_t);

  }
  printf("Total freelist mem: %zu\n", total_freelist_mem);

  bump_init();
  for (int i = 0; i < FREELIST_SIZE; i++) {
    if (!CHECK_MALLOC_FREE) {
      push_freelist(qdbp_malloc(sizeof(struct qdbp_object)));

#define PUSH_FREELIST(n)                                                       \
  push_capture_freelist##n(qdbp_malloc(sizeof(qdbp_object_ptr) * n));          \
  push_field_freelist##n(qdbp_malloc(sizeof(struct qdbp_field) * n));
      FORALL_FREELISTS(PUSH_FREELIST)
    }
  }
}
#endif
