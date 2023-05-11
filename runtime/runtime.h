// This file gets included first in every qdbp progrram
#ifndef QDBP_RUNTIME_H
#define QDBP_RUNTIME_H
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
/*
    ====================================================
    Types
    ====================================================
*/
// FIXME: All identifiers should start with __qdbp
// FIXME: Add more typedefs?
// FIXME: Assert all refcounts are 0 at end
// FIXME: Pass by pointer more
struct qdbp_object;
struct qdbp_method {
  struct qdbp_object *captures;
  uint64_t captures_size;
  void *code; // We don't know how many args so stay as a void* for now
};
struct qdbp_field {
  uint64_t label;
  struct qdbp_method method;
};

struct qdbp_prototype {
  uint64_t size;
  struct qdbp_field *fields;
};

struct qdbp_variant {
  uint64_t label;
  struct qdbp_object *value;
};

union qdbp_object_data {
  int64_t i;
  double f;
  // FIXME: Unicode support
  char *s;
  struct qdbp_prototype prototype;
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
  int64_t refcount;
  enum qdbp_object_kind kind;
  union qdbp_object_data data;
};
typedef struct qdbp_object *qdbp_object_ptr;
typedef struct qdbp_variant *qdbp_variant_ptr;
typedef struct qdbp_prototype *qdbp_prototype_ptr;
typedef struct qdbp_method *qdbp_method_ptr;
typedef struct qdbp_field *qdbp_field_ptr;

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
  for (uint64_t i = 0; i < proto->size; i++) {
    drop_method(&(proto->fields[i].method));
  }
}

void drop_variant(qdbp_variant_ptr variant) { drop(variant->value); }

void drop_method(qdbp_method_ptr method) {
  for (int i = 0; i < method->captures_size; i++) {
    drop(&(method->captures[i]));
  }
}

void drop(qdbp_object_ptr obj) {
  assert(obj->refcount > 0);
  obj->refcount--;
  if (obj->refcount == 0) {
    switch (obj->kind) {
    case QDBP_INT:
    case QDBP_FLOAT:
    case QDBP_STRING:
      break;
    case QDBP_PROTOTYPE:
      drop_prototype(&(obj->data.prototype));
    case QDBP_VARIANT:
      drop_variant(&(obj->data.variant));
    }
  }
}

void dup(qdbp_object_ptr obj) {
  assert(obj->refcount > 0);
  obj->refcount++;
}

void dup_captures(qdbp_method_ptr method) {
  for (int i = 0; i < method->captures_size; i++) {
    dup(&(method->captures[i]));
  }
}

/*
    ====================================================
    Standard Memory Utilities
    ====================================================
*/
void *qdbp_malloc(uint64_t size) { return malloc(size); }
void qdbp_memcpy(void *dest, const void *src, uint64_t size) {
  memcpy(dest, src, size);
}
/*
    ====================================================
    Prototype Utilities
    ====================================================
*/
uint64_t prototype_get(const qdbp_prototype_ptr proto, uint64_t label) {
  for (uint64_t i = 0; i < proto->size; i++) {
    if (proto->fields[i].label == label) {
      return i;
    }
  }
  assert(false);
}

// FIXME: These should just return prototypes not prototype ptrs
struct qdbp_prototype raw_prototype_replace(const qdbp_prototype_ptr src,
                                            const qdbp_field_ptr new_field) {
  // copy src
  qdbp_field_ptr new_fields =
      (qdbp_field_ptr)qdbp_malloc(sizeof(struct qdbp_field) * src->size);
  qdbp_memcpy(new_fields, src->fields, sizeof(struct qdbp_field) * src->size);
  struct qdbp_prototype new_prototype = {src->size, new_fields};
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
  struct qdbp_prototype new_prototype = {src->size + 1, new_fields};
  // Add the new field
  new_prototype.fields[src->size] = *new_field;
  return new_prototype;
}

void dup_prototype_captures(qdbp_prototype_ptr proto) {
  for (uint64_t i = 0; i < proto->size; i++) {
    dup_captures(&(proto->fields[i].method));
  }
}
void dup_prototype_captures_except(qdbp_prototype_ptr proto, uint64_t except) {
  for (uint64_t i = 0; i < proto->size; i++) {
    if (i != except) {
      dup_captures(&(proto->fields[i].method));
    }
  }
}
/*
    ==========================================================
    Object Utilities. See Fig 7 of the perceus refcount paper
    ==========================================================
*/

/* Prototype Invoke
`invoke prototype label args`
  - lookup the required method
  - dup the captures
  - get the code ptr
  - drop the prototype
  - call the code ptr
*/
#define QDBP_INVOKE(object, label, args, method_type)                          \
  (assert((object)->kind == QDBP_PROTOTYPE), assert((object)->refcount > 0),   \
   qdbp_prototype_ptr prototype = &((object)->data.prototype),                 \
   uint64_t method_index = prototype_get(prototype, (label)),                  \
   qdbp_method_ptr method = &(prototype->fields[method_index].method),         \
   dup_captures(method), method_type code_ptr = (method_type)method->code,     \
   drop(object),                                                               \
   code_ptr args QDBP_INVOKE((qdbp_object_ptr)NULL, 0, (), (*)(int)))
/* Prototype Extension
`extend prototype label method`
  - Do the prototype extension
  - Set the refcount to 1
  - Dup all the captures
  - Drop the old prototype
*/
qdbp_object_ptr extend(qdbp_object_ptr obj, const qdbp_field_ptr f) {
  assert(obj->kind == QDBP_PROTOTYPE);
  assert(obj->refcount > 0);
  qdbp_prototype_ptr prototype = &(obj->data.prototype);
  struct qdbp_prototype new_prototype = raw_prototype_extend(prototype, f);
  qdbp_object_ptr new_obj = (qdbp_object_ptr)qdbp_malloc(sizeof(struct qdbp_object));
  new_obj->kind = QDBP_PROTOTYPE;
  new_obj->refcount = 1;
  new_obj->data.prototype = new_prototype;
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
qdbp_object_ptr replace(qdbp_object_ptr obj, const qdbp_field_ptr f) {
  assert(obj->kind == QDBP_PROTOTYPE);
  assert(obj->refcount > 0);
  struct qdbp_prototype new_prototype =
      raw_prototype_replace(&(obj->data.prototype), f);
  dup_prototype_captures_except(&(obj->data.prototype), f->label);
  qdbp_object_ptr new_obj = (qdbp_object_ptr)qdbp_malloc(sizeof(struct qdbp_object));
  new_obj->kind = QDBP_PROTOTYPE;
  new_obj->refcount = 1;
  new_obj->data.prototype = new_prototype;
  drop(obj);
  return new_obj;
}
/* Variant Creation
`variant label value`
  - create a new variant w/ refcount 1 in the heap
*/
qdbp_object_ptr variant_create(uint64_t label, qdbp_object_ptr value) {
  assert(value->refcount > 0);
  qdbp_object_ptr new_obj = (qdbp_object_ptr)qdbp_malloc(sizeof(struct qdbp_object));
  new_obj->kind = QDBP_VARIANT;
  new_obj->refcount = 1;
  new_obj->data.variant.label = label;
  new_obj->data.variant.value = value;
  return new_obj;
}
/* Variant Pattern Matching
  - dup the variant payload
  - drop the variant
  - execute the variant stuff
*/
qdbp_object_ptr decompose_variant(qdbp_object_ptr obj, uint64_t *label) {
  assert(obj->kind == QDBP_VARIANT);
  assert(obj->refcount > 0);
  qdbp_object_ptr value = obj->data.variant.value;
  *label = obj->data.variant.label;
  dup(value);
  drop(obj);
  return value;
}

// Let expression. Assumes that lhs has been declared already
#define LET(lhs, rhs, in) (lhs = (rhs), (in))
#endif
int main() {}