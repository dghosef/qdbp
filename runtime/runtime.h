// This file gets included first in every qdbp progrram

#ifndef QDBP_RUNTIME_H
#define QDBP_RUNTIME_H
#include <Judy.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// config
// You could also add fsanitize=undefined
static const bool REUSE_ANALYSIS = true;
static const bool OBJ_FREELIST = true;
static const bool FIELD_FREELIST = true;
#define FREELIST_SIZE 1000

// Dynamic checks
static const bool CHECK_MALLOC_FREE = true; // very slow
static const bool VERIFY_REFCOUNTS = true;
static const bool DYNAMIC_TYPECHECK = true;
#define SAFE

#ifndef SAFE
#define assert(x) do {
}
while (0)
#endif

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
typedef uint32_t tag_t;
typedef uint32_t refcount_t;

struct qdbp_object;
struct qdbp_method {
  struct qdbp_object **captures;
  size_t captures_size; // FIXME: Get rid of somehow?
  void *code;
};
struct qdbp_field {
  struct qdbp_method method;
};

struct qdbp_prototype {
  Pvoid_t labels;
};

struct qdbp_variant {
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
  QDBP_VARIANT // Must be last
};

struct __attribute__((packed)) qdbp_object_metadata {
  refcount_t rc;
  tag_t tag;
};
struct qdbp_object {
  struct qdbp_object_metadata metadata;
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

// Reference counting
bool is_unique(qdbp_object_ptr obj);
void incref(qdbp_object_ptr obj, refcount_t amount);
void decref(qdbp_object_ptr obj, refcount_t amount);
void set_refcount(qdbp_object_ptr obj, refcount_t refcount);
refcount_t get_refcount(qdbp_object_ptr obj);

#define assert_refcount(obj)                                                   \
  assert(!is_unboxed_int(obj));                                                \
  do {                                                                         \
    if (VERIFY_REFCOUNTS) {                                                    \
      assert((obj));                                                           \
      if (get_refcount(obj) <= 0) {                                            \
        printf("refcount of %u\n", get_refcount(obj));                         \
        assert(false);                                                         \
      };                                                                       \
    }                                                                          \
  } while (0);
void drop(qdbp_object_ptr obj, refcount_t cnt);
void obj_dup(qdbp_object_ptr obj, refcount_t cnt);
void dup_captures(qdbp_method_ptr method);
void dup_prototype_captures(qdbp_prototype_ptr proto);
void dup_prototype_captures_except(qdbp_prototype_ptr proto, label_t except);
// Memory
void *qdbp_malloc(size_t size);
void qdbp_free(void *ptr);
void qdbp_memcpy(void *dest, const void *src, size_t n);
void qdbp_free_field(qdbp_field_ptr field);
qdbp_field_ptr qdbp_malloc_field();
void free_fields(qdbp_prototype_ptr proto);
void qdbp_free_obj(qdbp_object_ptr obj);
void check_mem();
void del_prototype(qdbp_prototype_ptr proto);
void del_variant(qdbp_variant_ptr variant);
qdbp_object_arr make_capture_arr(size_t size);
void free_capture_arr(qdbp_object_arr arr, size_t size);
void del_method(qdbp_method_ptr method);
void del_obj(qdbp_object_ptr obj);
qdbp_object_ptr qdbp_malloc_obj();
// Object creation
qdbp_object_ptr make_object(tag_t tag, union qdbp_object_data data);
qdbp_object_ptr empty_prototype();
qdbp_object_ptr qdbp_true();
qdbp_object_ptr qdbp_false();

// Prototypes

size_t proto_size(qdbp_prototype_ptr proto);
void label_add(qdbp_prototype_ptr proto, label_t label, qdbp_field_ptr field);
void label_replace(qdbp_prototype_ptr proto, label_t label,
                   qdbp_field_ptr field);
qdbp_field_ptr label_get(qdbp_prototype_ptr proto, label_t label);
void copy_captures_except(qdbp_prototype_ptr new_prototype, label_t except);
qdbp_object_arr get_method(qdbp_object_ptr obj, label_t label,
                           void **code_ptr /*output param*/);
qdbp_object_arr make_captures(qdbp_object_arr captures, size_t size);
__attribute__((always_inline)) qdbp_object_ptr extend(qdbp_object_ptr obj,
                                                      label_t label, void *code,
                                                      qdbp_object_arr captures,
                                                      size_t captures_size);
qdbp_object_ptr invoke_1(qdbp_object_ptr receiver, label_t label,
                         qdbp_object_ptr arg0);
qdbp_object_ptr invoke_2(qdbp_object_ptr receiver, label_t label,
                         qdbp_object_ptr arg0, qdbp_object_ptr arg1);
__attribute__((always_inline)) qdbp_object_ptr
replace(qdbp_object_ptr obj, label_t label, void *code,
        qdbp_object_arr captures, size_t captures_size);

// MUST KEEP IN SYNC WITH namesToInts.ml
enum NUMBER_LABELS {
  VAL = 0,
  PRINT = 1,
  ADD = 2,
  SUB = 3,
  MUL = 4,
  DIV = 5,
  MOD = 6,
  EQ = 7,
  NEQ = 8,
  LT = 9,
  GT = 10,
  LEQ = 11,
  GEQ = 12,
};

// Unboxed ints
bool is_unboxed_int(qdbp_object_ptr obj);
qdbp_object_ptr make_unboxed_int(int64_t value);
int64_t get_unboxed_int(qdbp_object_ptr obj);
qdbp_object_ptr unboxed_unary_op(qdbp_object_ptr obj, label_t op);
qdbp_object_ptr unboxed_binary_op(qdbp_object_ptr a, qdbp_object_ptr b, label_t op);
// Tags and Variants
enum qdbp_object_kind get_kind(qdbp_object_ptr obj);
void set_tag(qdbp_object_ptr o, tag_t t);
tag_t get_tag(qdbp_object_ptr o);
qdbp_object_ptr variant_create(tag_t tag, qdbp_object_ptr value);
void decompose_variant(qdbp_object_ptr obj, tag_t *tag,
                       qdbp_object_ptr *payload);
#define assert_obj_kind(obj, k)                                                \
  do {                                                                         \
    if (DYNAMIC_TYPECHECK) {                                                   \
      assert_refcount(obj);                                                    \
      assert(get_kind(obj) == k);                                              \
    }                                                                          \
  } while (0);

// Macros/Fns for the various kinds of expressions
#define DROP(v, cnt, expr) (drop(v, cnt), (expr))
#define DUP(v, cnt, expr) (obj_dup(v, cnt), (expr))
#define LET(lhs, rhs, in)                                                      \
  ((lhs = (rhs)), (in)) // assume lhs has been declared already
#define MATCH(tag1, tag2, arg, ifmatch, ifnomatch)                             \
  ((tag1) == (tag2) ? (LET(arg, payload, ifmatch)) : (ifnomatch))

qdbp_object_ptr match_failed();
qdbp_object_ptr qdbp_int(int64_t i);
qdbp_object_ptr qdbp_string(const char *src);
qdbp_object_ptr qdbp_float(double f);

void init();
#endif
