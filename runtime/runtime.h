// This file gets included first in every qdbp progrram

#ifndef QDBP_RUNTIME_H
#define QDBP_RUNTIME_H
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// config
// You could also add fsanitize=undefined
// Also could adjust inlining
static const bool REFCOUNT = true;
static const bool REUSE_ANALYSIS = true;
static const bool OBJ_FREELIST = true;
static const bool BOX_FREELIST = true;
static const bool HASHTABLE_FREELIST = true;
static const bool DIRECTORY_FREELIST = true;

#define FREELIST_SIZE 1000

// Dynamic checks
static const bool CHECK_MALLOC_FREE = false; // very slow
static const bool VERIFY_REFCOUNTS = false;
static const bool DYNAMIC_TYPECHECK = false;

// Hashtable settings
static const size_t INITIAL_CAPACITY = 16;
static const size_t INITIAL_CAPACITY_LG2 = 4;
static const size_t MAX_LOAD_FACTOR = 2;

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
  label_t label;
  struct qdbp_method method;
};
struct hashtable_header {
  size_t capacity;
  size_t capacity_lg2;
  size_t size;
  size_t *directory;
};
typedef union {
  struct hashtable_header header;
  struct qdbp_field field;
} hashtable;

struct qdbp_prototype {
  hashtable *labels;
};

struct qdbp_variant {
  struct qdbp_object *value;
};

struct boxed_int {
  int64_t value;
  struct qdbp_prototype other_labels;
};
union qdbp_object_data {
  struct qdbp_prototype prototype;
  int64_t i;
  double f;
  char *s;
  struct boxed_int *boxed_int;
  struct qdbp_variant variant;
};

enum qdbp_object_kind {
  QDBP_INT,
  QDBP_FLOAT,
  QDBP_STRING,
  QDBP_PROTOTYPE,
  QDBP_BOXED_INT,
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
__attribute__((always_inline)) bool is_unique(qdbp_object_ptr obj);
__attribute__((always_inline)) void incref(qdbp_object_ptr obj,
                                           refcount_t amount);
__attribute__((always_inline)) void decref(qdbp_object_ptr obj,
                                           refcount_t amount);
__attribute__((always_inline)) void set_refcount(qdbp_object_ptr obj,
                                                 refcount_t refcount);
__attribute__((always_inline)) refcount_t get_refcount(qdbp_object_ptr obj);

#define assert_refcount(obj)                                                   \
  do {                                                                         \
    if (VERIFY_REFCOUNTS && obj) {                                             \
      assert(!is_unboxed_int(obj));                                            \
      assert((obj));                                                           \
      if (get_refcount(obj) <= 0) {                                            \
        printf("refcount of %u\n", get_refcount(obj));                         \
        assert(false);                                                         \
      };                                                                       \
    }                                                                          \
  } while (0);
__attribute__((always_inline)) void drop(qdbp_object_ptr obj, refcount_t cnt);
__attribute__((always_inline)) void obj_dup(qdbp_object_ptr obj,
                                            refcount_t cnt);
void dup_captures(qdbp_method_ptr method);
void dup_prototype_captures(qdbp_prototype_ptr proto);
void dup_prototype_captures_except(qdbp_prototype_ptr proto, label_t except);
// Hash table
size_t hashtable_size(hashtable *table);
__attribute__((always_inline))
hashtable *new_ht();
__attribute__((always_inline))
void del_ht(hashtable *table);
__attribute__((always_inline))
hashtable *ht_duplicate(hashtable *table);
__attribute__((always_inline))
qdbp_field_ptr ht_find(hashtable *table, label_t label);
 __attribute__((warn_unused_result))
__attribute__((always_inline))
hashtable *ht_insert(hashtable *table, const qdbp_field_ptr fld);
#define HT_ITER(ht, fld, tmp)                                                  \
  for (tmp = 0; tmp < (ht)->header.size &&                                     \
                (fld = &((ht)[(ht)->header.directory[tmp]].field), true);         \
       tmp++)
// Memory
hashtable *qdbp_calloc_hashtable();
hashtable *qdbp_malloc_hashtable();
void qdbp_free_hashtable(hashtable *ht);
void duplicate_labels(qdbp_prototype_ptr src,
                                                qdbp_prototype_ptr dest);
void *qdbp_malloc(size_t size, const char *message);
void *qdbp_calloc(size_t nitems, size_t elem_size, const char *msg);
void qdbp_free(void *ptr);
void qdbp_memcpy(void *dest, const void *src, size_t n);
__attribute__((always_inline)) void qdbp_free_field(qdbp_field_ptr field);
void qdbp_free_boxed_int(struct boxed_int *i);
struct boxed_int *qdbp_malloc_boxed_int();
void free_fields(qdbp_prototype_ptr proto);
__attribute__((always_inline)) void qdbp_free_obj(qdbp_object_ptr obj);
void check_mem();
void del_prototype(qdbp_prototype_ptr proto);
void del_variant(qdbp_variant_ptr variant);
qdbp_object_arr make_capture_arr(size_t size);
void free_capture_arr(qdbp_object_arr arr, size_t size);
void del_method(qdbp_method_ptr method);
void del_obj(qdbp_object_ptr obj);
qdbp_object_ptr qdbp_malloc_obj();
void qdbp_free_directory(size_t *directory, hashtable *ht);
size_t *qdbp_malloc_directory();
// Object creation
__attribute__((always_inline)) qdbp_object_ptr
make_object(tag_t tag, union qdbp_object_data data);
__attribute__((always_inline)) qdbp_object_ptr empty_prototype();
__attribute__((always_inline)) qdbp_object_ptr qdbp_true();
__attribute__((always_inline)) qdbp_object_ptr qdbp_false();
__attribute__((always_inline)) qdbp_object_ptr int_proto(int64_t i);

// Prototypes

__attribute__((always_inline)) size_t proto_size(qdbp_prototype_ptr proto);
__attribute__((always_inline)) void
label_add(qdbp_prototype_ptr proto, label_t label, qdbp_field_ptr field);
__attribute__((always_inline)) qdbp_field_ptr
label_get(qdbp_prototype_ptr proto, label_t label);
void copy_captures_except(qdbp_prototype_ptr new_prototype, label_t except);
__attribute__((always_inline)) qdbp_object_arr
get_method(qdbp_object_ptr obj, label_t label,
           void **code_ptr /*output param*/);
qdbp_object_arr make_captures(qdbp_object_arr captures, size_t size);
__attribute__((always_inline)) qdbp_object_ptr extend(qdbp_object_ptr obj,
                                                      label_t label, void *code,
                                                      qdbp_object_arr captures,
                                                      size_t captures_size);
__attribute__((always_inline)) qdbp_object_ptr
invoke_1(qdbp_object_ptr receiver, label_t label, qdbp_object_ptr arg0);
__attribute__((always_inline)) qdbp_object_ptr
invoke_2(qdbp_object_ptr receiver, label_t label, qdbp_object_ptr arg0,
         qdbp_object_ptr arg1);
qdbp_object_ptr replace(qdbp_object_ptr obj, label_t label, void *code,
                        qdbp_object_arr captures, size_t captures_size);

// MUST KEEP IN SYNC WITH namesToInts.ml
enum NUMBER_LABELS {
  VAL = 0,
  PRINT = 1,
  ADD = 2, // MUST be the first arith op after all unary ops
  SUB = 3,
  MUL = 4,
  DIV = 5,
  MOD = 6, // MUST be the last op before all the comparison ops
  EQ = 7,  // MUST be the first op after all the arithmetic ops
  NEQ = 8,
  LT = 9,
  GT = 10,
  LEQ = 11,
  GEQ = 12,
  NUM_OP_CNT
};

// ints
__attribute__((always_inline)) bool is_unboxed_int(qdbp_object_ptr obj);
__attribute__((always_inline)) qdbp_object_ptr make_unboxed_int(int64_t value);
int64_t get_unboxed_int(qdbp_object_ptr obj);
qdbp_object_ptr unboxed_unary_op(qdbp_object_ptr obj, label_t op);
qdbp_object_ptr unboxed_binary_op(int64_t a, int64_t b, label_t op);
bool is_boxed_int(qdbp_object_ptr obj);
int64_t get_boxed_int(qdbp_object_ptr obj);
qdbp_object_ptr boxed_binary_op(qdbp_object_ptr a, qdbp_object_ptr b,
                                label_t op);
qdbp_object_ptr make_int_proto(int64_t value);
qdbp_object_ptr box(qdbp_object_ptr obj, label_t label, void *code,
                    qdbp_object_arr captures, size_t captures_size);
qdbp_object_ptr box_extend(qdbp_object_ptr obj, label_t label, void *code,
                           qdbp_object_arr captures, size_t captures_size);
qdbp_object_ptr boxed_int_replace(qdbp_object_ptr obj, label_t label,
                                  void *code, qdbp_object_arr captures,
                                  size_t captures_size);
qdbp_object_ptr boxed_unary_op(qdbp_object_ptr arg0, label_t label);
// Tags and Variants
__attribute__((always_inline)) enum qdbp_object_kind
get_kind(qdbp_object_ptr obj);
__attribute__((always_inline)) void set_tag(qdbp_object_ptr o, tag_t t);
__attribute__((always_inline)) tag_t get_tag(qdbp_object_ptr o);
__attribute__((always_inline)) qdbp_object_ptr
variant_create(tag_t tag, qdbp_object_ptr value);
__attribute__((always_inline)) void
decompose_variant(qdbp_object_ptr obj, tag_t *tag, qdbp_object_ptr *payload);
#define assert_obj_kind(obj, k)                                                \
  do {                                                                         \
    if (obj && DYNAMIC_TYPECHECK) {                                            \
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
__attribute__((always_inline)) qdbp_object_ptr qdbp_int(int64_t i);
__attribute__((always_inline)) qdbp_object_ptr qdbp_string(const char *src);
__attribute__((always_inline)) qdbp_object_ptr qdbp_float(double f);

void init();
#endif
