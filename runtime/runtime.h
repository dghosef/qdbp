// This file gets included first in every qdbp progrram
// FIXME: Have record literals include label names
#ifndef QDBP_RUNTIME_H
#define QDBP_RUNTIME_H
#ifndef FAST // Has a lot of asserts and the like
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

struct qdbp_prototype {
  size_t size;
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
  const char *s;
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
/*
====================================================
Basic Utilities
====================================================
*/
#define assert_refcount(obj)                                                   \
  do {                                                                         \
    assert((obj));                                                             \
    if ((obj)->refcount <= 0) {                                                \
      printf("refcount of %lld\n", (obj)->refcount);                           \
      assert(false);                                                           \
    };                                                                         \
  } while (0);
#define assert_obj_kind(obj, k)                                                \
  assert_refcount(obj);                                                        \
  assert((obj)->kind == k);
void *qdbp_malloc(size_t size) { return malloc(size); }
void qdbp_memcpy(void *dest, const void *src, size_t size) {
  memcpy(dest, src, size);
}
// Keep track of objects for testing
struct object_list_node {
  qdbp_object_ptr obj;
  struct object_list_node *next;
};
struct object_list_node *object_list = NULL;
void print_refcounts() {
  struct object_list_node *node = object_list;
  while (node) {
    refcount_t refcount = node->obj->refcount;
    if (refcount > 0) {
      printf("Error: refcount of %p is %lld\n", (void *)node->obj, refcount);
    }
    node = node->next;
  }
}
qdbp_object_ptr make_object(enum qdbp_object_kind kind,
                            union qdbp_object_data data) {
  qdbp_object_ptr new_obj =
      (qdbp_object_ptr)qdbp_malloc(sizeof(struct qdbp_object));
  // add new_obj to object_list
  struct object_list_node *new_obj_list =
      (struct object_list_node *)qdbp_malloc(sizeof(struct object_list_node));
  new_obj_list->obj = new_obj;
  new_obj_list->next = object_list;
  object_list = new_obj_list;
  new_obj->refcount = 1;
  new_obj->kind = kind;
  new_obj->data = data;
  return new_obj;
}

qdbp_object_ptr empty_prototype() {
  qdbp_object_ptr obj = make_object(
      QDBP_PROTOTYPE,
      (union qdbp_object_data){.prototype = {.size = 0, .fields = NULL}});
  return obj;
}
/*
    ====================================================
    Reference Counting Functions
    ====================================================
*/

void drop_method(qdbp_method_ptr method);
void drop_prototype(qdbp_prototype_ptr proto);
void drop_variant(qdbp_variant_ptr variant);
void drop(qdbp_object_ptr obj);

void drop_prototype(qdbp_prototype_ptr proto) {
  assert(proto);
  for (size_t i = 0; i < proto->size; i++) {
    assert(proto->fields);
    drop_method(&(proto->fields[i].method));
  }
}

void drop_variant(qdbp_variant_ptr variant) {
  assert(variant);
  drop(variant->value);
}

// Be careful about dropping captures
void drop_method(qdbp_method_ptr method) {
  assert(method);
  for (size_t i = 0; i < method->captures_size; i++) {
    drop((method->captures[i]));
  }
}

void drop(qdbp_object_ptr obj) {
  assert_refcount(obj);
  obj->refcount--;
  if (obj->refcount <= 0) {
    switch (obj->kind) {
    case QDBP_INT:
    case QDBP_FLOAT:
    case QDBP_STRING:
      break;
    case QDBP_PROTOTYPE:
      drop_prototype(&(obj->data.prototype));
      break;
    case QDBP_VARIANT:
      drop_variant(&(obj->data.variant));
      break;
    }
  }
}

void dup(qdbp_object_ptr obj) {
  assert_refcount(obj);
  obj->refcount++;
}

void dup_captures(qdbp_method_ptr method) {
  for (size_t i = 0; i < method->captures_size; i++) {
    dup((method->captures[i]));
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

size_t prototype_get(const qdbp_prototype_ptr proto, label_t label) {
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

struct qdbp_prototype raw_prototype_replace(const qdbp_prototype_ptr src,
                                            const qdbp_field_ptr new_field) {
  assert(src->size > 0);
  // copy src
  qdbp_field_ptr new_fields =
      (qdbp_field_ptr)qdbp_malloc(sizeof(struct qdbp_field) * src->size);
  qdbp_memcpy(new_fields, src->fields, sizeof(struct qdbp_field) * src->size);
  struct qdbp_prototype new_prototype = {.size = src->size,
                                         .fields = new_fields};
  // overwrite the field
  new_prototype.fields[prototype_get(&new_prototype, new_field->label)] =
      *new_field;
  return new_prototype;
}

struct qdbp_prototype raw_prototype_extend(const qdbp_prototype_ptr src,
                                           const qdbp_field_ptr new_field) {
  // copy src
  qdbp_field_ptr new_fields =
      (qdbp_field_ptr)qdbp_malloc(sizeof(struct qdbp_field) * (src->size + 1));
  qdbp_memcpy(new_fields, src->fields, sizeof(struct qdbp_field) * src->size);
  struct qdbp_prototype new_prototype = {.size = src->size + 1,
                                         .fields = new_fields};
  // Add the new field
  new_prototype.fields[src->size] = *new_field;
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
  drop(obj);
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
  drop(obj);
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
  struct qdbp_prototype new_prototype =
      raw_prototype_replace(&(obj->data.prototype), &f);
  dup_prototype_captures_except(&(obj->data.prototype), f.label);

  qdbp_object_ptr new_obj = make_object(
      QDBP_PROTOTYPE, (union qdbp_object_data){.prototype = new_prototype});

  drop(obj);
  return new_obj;
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
  dup(value);
  drop(obj);
  // return value, tag
  *payload = value;
}

// Macros/Fns for the various kinds of expressions
#define DROP(v, expr) (drop(v), (expr))
#define DUP(v, expr) (dup(v), (expr))
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

qdbp_object_ptr qdbp_string(const char *s) {
  qdbp_object_ptr new_obj =
      make_object(QDBP_STRING, (union qdbp_object_data){.s = s});
  return new_obj;
}

qdbp_object_ptr qdbp_float(double f) {
  qdbp_object_ptr new_obj =
      make_object(QDBP_FLOAT, (union qdbp_object_data){.f = f});
  return new_obj;
}
#endif
#endif
