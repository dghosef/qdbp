// Rename everything
// Reogranize everything
// Make everything static
// Check the naming in macros
// Maybe add a C-style preprocessor
#ifndef QDBP_RUNTIME_H
#define QDBP_RUNTIME_H
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// runtime.h
#ifdef QDBP_RUNTIME_H
// config
// You could also add fsanitize=undefined
// Also could adjust inlining
#define _QDBP_REFCOUNT true
#define _QDBP_REUSE_ANALYSIS true
#define _QDBP_OBJ_FREELIST true
#define _QDBP_BOX_FREELIST false

#define _QDBP_FREELIST_SIZE 1000

// Dynamic checks
#define _QDBP_CHECK_MALLOC_FREE false
#define _QDBP_VERIFY_REFCOUNTS false
#define _QDBP_DYNAMIC_TYPECHECK false

// Hashtable settings
/*
  Currently, the strategy is to initialize the hashtable to be 1/2 full and
  let it grow till it is full. Then, we resize. This is because extension is
  relatively rare
*/
static const size_t _QDBP_LOAD_FACTOR_NUM = 1;
static const size_t _QDBP_LOAD_FACTOR_DEN = 1;

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

typedef uint32_t _qdbp_label_t;
typedef uint32_t _qdbp_tag_t;
typedef uint32_t _qdbp_refcount_t;

// FIXME: At some point check that we don't have too many captures
struct _qdbp_object;
struct __attribute__((packed)) _qdbp_method {
  struct _qdbp_object **captures;
  void *code;
  uint32_t captures_size;  // FIXME: Get rid of somehow?
};
struct __attribute__((packed)) __attribute__((aligned(sizeof(void *))))
_qdbp_field {
  struct _qdbp_method method;
  uint32_t label;
};
struct _qdbp_hashtable_header {
  size_t capacity;
  size_t size;
  size_t *directory;
};
typedef union {
  struct _qdbp_hashtable_header header;
  struct _qdbp_field field;
} _qdbp_hashtable;
struct _qdbp_prototype {
  _qdbp_hashtable *labels;
};

struct _qdbp_variant {
  struct _qdbp_object *value;
};

struct _boxed_int {
  int64_t value;
  struct _qdbp_prototype other_labels;
};
union qdbp_object_data {
  struct _qdbp_prototype prototype;
  int64_t i;
  double f;
  char *s;
  struct _boxed_int *boxed_int;
  struct _qdbp_variant variant;
};

enum qdbp_object_kind {
  QDBP_INT,
  QDBP_FLOAT,
  QDBP_STRING,
  QDBP_PROTOTYPE,
  QDBP_BOXED_INT,
  QDBP_VARIANT  // Must be last
};

struct __attribute__((packed)) qdbp_object_metadata {
  _qdbp_refcount_t rc;
  _qdbp_tag_t tag;
};
struct _qdbp_object {
  struct qdbp_object_metadata metadata;
  union qdbp_object_data data;
};
typedef struct _qdbp_object *qdbp_object_ptr;
typedef struct _qdbp_object **qdbp_object_arr;
typedef struct _qdbp_variant *qdbp_variant_ptr;
typedef struct _qdbp_prototype *qdbp_prototype_ptr;
typedef struct _qdbp_method *qdbp_method_ptr;
typedef struct _qdbp_field *qdbp_field_ptr;
/*
====================================================
Basic Utilities
====================================================
*/

// Reference counting
bool _qdbp_is_unique(qdbp_object_ptr obj);
void _qdbp_incref(qdbp_object_ptr obj, _qdbp_refcount_t amount);
void _qdbp_decref(qdbp_object_ptr obj, _qdbp_refcount_t amount);
void _qdbp_set_refcount(qdbp_object_ptr obj, _qdbp_refcount_t refcount);
_qdbp_refcount_t _qdbp_get_refcount(qdbp_object_ptr obj);

#define assert_refcount(obj)                                 \
  do {                                                       \
    if (_QDBP_VERIFY_REFCOUNTS && obj) {                     \
      assert(!_qdbp_is_unboxed_int(obj));                    \
      assert((obj));                                         \
      if (_qdbp_get_refcount(obj) <= 0) {                    \
        printf("refcount of %u\n", _qdbp_get_refcount(obj)); \
        assert(false);                                       \
      };                                                     \
    }                                                        \
  } while (0);
__attribute__((always_inline)) void _qdbp_drop(qdbp_object_ptr obj,
                                               _qdbp_refcount_t cnt);
void _qdbp_obj_dup(qdbp_object_ptr obj, _qdbp_refcount_t cnt);
void _qdbp_dup_captures(qdbp_method_ptr method);
void _qdbp_dup_prototype_captures(qdbp_prototype_ptr proto);
void _qdbp_dup_prototype_captures_except(qdbp_prototype_ptr proto,
                                         _qdbp_label_t except);
// Hash table
size_t _qdbp_hashtable_size(_qdbp_hashtable *table);
_qdbp_hashtable *_qdbp_new_ht(size_t capacity);
void _qdbp_del_ht(_qdbp_hashtable *table);
_qdbp_hashtable *_qdbp_ht_duplicate(_qdbp_hashtable *table);
qdbp_field_ptr _qdbp_ht_find(_qdbp_hashtable *table, _qdbp_label_t label);
__attribute__((warn_unused_result)) _qdbp_hashtable *_qdbp_ht_insert(
    _qdbp_hashtable *table, const qdbp_field_ptr fld);
#define HT_ITER(ht, fld, tmp)                                             \
  for (tmp = 0; tmp < (ht)->header.size &&                                \
                (fld = &((ht)[(ht)->header.directory[tmp]].field), true); \
       tmp++)
// Memory
void _qdbp_duplicate_labels(qdbp_prototype_ptr src, qdbp_prototype_ptr dest);
void *_qdbp_malloc(size_t size, const char *message);
void _qdbp_free(void *ptr);
void _qdbp_memcpy(void *dest, const void *src, size_t n);
void _qdbp_free_field(qdbp_field_ptr field);
void _qdbp_free_boxed_int(struct _boxed_int *i);
struct _boxed_int *_qdbp_malloc_boxed_int();
void _qdbp_free_fields(qdbp_prototype_ptr proto);
void _qdbp_free_obj(qdbp_object_ptr obj);
void _qdbp_check_mem();
void _qdbp_del_prototype(qdbp_prototype_ptr proto);
void _qdbp_del_variant(qdbp_variant_ptr variant);
qdbp_object_arr _qdbp_make_capture_arr(size_t size);
void _qdbp_free_capture_arr(qdbp_object_arr arr, size_t size);
void _qdbp_del_method(qdbp_method_ptr method);
void _qdbp_del_obj(qdbp_object_ptr obj);
qdbp_object_ptr _qdbp_malloc_obj();
size_t *_qdbp_malloc_directory();
// Object creation
qdbp_object_ptr _qdbp_make_object(_qdbp_tag_t tag, union qdbp_object_data data);
qdbp_object_ptr _qdbp_empty_prototype();
qdbp_object_ptr _qdbp_true();
qdbp_object_ptr _qdbp_false();
qdbp_object_ptr _qdbp_int_proto(int64_t i);

// Prototypes

size_t _qdbp_proto_size(qdbp_prototype_ptr proto);
void _qdbp_label_add(qdbp_prototype_ptr proto, _qdbp_label_t label,
                     qdbp_field_ptr field, size_t defaultsize);
qdbp_field_ptr _qdbp_label_get(qdbp_prototype_ptr proto, _qdbp_label_t label);
void _qdbp_copy_captures_except(qdbp_prototype_ptr new_prototype,
                                _qdbp_label_t except);
qdbp_object_arr _qdbp_get_method(qdbp_object_ptr obj, _qdbp_label_t label,
                                 void **code_ptr /*output param*/);
qdbp_object_arr _qdbp_make_captures(qdbp_object_arr captures, size_t size);
__attribute__((always_inline)) qdbp_object_ptr _qdbp_extend(
    qdbp_object_ptr obj, _qdbp_label_t label, void *code,
    qdbp_object_arr captures, size_t captures_size, size_t defaultsize);
__attribute__((always_inline)) qdbp_object_ptr _qdbp_invoke_1(
    qdbp_object_ptr receiver, _qdbp_label_t label, qdbp_object_ptr arg0);
__attribute__((always_inline)) qdbp_object_ptr _qdbp_invoke_2(
    qdbp_object_ptr receiver, _qdbp_label_t label, qdbp_object_ptr arg0,
    qdbp_object_ptr arg1);

__attribute__((always_inline)) qdbp_object_ptr _qdbp_replace(
    qdbp_object_ptr obj, _qdbp_label_t label, void *code,
    qdbp_object_arr captures, size_t captures_size);

// MUST KEEP IN SYNC WITH namesToInts.ml
enum NUMBER_LABELS {
  VAL = 64,
  PRINT = 69,
  ADD = 84,  // MUST be the first arith op after all unary ops
  SUB = 139,
  MUL = 140,
  DIV = 193,
  MOD = 254,  // MUST be the last op before all the comparison ops
  EQ = 306,   // MUST be the first op after all the arithmetic ops
  NEQ = 355,
  LT = 362,
  GT = 447,
  LEQ = 455,
  GEQ = 461,
  MAX_OP = 1000
};

// ints
bool _qdbp_is_unboxed_int(qdbp_object_ptr obj);
qdbp_object_ptr _qdbp_make_unboxed_int(int64_t value);
int64_t _qdbp_get_unboxed_int(qdbp_object_ptr obj);
qdbp_object_ptr _qdbp_unboxed_unary_op(qdbp_object_ptr obj, _qdbp_label_t op);
qdbp_object_ptr _qdbp_unboxed_binary_op(int64_t a, int64_t b, _qdbp_label_t op);
bool _qdbp_is_boxed_int(qdbp_object_ptr obj);
int64_t _qdbp_get_boxed_int(qdbp_object_ptr obj);
qdbp_object_ptr _qdbp_boxed_binary_op(qdbp_object_ptr a, qdbp_object_ptr b,
                                      _qdbp_label_t op);
qdbp_object_ptr _qdbp_make_int_proto(int64_t value);
qdbp_object_ptr _qdbp_box(qdbp_object_ptr obj, _qdbp_label_t label, void *code,
                          qdbp_object_arr captures, size_t captures_size);
qdbp_object_ptr _qdbp_box_extend(qdbp_object_ptr obj, _qdbp_label_t label,
                                 void *code, qdbp_object_arr captures,
                                 size_t captures_size);
qdbp_object_ptr _qdbp_boxed_int_replace(qdbp_object_ptr obj,
                                        _qdbp_label_t label, void *code,
                                        qdbp_object_arr captures,
                                        size_t captures_size);
qdbp_object_ptr _qdbp_boxed_unary_op(qdbp_object_ptr arg0, _qdbp_label_t label);
// Tags and Variants
enum qdbp_object_kind _qdbp_get_kind(qdbp_object_ptr obj);
void _qdbp_set_tag(qdbp_object_ptr o, _qdbp_tag_t t);
_qdbp_tag_t _qdbp_get_tag(qdbp_object_ptr o);
qdbp_object_ptr _qdbp_variant_create(_qdbp_tag_t tag, qdbp_object_ptr value);
void _qdbp_decompose_variant(qdbp_object_ptr obj, _qdbp_tag_t *tag,
                             qdbp_object_ptr *payload);
#define assert_obj_kind(obj, k)           \
  do {                                    \
    if (obj && _QDBP_DYNAMIC_TYPECHECK) { \
      assert_refcount(obj);               \
      assert(_qdbp_get_kind(obj) == k);   \
    }                                     \
  } while (0);

// Macros/Fns for the various kinds of expressions
#define DROP(v, cnt, expr) (drop(v, cnt), (expr))
#define DUP(v, cnt, expr) (obj_dup(v, cnt), (expr))
#define LET(lhs, rhs, in) \
  ((lhs = (rhs)), (in))  // assume lhs has been declared already
#define MATCH(tag1, tag2, arg, ifmatch, ifnomatch) \
  ((tag1) == (tag2) ? (LET(arg, payload, ifmatch)) : (ifnomatch))

qdbp_object_ptr _qdbp_match_failed();
qdbp_object_ptr _qdbp_int(int64_t i);
qdbp_object_ptr _qdbp_string(const char *src);
qdbp_object_ptr _qdbp_float(double f);

void _qdbp_init();
#endif
// basic_fns.h
#ifdef QDBP_RUNTIME_H
static qdbp_object_ptr qdbp_abort() {
  printf("aborting\n");
  exit(0);
}

#define qdbp_int_binop(name, op)                                            \
  static qdbp_object_ptr name(qdbp_object_ptr a, qdbp_object_ptr b) {       \
    assert_obj_kind(a, QDBP_INT);                                           \
    assert_obj_kind(b, QDBP_INT);                                           \
    if (_qdbp_is_unique(a)) {                                               \
      a->data.i = a->data.i op b->data.i;                                   \
      _qdbp_drop(b, 1);                                                     \
      return a;                                                             \
    } else if (_qdbp_is_unique(b)) {                                        \
      b->data.i = a->data.i op b->data.i;                                   \
      _qdbp_drop(a, 1);                                                     \
      return b;                                                             \
    } else {                                                                \
      qdbp_object_ptr result = _qdbp_make_object(                           \
          QDBP_INT, (union qdbp_object_data){.i = a->data.i op b->data.i}); \
      _qdbp_drop(a, 1);                                                     \
      _qdbp_drop(b, 1);                                                     \
      return result;                                                        \
    }                                                                       \
  }

#define qdbp_float_binop(name, op)                                            \
  static qdbp_object_ptr name(qdbp_object_ptr a, qdbp_object_ptr b) {         \
    assert_obj_kind(a, QDBP_FLOAT);                                           \
    assert_obj_kind(b, QDBP_FLOAT);                                           \
    if (_qdbp_is_unique(a)) {                                                 \
      a->data.f = a->data.f op b->data.f;                                     \
      _qdbp_drop(b, 1);                                                       \
      return a;                                                               \
    } else if (_qdbp_is_unique(b)) {                                          \
      b->data.f = a->data.f op b->data.f;                                     \
      _qdbp_drop(a, 1);                                                       \
      return b;                                                               \
    } else {                                                                  \
      qdbp_object_ptr result = _qdbp_make_object(                             \
          QDBP_FLOAT, (union qdbp_object_data){.f = a->data.f op b->data.f}); \
      _qdbp_drop(a, 1);                                                       \
      _qdbp_drop(b, 1);                                                       \
      return result;                                                          \
    }                                                                         \
  }

#define qdbp_intcmp_binop(name, op)                                   \
  static qdbp_object_ptr name(qdbp_object_ptr a, qdbp_object_ptr b) { \
    assert_obj_kind(a, QDBP_INT);                                     \
    assert_obj_kind(b, QDBP_INT);                                     \
    qdbp_object_ptr result =                                          \
        a->data.i op b->data.i ? _qdbp_true() : _qdbp_false();        \
    _qdbp_drop(a, 1);                                                 \
    _qdbp_drop(b, 1);                                                 \
    return result;                                                    \
  }

#define qdbp_floatcmp_binop(name, op)                                 \
  static qdbp_object_ptr name(qdbp_object_ptr a, qdbp_object_ptr b) { \
    assert_obj_kind(a, QDBP_FLOAT);                                   \
    assert_obj_kind(b, QDBP_FLOAT);                                   \
    qdbp_object_ptr result =                                          \
        a->data.f op b->data.f ? _qdbp_true() : _qdbp_false();        \
    _qdbp_drop(a, 1);                                                 \
    _qdbp_drop(b, 1);                                                 \
    return result;                                                    \
  }
static char *empty_string() {
  char *s = (char *)_qdbp_malloc(1, "empty_string");
  s[0] = '\0';
  return s;
}
static char *c_str_concat(const char *a, const char *b) {
  int lena = strlen(a);
  int lenb = strlen(b);
  char *con = (char *)_qdbp_malloc(lena + lenb + 1, "c_str_concat");
  // copy & concat (including string termination)
  memcpy(con, a, lena);
  memcpy(con + lena, b, lenb + 1);
  return con;
}
// concat_string
static qdbp_object_ptr concat_string(qdbp_object_ptr a, qdbp_object_ptr b) {
  assert_obj_kind(a, QDBP_STRING);
  assert_obj_kind(b, QDBP_STRING);
  const char *a_str = a->data.s;
  const char *b_str = b->data.s;
  _qdbp_drop(a, 1);
  _qdbp_drop(b, 1);
  return _qdbp_make_object(
      QDBP_STRING, (union qdbp_object_data){.s = c_str_concat(a_str, b_str)});
}
// qdbp_print_string_int
static qdbp_object_ptr qdbp_print_string_int(qdbp_object_ptr s) {
  assert_obj_kind(s, QDBP_STRING);
  printf("%s", s->data.s);
  fflush(stdout);
  _qdbp_drop(s, 1);
  return _qdbp_make_object(QDBP_INT, (union qdbp_object_data){.i = 0});
}
// qdbp_float_to_string
static qdbp_object_ptr qdbp_empty_string() {
  return _qdbp_make_object(QDBP_STRING,
                           (union qdbp_object_data){.s = empty_string()});
}
static qdbp_object_ptr qdbp_zero_int() {
  return _qdbp_make_object(QDBP_INT, (union qdbp_object_data){.i = 0});
}

// the autoformatter is weird
qdbp_int_binop(qdbp_int_add_int, +);
qdbp_int_binop(qdbp_int_sub_int, -);
qdbp_int_binop(qdbp_int_mul_int, *);
qdbp_int_binop(qdbp_int_div_int, /);
qdbp_int_binop(qdbp_int_mod_int, %);
qdbp_intcmp_binop(qdbp_int_lt_bool, <);
qdbp_intcmp_binop(qdbp_int_le_bool, <=);
qdbp_intcmp_binop(qdbp_int_gt_bool, >);
qdbp_intcmp_binop(qdbp_int_ge_bool, >=);
qdbp_intcmp_binop(qdbp_int_eq_bool, ==);
qdbp_intcmp_binop(qdbp_int_ne_bool, !=);
static size_t _qdbp_itoa_bufsize(int64_t i) {
  size_t result = 2;  // '\0' at the end, first digit
  if (i < 0) {
    i *= -1;
    result++;  // '-' character
  }
  while (i > 9) {
    result++;
    i /= 10;
  }
  return result;
}
static qdbp_object_ptr _qdbp_int_to_string(qdbp_object_ptr i) {
  assert_obj_kind(i, QDBP_INT);
  int64_t val = i->data.i;
  _qdbp_drop(i, 1);
  int bufsize = _qdbp_itoa_bufsize(val);
  char *s = (char *)_qdbp_malloc(bufsize, "qdbp_int_to_string");
  assert(snprintf(s, bufsize, "%lld", val) == bufsize - 1);
  return _qdbp_make_object(QDBP_STRING, (union qdbp_object_data){.s = s});
}

static qdbp_object_ptr _qdbp_zero_float() {
  return _qdbp_make_object(QDBP_FLOAT, (union qdbp_object_data){.f = 0.0});
}
qdbp_float_binop(qdbp_float_add_float, +);
qdbp_float_binop(qdbp_float_sub_float, -);
qdbp_float_binop(qdbp_float_mul_float, *);
qdbp_float_binop(qdbp_float_div_float, /)
    qdbp_floatcmp_binop(qdbp_float_lt_bool, <);
qdbp_floatcmp_binop(qdbp_float_le_bool, <=)
    qdbp_floatcmp_binop(qdbp_float_gt_bool, >);
qdbp_floatcmp_binop(qdbp_float_ge_bool, >=);

static qdbp_object_ptr _qdbp_float_mod_float(qdbp_object_ptr a,
                                             qdbp_object_ptr b) {
  assert_obj_kind(a, QDBP_FLOAT);
  assert_obj_kind(b, QDBP_FLOAT);
  qdbp_object_ptr result = _qdbp_make_object(
      QDBP_FLOAT, (union qdbp_object_data){.f = fmod(a->data.f, b->data.f)});
  _qdbp_drop(a, 1);
  _qdbp_drop(b, 1);
  return result;
}
#endif
// basic_objs.c
#ifdef QDBP_RUNTIME_H
qdbp_object_ptr _qdbp_int(int64_t i) {
  qdbp_object_ptr new_obj =
      _qdbp_make_object(QDBP_INT, (union qdbp_object_data){.i = i});
  return new_obj;
}

qdbp_object_ptr _qdbp_string(const char *src) {
  char *dest = (char *)_qdbp_malloc(strlen(src) + 1, "qdbp_string");
  strcpy(dest, src);
  qdbp_object_ptr new_obj =
      _qdbp_make_object(QDBP_STRING, (union qdbp_object_data){.s = dest});
  return new_obj;
}

qdbp_object_ptr _qdbp_float(double f) {
  qdbp_object_ptr new_obj =
      _qdbp_make_object(QDBP_FLOAT, (union qdbp_object_data){.f = f});
  return new_obj;
}

qdbp_object_ptr _qdbp_match_failed() {
  assert(false);
  __builtin_unreachable();
}

// Keep track of objects for testing
qdbp_object_ptr _qdbp_make_object(_qdbp_tag_t tag,
                                  union qdbp_object_data data) {
  qdbp_object_ptr new_obj = _qdbp_malloc_obj();
  _qdbp_set_refcount(new_obj, 1);
  _qdbp_set_tag(new_obj, tag);
  new_obj->data = data;
  return new_obj;
}

qdbp_object_ptr _qdbp_empty_prototype() { return NULL; }
qdbp_object_ptr _qdbp_true() { return (qdbp_object_ptr)(0xF0); }
qdbp_object_ptr _qdbp_false() { return (qdbp_object_ptr)(0xE0); }
#endif
// hashtable.c
#ifdef QDBP_RUNTIME_H
__attribute__((always_inline)) size_t _qdbp_hashtable_size(
    _qdbp_hashtable *table) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(table);
  }
  return table->header.size;
}

__attribute__((always_inline)) _qdbp_hashtable *_qdbp_new_ht(size_t capacity) {
  // FIXME: Put into freelist
  _qdbp_hashtable *table = (_qdbp_hashtable *)_qdbp_malloc(
      (capacity + 1) * sizeof(_qdbp_hashtable), "ht");
  for (size_t i = 1; i <= capacity; i++) {
    table[i].field.method.code = NULL;
  }
  table->header.size = 0;
  table->header.capacity = capacity;
  table->header.directory = _qdbp_malloc(capacity * sizeof(size_t), "ht");
  return table;
}

__attribute__((always_inline)) void _qdbp_del_ht(_qdbp_hashtable *table) {
  _qdbp_free(table->header.directory);
  _qdbp_free(table);
}

__attribute__((always_inline)) _qdbp_hashtable *_qdbp_ht_duplicate(
    _qdbp_hashtable *table) {
  _qdbp_hashtable *new_table =
      _qdbp_malloc(sizeof(_qdbp_hashtable) +
                       (sizeof(_qdbp_hashtable) * table->header.capacity),
                   "ht");
  memcpy(new_table, table,
         sizeof(_qdbp_hashtable) +
             (sizeof(_qdbp_hashtable) * (table->header.capacity)));
  new_table->header.directory =
      _qdbp_malloc(sizeof(size_t) * (table->header.capacity), "ht");
  // FIXME: Only memcpy the table->header.size components
  memcpy(new_table->header.directory, table->header.directory,
         sizeof(size_t) * table->header.size);
  return new_table;
}

__attribute__((always_inline))
// n must be a power of 2
static size_t
_qdbp_fast_mod(size_t x, size_t n) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    // check that n is a power of 2
    assert((n & (n - 1)) == 0);
  }
  return x & (n - 1);
}

__attribute__((always_inline)) qdbp_field_ptr _qdbp_ht_find(
    _qdbp_hashtable *table, _qdbp_label_t label) {
  size_t index = _qdbp_fast_mod(label, table->header.capacity);
  // FIXME: Assert that label != 0 and then remove second condition
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(index <= table->header.capacity);
  }
  _qdbp_hashtable *fields = table + 1;
  if (fields[index].field.label == label) {
    return &(fields[index].field);
  }
  index = _qdbp_fast_mod(index + 1, table->header.capacity);
  while (fields[index].field.label != label) {
    if (_QDBP_DYNAMIC_TYPECHECK) {
      assert(fields[index].field.method.code != NULL);
    }
    index = _qdbp_fast_mod(index + 1, table->header.capacity);
    if (_QDBP_DYNAMIC_TYPECHECK) {
      assert(index <= table->header.capacity);
    }
  }
  return &(fields[index].field);
}

__attribute__((always_inline)) static void _qdbp_ht_insert_raw(
    _qdbp_hashtable *table, const qdbp_field_ptr fld) {
  size_t index = _qdbp_fast_mod(fld->label, table->header.capacity);
  _qdbp_hashtable *fields = table + 1;
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(index <= table->header.capacity);
  }
  while (fields[index].field.method.code != NULL) {
    index = _qdbp_fast_mod(index + 1, table->header.capacity);
    if (_QDBP_DYNAMIC_TYPECHECK) {
      assert(index <= table->header.capacity);
    }
  }
  fields[index].field = *fld;
  table->header.directory[table->header.size] = index + 1;
  table->header.size++;
}

__attribute__((always_inline)) _qdbp_hashtable *_qdbp_ht_insert(
    _qdbp_hashtable *table, const qdbp_field_ptr fld) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(fld->method.code != NULL);
  }
  if (table->header.size * _QDBP_LOAD_FACTOR_NUM >=
      table->header.capacity * _QDBP_LOAD_FACTOR_DEN) {
    _qdbp_hashtable *new_table = _qdbp_malloc(
        (table->header.capacity * 2 + 1) * sizeof(_qdbp_hashtable), "ht");
    for (size_t i = 1; i < table->header.capacity * 2 + 1; i++) {
      new_table[i].field.method.code = NULL;
    }
    new_table->header.size = 0;
    new_table->header.capacity = table->header.capacity * 2;
    new_table->header.directory =
        _qdbp_malloc(sizeof(size_t) * (new_table->header.capacity), "ht");
    size_t tmp;
    qdbp_field_ptr f;
    HT_ITER(table, f, tmp) { _qdbp_ht_insert_raw(new_table, f); }
    _qdbp_free(table->header.directory);
    _qdbp_free(table);
    table = new_table;
  }
  _qdbp_ht_insert_raw(table, fld);
  return table;
}

#endif
// int_proto.c
#ifdef QDBP_RUNTIME_H
static qdbp_object_ptr _qdbp_get_capture(qdbp_object_arr captures,
                                         qdbp_object_ptr o) {
  _qdbp_drop(o, 1);
  return captures[0];
}

static qdbp_object_ptr _qdbp_print_intproto(qdbp_object_arr captures,
                                            qdbp_object_ptr o) {
  assert_obj_kind(o, QDBP_PROTOTYPE);
  o = _qdbp_invoke_1(o, VAL, o);
  assert_obj_kind(o, QDBP_INT);
  printf("%lld", o->data.i);
  _qdbp_drop(o, 1);
  return _qdbp_empty_prototype();
}

#define MK_PROTO_ARITH_OP(name, op)                                         \
  static qdbp_object_ptr name(qdbp_object_arr captures, qdbp_object_ptr a,  \
                              qdbp_object_ptr b) {                          \
    _qdbp_obj_dup(a, 1);                                                    \
    qdbp_object_ptr a_val = _qdbp_invoke_1(a, VAL, a);                      \
    qdbp_object_ptr b_val = _qdbp_invoke_1(b, VAL, b);                      \
    assert_obj_kind(a_val, QDBP_INT);                                       \
    assert_obj_kind(b_val, QDBP_INT);                                       \
    int64_t result = ((a_val->data.i op b_val->data.i) << 1) >> 1;          \
    qdbp_object_arr capture = (qdbp_object_ptr[1]){0};                      \
    capture[0] =                                                            \
        _qdbp_make_object(QDBP_INT, (union qdbp_object_data){.i = result}); \
    _qdbp_drop(a_val, 1);                                                   \
    _qdbp_drop(b_val, 1);                                                   \
    return _qdbp_replace(a, VAL, (void *)_qdbp_get_capture, capture, 1);    \
  }

#define MK_PROTO_CMP_OP(name, op)                                          \
  static qdbp_object_ptr name(qdbp_object_arr captures, qdbp_object_ptr a, \
                              qdbp_object_ptr b) {                         \
    qdbp_object_ptr a_val = _qdbp_invoke_1(a, VAL, a);                     \
    qdbp_object_ptr b_val = _qdbp_invoke_1(b, VAL, b);                     \
    assert_obj_kind(a_val, QDBP_INT);                                      \
    assert_obj_kind(b_val, QDBP_INT);                                      \
    bool result = a_val->data.i op b_val->data.i;                          \
    _qdbp_drop(a_val, 1);                                                  \
    _qdbp_drop(b_val, 1);                                                  \
    return result ? _qdbp_true() : _qdbp_false();                          \
  }

MK_PROTO_ARITH_OP(qdbp_add, +)
MK_PROTO_ARITH_OP(qdbp_sub, -)
MK_PROTO_ARITH_OP(qdbp_mul, *)
MK_PROTO_ARITH_OP(qdbp_div, /)
MK_PROTO_ARITH_OP(qdbp_mod, %)
MK_PROTO_CMP_OP(qdbp_eq, ==)
MK_PROTO_CMP_OP(qdbp_neq, !=)
MK_PROTO_CMP_OP(qdbp_lt, <)
MK_PROTO_CMP_OP(qdbp_gt, >)
MK_PROTO_CMP_OP(qdbp_gte, >=)
MK_PROTO_CMP_OP(qdbp_lte, <=)

qdbp_object_ptr _qdbp_make_int_proto(int64_t value) {
  qdbp_object_ptr proto = _qdbp_empty_prototype();
  qdbp_object_ptr capture =
      _qdbp_make_object(QDBP_INT, (union qdbp_object_data){.i = value});
  proto = _qdbp_extend(proto, VAL, (void *)_qdbp_get_capture,
                       (qdbp_object_ptr[1]){capture}, 1, 16);
  proto = _qdbp_extend(proto, ADD, (void *)qdbp_add, NULL, 0, 16);
  proto = _qdbp_extend(proto, SUB, (void *)qdbp_sub, NULL, 0, 16);
  proto = _qdbp_extend(proto, MUL, (void *)qdbp_mul, NULL, 0, 16);
  proto = _qdbp_extend(proto, DIV, (void *)qdbp_div, NULL, 0, 16);
  proto = _qdbp_extend(proto, MOD, (void *)qdbp_mod, NULL, 0, 16);
  proto = _qdbp_extend(proto, EQ, (void *)qdbp_eq, NULL, 0, 16);
  proto = _qdbp_extend(proto, NEQ, (void *)qdbp_neq, NULL, 0, 16);
  proto = _qdbp_extend(proto, LT, (void *)qdbp_lt, NULL, 0, 16);
  proto = _qdbp_extend(proto, GT, (void *)qdbp_gt, NULL, 0, 16);
  proto = _qdbp_extend(proto, GEQ, (void *)qdbp_gte, NULL, 0, 16);
  proto = _qdbp_extend(proto, LEQ, (void *)qdbp_lte, NULL, 0, 16);
  proto = _qdbp_extend(proto, PRINT, (void *)_qdbp_print_intproto, NULL, 0, 16);
  return proto;
}

#endif
// mem.c
#ifdef QDBP_RUNTIME_H
// Keep track of mallocs and frees
struct _qdbp_malloc_list_node {
  void *ptr;
  struct _qdbp_malloc_list_node *next;
  const char *msg;
};
static struct _qdbp_malloc_list_node *_qdbp_malloc_list = NULL;

bool _qdbp_malloc_list_contains(void *ptr) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(!_qdbp_is_unboxed_int((qdbp_object_ptr)ptr));
  }
  struct _qdbp_malloc_list_node *node = _qdbp_malloc_list;
  while (node) {
    if (node->ptr == ptr) {
      return true;
    }
    node = node->next;
  }
  return false;
}
void _qdbp_add_to_malloc_list(void *ptr, const char *msg) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(!_qdbp_is_unboxed_int((qdbp_object_ptr)ptr));
  }
  assert(!_qdbp_malloc_list_contains(ptr));
  struct _qdbp_malloc_list_node *new_node =
      (struct _qdbp_malloc_list_node *)malloc(
          sizeof(struct _qdbp_malloc_list_node));
  new_node->ptr = ptr;
  new_node->next = _qdbp_malloc_list;
  new_node->msg = msg;
  _qdbp_malloc_list = new_node;
}
void _qdbp_remove_from_malloc_list(void *ptr) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(!_qdbp_is_unboxed_int((qdbp_object_ptr)ptr));
  }
  assert(_qdbp_malloc_list_contains(ptr));
  struct _qdbp_malloc_list_node *node = _qdbp_malloc_list;
  struct _qdbp_malloc_list_node *prev = NULL;
  while (node) {
    if (node->ptr == ptr) {
      if (prev) {
        prev->next = node->next;
      } else {
        _qdbp_malloc_list = node->next;
      }
      free(node);
      return;
    }
    prev = node;
    node = node->next;
  }
  assert(false);
}
void *_qdbp_malloc(size_t size, const char *msg) {
  if (size == 0) {
    return NULL;
  }
  void *ptr;
  ptr = malloc(size);
  if (_QDBP_CHECK_MALLOC_FREE) {
    _qdbp_add_to_malloc_list(ptr, msg);
  }
  assert(ptr);
  return ptr;
}
void _qdbp_free(void *ptr) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(!_qdbp_is_unboxed_int((qdbp_object_ptr)ptr));
  }
  if (_QDBP_CHECK_MALLOC_FREE) {
    _qdbp_remove_from_malloc_list(ptr);
  }
  free(ptr);
}

#define MK_FREELIST(type, name)                       \
  struct name##_t {                                   \
    type objects[_QDBP_FREELIST_SIZE];                \
    size_t idx;                                       \
  };                                                  \
  struct name##_t name = {.idx = 0, .objects = {0}};  \
                                                      \
  static type pop_##name() {                          \
    if (name.idx == 0) {                              \
      assert(false);                                  \
    }                                                 \
    name.idx--;                                       \
    return name.objects[name.idx];                    \
  }                                                   \
  static bool push_##name(type obj) {                 \
    if (name.idx == _QDBP_FREELIST_SIZE) {            \
      return false;                                   \
    }                                                 \
    name.objects[name.idx] = obj;                     \
    name.idx++;                                       \
    return true;                                      \
  }                                                   \
  static void name##_remove_all_from_malloc_list() {  \
    for (size_t i = 0; i < name.idx; i++) {           \
      _qdbp_remove_from_malloc_list(name.objects[i]); \
    }                                                 \
  }
void _qdbp_free_fields(qdbp_prototype_ptr proto) {
  struct _qdbp_field *cur_field;

  qdbp_field_ptr field;
  size_t tmp;
  HT_ITER(proto->labels, field, tmp) { _qdbp_del_method(&field->method); }
  _qdbp_del_ht(proto->labels);
}

MK_FREELIST(qdbp_object_ptr, freelist)

void _qdbp_free_obj(qdbp_object_ptr obj) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(!_qdbp_is_unboxed_int(obj));
  }
  if (_QDBP_OBJ_FREELIST && push_freelist(obj)) {
  } else {
    _qdbp_free((void *)obj);
  }
}

qdbp_object_ptr _qdbp_malloc_obj() {
  if (_QDBP_OBJ_FREELIST && freelist.idx > 0) {
    return pop_freelist();
  } else {
    return (qdbp_object_ptr)_qdbp_malloc(sizeof(struct _qdbp_object), "field");
  }
}
MK_FREELIST(struct _boxed_int *, box_freelist)
struct _boxed_int *_qdbp_malloc_boxed_int() {
  if (_QDBP_BOX_FREELIST && box_freelist.idx > 0) {
    return pop_box_freelist();
  } else {
    return _qdbp_malloc(sizeof(struct _boxed_int), "box");
  }
}
void _qdbp_free_boxed_int(struct _boxed_int *i) {
  if (_QDBP_BOX_FREELIST && push_box_freelist(i)) {
  } else {
    _qdbp_free(i);
  }
}
void _qdbp_init() {
  for (int i = 0; i < _QDBP_FREELIST_SIZE; i++) {
    if (!_QDBP_CHECK_MALLOC_FREE) {
      push_freelist(_qdbp_malloc(sizeof(struct _qdbp_object), "obj init"));
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
      freelist_remove_all_from_malloc_list();
    }
    if (_QDBP_BOX_FREELIST) {
      box_freelist_remove_all_from_malloc_list();
    }
    struct _qdbp_malloc_list_node *node = _qdbp_malloc_list;
    while (node) {
      printf("Error: %p(%s) was malloc'd but not freed\n", node->ptr,
             node->msg);
      node = node->next;
    }
  }
}
void _qdbp_del_prototype(qdbp_prototype_ptr proto) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(proto);
  }
  qdbp_field_ptr *PValue;

  _qdbp_free_fields(proto);
}

void _qdbp_del_variant(qdbp_variant_ptr variant) {
  assert(variant);
  _qdbp_drop(variant->value, 1);
}

qdbp_object_arr _qdbp_make_capture_arr(size_t size) {
  qdbp_object_arr capture;
  return (qdbp_object_ptr *)_qdbp_malloc(sizeof(qdbp_object_ptr) * size,
                                         "captures");
}
void _qdbp_free_capture_arr(qdbp_object_arr arr, size_t size) {
  if (arr) {
    _qdbp_free(arr);
  }
}
void _qdbp_del_method(qdbp_method_ptr method) {
  assert(method);
  for (size_t i = 0; i < method->captures_size; i++) {
    _qdbp_drop((method->captures[i]), 1);
  }
  if (method->captures_size > 0) {
    _qdbp_free_capture_arr(method->captures, method->captures_size);
  }
}

void _qdbp_del_obj(qdbp_object_ptr obj) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(!_qdbp_is_unboxed_int(obj));
  }
  switch (_qdbp_get_kind(obj)) {
    case QDBP_INT:
      break;
    case QDBP_FLOAT:
      break;
    case QDBP_BOXED_INT:
      _qdbp_del_prototype(&(obj->data.boxed_int->other_labels));
      _qdbp_free(obj->data.boxed_int);
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

void _qdbp_duplicate_labels(qdbp_prototype_ptr src, qdbp_prototype_ptr dest) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(!dest->labels);
  }
  dest->labels = _qdbp_ht_duplicate(src->labels);
}

#endif
// proto.c
#ifdef QDBP_RUNTIME_H
void _qdbp_label_add(qdbp_prototype_ptr proto, _qdbp_label_t label,
                     qdbp_field_ptr field, size_t defaultsize) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(proto);
    assert(field);
    // check that defaultsize is a pwr of 2
    assert((defaultsize & (defaultsize - 1)) == 0);
  }
  if (!proto->labels) {
    proto->labels = _qdbp_new_ht(defaultsize);
  }
  proto->labels = _qdbp_ht_insert(proto->labels, field);
}
qdbp_field_ptr _qdbp_label_get(qdbp_prototype_ptr proto, _qdbp_label_t label) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(proto);
    assert(proto->labels);
  }
  return _qdbp_ht_find(proto->labels, label);
}

void _qdbp_copy_captures_except(qdbp_prototype_ptr new_prototype,
                                _qdbp_label_t except) {
  // Copy all the capture arrays except the new one
  qdbp_field_ptr field;
  size_t tmp;
  HT_ITER(new_prototype->labels, field, tmp) {
    if (field->label != except) {
      qdbp_object_arr original = field->method.captures;
      field->method.captures =
          _qdbp_make_capture_arr(field->method.captures_size);
      _qdbp_memcpy(field->method.captures, original,
                   sizeof(qdbp_object_ptr) * field->method.captures_size);
    }
  }
}

static struct _qdbp_prototype _qdbp_raw_prototype_replace(
    const qdbp_prototype_ptr src, const qdbp_field_ptr new_field,
    _qdbp_label_t new_label) {
  size_t src_size = _qdbp_proto_size(src);
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(src_size);
    assert(new_field->label == new_label);
  }
  struct _qdbp_prototype new_prototype = {.labels = NULL};

  // FIXME: Have the overwriting done in duplicate_labels
  // to avoid extra work of inserting twice
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(new_prototype.labels == NULL);
  }
  _qdbp_duplicate_labels(src, &new_prototype);

  *(_qdbp_ht_find(new_prototype.labels, new_label)) = *new_field;
  _qdbp_copy_captures_except(&new_prototype, new_label);
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(new_prototype.labels);
  }
  return new_prototype;
}

static struct _qdbp_prototype _qdbp_raw_prototype_extend(
    const qdbp_prototype_ptr src, const qdbp_field_ptr new_field,
    size_t new_label, size_t defaultsize) {
  // copy src
  size_t src_size = _qdbp_proto_size(src);
  struct _qdbp_prototype new_prototype = {.labels = NULL};

  _qdbp_duplicate_labels(src, &new_prototype);
  _qdbp_label_add(&new_prototype, new_label, new_field, defaultsize);
  _qdbp_copy_captures_except(&new_prototype, new_label);
  return new_prototype;
}

qdbp_object_arr _qdbp_get_method(qdbp_object_ptr obj, _qdbp_label_t label,
                                 void **code_ptr /*output param*/) {
  assert_obj_kind(obj, QDBP_PROTOTYPE);
  struct _qdbp_method m =
      _qdbp_label_get(&(obj->data.prototype), label)->method;
  _qdbp_dup_captures(&m);
  *code_ptr = m.code;
  qdbp_object_arr ret = m.captures;
  return ret;
}

qdbp_object_arr _qdbp_make_captures(qdbp_object_arr captures, size_t size) {
  if (size == 0) {
    return NULL;
  } else {
    qdbp_object_arr out = (qdbp_object_arr)_qdbp_malloc(
        sizeof(struct _qdbp_object *) * size, "captures");
    _qdbp_memcpy(out, captures, sizeof(struct _qdbp_object *) * size);
    return out;
  }
}

qdbp_object_ptr _qdbp_extend(qdbp_object_ptr obj, _qdbp_label_t label,
                             void *code, qdbp_object_arr captures,
                             size_t captures_size, size_t defaultsize) {
  if (!obj) {
    obj = _qdbp_make_object(
        QDBP_PROTOTYPE,
        (union qdbp_object_data){
            .prototype = {.labels = _qdbp_new_ht(defaultsize)}});
  } else if (_qdbp_is_unboxed_int(obj)) {
    return _qdbp_box(obj, label, code, captures, captures_size);
  } else if (_qdbp_is_boxed_int(obj)) {
    return _qdbp_box_extend(obj, label, code, captures, captures_size);
  }
  struct _qdbp_field f = {
      .label = label,
      .method = {.captures = _qdbp_make_captures(captures, captures_size),
                 .captures_size = captures_size,
                 .code = code}};
  assert_obj_kind(obj, QDBP_PROTOTYPE);
  if (!_QDBP_REUSE_ANALYSIS || !_qdbp_is_unique(obj)) {
    qdbp_prototype_ptr prototype = &(obj->data.prototype);
    struct _qdbp_prototype new_prototype =
        _qdbp_raw_prototype_extend(prototype, &f, label, defaultsize);
    qdbp_object_ptr new_obj = _qdbp_make_object(
        QDBP_PROTOTYPE, (union qdbp_object_data){.prototype = new_prototype});
    _qdbp_dup_prototype_captures(prototype);
    _qdbp_drop(obj, 1);
    return new_obj;
  } else {
    _qdbp_label_add(&(obj->data.prototype), label, &f, defaultsize);
    return obj;
  }
}
qdbp_object_ptr _qdbp_replace(qdbp_object_ptr obj, _qdbp_label_t label,
                              void *code, qdbp_object_arr captures,
                              size_t captures_size) {
  if (!obj) {
    obj = _qdbp_make_object(QDBP_PROTOTYPE, (union qdbp_object_data){
                                                .prototype = {.labels = NULL}});
  } else if (_qdbp_is_unboxed_int(obj)) {
    obj = _qdbp_make_int_proto(_qdbp_get_unboxed_int(obj));
    assert_obj_kind(obj, QDBP_PROTOTYPE);
  } else if (_qdbp_is_boxed_int(obj) && label < MAX_OP) {
    int64_t i = _qdbp_get_boxed_int(obj);
    _qdbp_drop(obj, 1);
    obj = _qdbp_make_int_proto(i);
    assert_obj_kind(obj, QDBP_PROTOTYPE);
  } else if (_qdbp_is_boxed_int(obj) && label >= MAX_OP) {
    return _qdbp_boxed_int_replace(obj, label, code, captures, captures_size);
  }

  assert_obj_kind(obj, QDBP_PROTOTYPE);
  struct _qdbp_field f = {
      .label = label,
      .method = {.captures = _qdbp_make_captures(captures, captures_size),
                 .captures_size = captures_size,
                 .code = code}};
  if (!_QDBP_REUSE_ANALYSIS || !_qdbp_is_unique(obj)) {
    struct _qdbp_prototype new_prototype =
        _qdbp_raw_prototype_replace(&(obj->data.prototype), &f, label);
    _qdbp_dup_prototype_captures_except(&(obj->data.prototype), label);

    qdbp_object_ptr new_obj = _qdbp_make_object(
        QDBP_PROTOTYPE, (union qdbp_object_data){.prototype = new_prototype});

    _qdbp_drop(obj, 1);
    return new_obj;
  } else {
    // find the index to replace
    qdbp_field_ptr field = _qdbp_label_get(&(obj->data.prototype), label);
    assert(field->label == label);
    // reuse the field
    _qdbp_del_method(&(field->method));
    field->label = f.label;
    field->method = f.method;
    return obj;
  }
}

size_t _qdbp_proto_size(qdbp_prototype_ptr proto) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(proto);
  }
  return _qdbp_hashtable_size(proto->labels);
}
qdbp_object_ptr _qdbp_invoke_1(qdbp_object_ptr receiver, _qdbp_label_t label,
                               qdbp_object_ptr arg0) {
  if (_qdbp_is_unboxed_int(arg0)) {
    return _qdbp_unboxed_unary_op(arg0, label);
  } else if (_qdbp_is_boxed_int(arg0)) {
    if (label < ADD) {
      return _qdbp_boxed_unary_op(arg0, label);
    } else {
      qdbp_field_ptr field =
          _qdbp_label_get(&(arg0->data.boxed_int->other_labels), label);
      _qdbp_dup_captures(&(field->method));
      return ((qdbp_object_ptr(*)(qdbp_object_arr,
                                  qdbp_object_ptr))field->method.code)(
          field->method.captures, arg0);
    }
  } else {
    void *code;
    qdbp_object_arr captures = _qdbp_get_method(receiver, label, &code);
    return ((qdbp_object_ptr(*)(qdbp_object_arr, qdbp_object_ptr))code)(
        captures, arg0);
  }
}

qdbp_object_ptr _qdbp_invoke_2(qdbp_object_ptr receiver, _qdbp_label_t label,
                               qdbp_object_ptr arg0, qdbp_object_ptr arg1) {
  // UB UB
  if (_qdbp_is_unboxed_int(arg0) && _qdbp_is_unboxed_int(arg1)) {
    return _qdbp_unboxed_binary_op(_qdbp_get_unboxed_int(arg0),
                                   _qdbp_get_unboxed_int(arg1), label);
  }
  // UB B
  else if (_qdbp_is_unboxed_int(arg0) && _qdbp_is_boxed_int(arg1)) {
    qdbp_object_ptr result = _qdbp_unboxed_binary_op(
        _qdbp_get_unboxed_int(arg0), _qdbp_get_boxed_int(arg1), label);
    _qdbp_drop(arg1, 1);
    return result;
  }
  // UB BB
  else if (_qdbp_is_unboxed_int(arg0) &&
           _qdbp_get_kind(arg1) == QDBP_PROTOTYPE) {
    int64_t a = _qdbp_get_unboxed_int(arg0);
    arg1 = _qdbp_invoke_1(arg1, VAL, arg1);
    if (_QDBP_DYNAMIC_TYPECHECK) {
      assert(_qdbp_get_kind(arg1) == QDBP_INT);
    }
    int64_t b = arg1->data.i;
    _qdbp_drop(arg1, 1);
    return _qdbp_unboxed_binary_op(a, b, label);
  }
  // B UB
  // B B
  // B BB
  else if (_qdbp_is_boxed_int(arg0)) {
    if (label < MAX_OP) {
      // print label
      return _qdbp_boxed_binary_op(arg0, arg1, label);
    } else {
      // get the method from the binary op
      qdbp_field_ptr field =
          _qdbp_label_get(&arg0->data.boxed_int->other_labels, label);
      qdbp_object_ptr (*code)(qdbp_object_arr, qdbp_object_ptr,
                              qdbp_object_ptr) =
          ((qdbp_object_ptr(*)(qdbp_object_arr, qdbp_object_ptr,
                               qdbp_object_ptr))field->method.code);
      qdbp_object_arr captures = field->method.captures;
      return code(captures, arg0, arg1);
    }
  }
  // BB UB
  // BB B

  else {
    if (_QDBP_DYNAMIC_TYPECHECK) {
      assert(_qdbp_get_kind(arg0) == QDBP_PROTOTYPE);
    }
    void *code;
    qdbp_object_arr captures = _qdbp_get_method(receiver, label, &code);
    return ((qdbp_object_ptr(*)(qdbp_object_arr, qdbp_object_ptr,
                                qdbp_object_ptr))code)(captures, arg0, arg1);
  }
}

#endif
// refcount.c
#ifdef QDBP_RUNTIME_H
void _qdbp_incref(qdbp_object_ptr obj, _qdbp_refcount_t amount) {
  if (!_QDBP_REFCOUNT) {
    return;
  }
  if (_QDBP_DYNAMIC_TYPECHECK &&
      (_qdbp_is_unboxed_int(obj) || obj == _qdbp_true() ||
       obj == _qdbp_false() || !obj)) {
    assert(false);
  }
  obj->metadata.rc += amount;
}
void _qdbp_decref(qdbp_object_ptr obj, _qdbp_refcount_t amount) {
  if (!_QDBP_REFCOUNT) {
    return;
  }
  if (_QDBP_DYNAMIC_TYPECHECK &&
      (_qdbp_is_unboxed_int(obj) || obj == _qdbp_true() ||
       obj == _qdbp_false() || !obj)) {
    assert(false);
  }
  obj->metadata.rc -= amount;
}
void _qdbp_set_refcount(qdbp_object_ptr obj, _qdbp_refcount_t refcount) {
  if (!_QDBP_REFCOUNT) {
    return;
  }
  if (_QDBP_DYNAMIC_TYPECHECK &&
      (_qdbp_is_unboxed_int(obj) || obj == _qdbp_true() ||
       obj == _qdbp_false() || !obj)) {
    assert(false);
  }
  obj->metadata.rc = refcount;
}
_qdbp_refcount_t _qdbp_get_refcount(qdbp_object_ptr obj) {
  if (!_QDBP_REFCOUNT) {
    return 100;
  }
  if (_QDBP_DYNAMIC_TYPECHECK &&
      (_qdbp_is_unboxed_int(obj) || obj == _qdbp_true() ||
       obj == _qdbp_false() || !obj)) {
    assert(false);
  }
  return obj->metadata.rc;
}

void _qdbp_drop(qdbp_object_ptr obj, _qdbp_refcount_t cnt) {
  if (!_QDBP_REFCOUNT) {
    return;
  }
  if (!obj || _qdbp_is_unboxed_int(obj) || obj == _qdbp_true() ||
      obj == _qdbp_false()) {
    return;
  } else {
    assert_refcount(obj);
    if (_QDBP_VERIFY_REFCOUNTS) {
      assert(_qdbp_get_refcount(obj) >= cnt);
    }
    _qdbp_decref(obj, cnt);
    if (_qdbp_get_refcount(obj) == 0) {
      _qdbp_del_obj(obj);
      return;
    }
    return;
  }
}

void _qdbp_obj_dup(qdbp_object_ptr obj, _qdbp_refcount_t cnt) {
  if (!_QDBP_REFCOUNT) {
    return;
  }
  if (!obj || _qdbp_is_unboxed_int(obj) || obj == _qdbp_true() ||
      obj == _qdbp_false()) {
    return;
  } else {
    assert_refcount(obj);
    _qdbp_incref(obj, cnt);
  }
}

void _qdbp_dup_captures(qdbp_method_ptr method) {
  if (!_QDBP_REFCOUNT) {
    return;
  }
  for (size_t i = 0; i < method->captures_size; i++) {
    _qdbp_obj_dup((method->captures[i]), 1);
  }
}

void _qdbp_dup_prototype_captures(qdbp_prototype_ptr proto) {
  if (!_QDBP_REFCOUNT) {
    return;
  }
  qdbp_field_ptr field;
  size_t tmp;
  HT_ITER(proto->labels, field, tmp) { _qdbp_dup_captures(&(field->method)); }
}
void _qdbp_dup_prototype_captures_except(qdbp_prototype_ptr proto,
                                         _qdbp_label_t except) {
  if (!_QDBP_REFCOUNT) {
    return;
  }
  qdbp_field_ptr field;
  size_t tmp;
  HT_ITER(proto->labels, field, tmp) {
    if (field->label != except) {
      _qdbp_dup_captures(&(field->method));
    }
  }
}

bool _qdbp_is_unique(qdbp_object_ptr obj) {
  if (!_QDBP_REFCOUNT) {
    return false;
  }
  if (_qdbp_is_unboxed_int(obj) || obj == _qdbp_true() ||
      obj == _qdbp_false() || !obj) {
    return true;
  } else {
    if (_QDBP_VERIFY_REFCOUNTS) {
      assert(obj);
      assert(_qdbp_get_refcount(obj) >= 0);
    }
    return _qdbp_get_refcount(obj) == 1;
  }
}

#endif
// tag.c
#ifdef QDBP_RUNTIME_H
enum qdbp_object_kind _qdbp_get_kind(qdbp_object_ptr obj) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(!_qdbp_is_unboxed_int(obj));
    assert(obj != _qdbp_true());
    assert(obj != _qdbp_false());
    assert(obj->metadata.tag != QDBP_VARIANT);
  }
  if (obj->metadata.tag > QDBP_VARIANT) {
    return QDBP_VARIANT;
  } else {
    return obj->metadata.tag;
  }
}

void _qdbp_set_tag(qdbp_object_ptr o, _qdbp_tag_t t) { o->metadata.tag = t; }
_qdbp_tag_t _qdbp_get_tag(qdbp_object_ptr o) { return o->metadata.tag; }
qdbp_object_ptr _qdbp_variant_create(_qdbp_tag_t tag, qdbp_object_ptr value) {
  if (tag == 21 && value == NULL) {
    // must keep up to date with basic_objs and namesToInts
    // and the function below
    return _qdbp_true();
  } else if (tag == 20 && value == NULL) {
    return _qdbp_false();
  }
  assert_refcount(value);
  qdbp_object_ptr new_obj = _qdbp_make_object(
      tag, (union qdbp_object_data){.variant = {.value = value}});
  return new_obj;
}
/* Variant Pattern Matching
  - dup the variant payload
  - drop the variant
  - execute the variant stuff
*/
void _qdbp_decompose_variant(qdbp_object_ptr obj, _qdbp_tag_t *tag,
                             qdbp_object_ptr *payload) {
  // must keep up to date with basic_objs and namesToInts
  // and the function above
  if (obj == _qdbp_true()) {
    *tag = 21;
    *payload = NULL;
    return;
  } else if (obj == _qdbp_false()) {
    *tag = 20;
    *payload = NULL;
    return;
  }
  assert_obj_kind(obj, QDBP_VARIANT);
  qdbp_object_ptr value = obj->data.variant.value;
  *tag = _qdbp_get_tag(obj);
  // return value, tag
  *payload = value;
}

#endif
// unboxed_int.c
#ifdef QDBP_RUNTIME_H
bool _qdbp_is_unboxed_int(qdbp_object_ptr obj) {
  return ((uintptr_t)obj & 1) == 1;
}

bool _qdbp_is_boxed_int(qdbp_object_ptr obj) {
  return !_qdbp_is_unboxed_int(obj) && _qdbp_get_kind(obj) == QDBP_BOXED_INT;
}
qdbp_object_ptr _qdbp_make_unboxed_int(int64_t value) {
  return (qdbp_object_ptr)((intptr_t)(value << 1) | 1);
}

int64_t _qdbp_get_unboxed_int(qdbp_object_ptr obj) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(_qdbp_is_unboxed_int(obj));
  }
  return ((intptr_t)obj) >> 1;
}
int64_t _qdbp_get_boxed_int(qdbp_object_ptr obj) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(_qdbp_is_boxed_int(obj));
  }
  return obj->data.boxed_int->value;
}

int64_t _qdbp_unbox_int(qdbp_object_ptr obj) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(_qdbp_is_boxed_int(obj));
  }
  return obj->data.boxed_int->value;
}

qdbp_object_ptr _qdbp_unboxed_unary_op(qdbp_object_ptr obj, _qdbp_label_t op) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(_qdbp_is_unboxed_int(obj));
    assert(op < MAX_OP);
  }
  int64_t i = _qdbp_get_unboxed_int(obj);
  switch (op) {
    case VAL:
      return _qdbp_int(i);
      break;
    case PRINT:
      printf("%lld\n", i);
      return _qdbp_empty_prototype();
      break;
    default:
      printf("unboxed_unary_op: %u\n", op);
      assert(false);
      __builtin_unreachable();
  }
}
#define MK_ARITH_SWITCH \
  MK_ARITH_CASE(ADD, +) \
  MK_ARITH_CASE(SUB, -) \
  MK_ARITH_CASE(MUL, *) \
  MK_ARITH_CASE(DIV, /) \
  MK_ARITH_CASE(MOD, %)
#define MK_CMP_SWITCH  \
  MK_CMP_CASE(EQ, ==)  \
  MK_CMP_CASE(NEQ, !=) \
  MK_CMP_CASE(LT, <)   \
  MK_CMP_CASE(GT, >)   \
  MK_CMP_CASE(LEQ, <=) \
  MK_CMP_CASE(GEQ, >=)
#define MK_SWITCH \
  MK_ARITH_SWITCH \
  MK_CMP_SWITCH

qdbp_object_ptr _qdbp_unboxed_binary_op(int64_t a, int64_t b,
                                        _qdbp_label_t op) {
  switch (op) {
#define MK_ARITH_CASE(n, op) \
  case n:                    \
    return _qdbp_make_unboxed_int(a op b);
#define MK_CMP_CASE(n, op) \
  case n:                  \
    return a op b ? _qdbp_true() : _qdbp_false();
    MK_SWITCH
#undef MK_ARITH_CASE
#undef MK_CMP_CASE
    default:
      assert(false);
      __builtin_unreachable();
  }
}

static qdbp_object_ptr _qdbp_boxed_binary_arith_op(qdbp_object_ptr a,
                                                   qdbp_object_ptr b,
                                                   _qdbp_label_t op) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(_qdbp_is_boxed_int(a));
    assert(op < EQ);
  }
  int64_t a_int = _qdbp_get_boxed_int(a);
  int64_t b_int;
  if (_qdbp_is_unboxed_int(b)) {
    b_int = _qdbp_get_unboxed_int(b);
    _qdbp_drop(b, 1);
  } else if (_qdbp_is_boxed_int(b)) {
    b_int = b->data.boxed_int->value;
    _qdbp_drop(b, 1);
  } else {
    qdbp_object_ptr b_val = _qdbp_invoke_1(b, VAL, b);
    if (_QDBP_DYNAMIC_TYPECHECK) {
      assert(_qdbp_get_kind(b_val) == QDBP_INT);
    }
    b_int = b_val->data.i;
    _qdbp_drop(b_val, 1);
  }
  qdbp_object_ptr ret;
  if (_qdbp_is_unique(a) && _QDBP_REUSE_ANALYSIS) {
    ret = a;
  } else {
    ret = _qdbp_malloc_obj();
    _qdbp_set_refcount(ret, 1);
    _qdbp_set_tag(ret, QDBP_BOXED_INT);
    ret->data.boxed_int = _qdbp_malloc_boxed_int();
    ret->data.boxed_int->other_labels.labels = NULL;
    qdbp_field_ptr *PValue;
    _qdbp_duplicate_labels(&a->data.boxed_int->other_labels,
                           &ret->data.boxed_int->other_labels);
    _qdbp_drop(a, 1);
  }
#define MK_ARITH_CASE(n, op)                     \
  case n:                                        \
    ret->data.boxed_int->value = a_int op b_int; \
    break;

  switch (op) {
    MK_ARITH_SWITCH
    default:
      printf("boxed_binary_arith_op: %u\n", op);
      assert(false);
      __builtin_unreachable();
  }
  // Ensure that all math is done in 63 bits
  ret->data.boxed_int->value <<= 1;
  ret->data.boxed_int->value >>= 1;
  return ret;
#undef MK_ARITH_CASE
}

static qdbp_object_ptr _qdbp_boxed_binary_cmp_op(qdbp_object_ptr a,
                                                 qdbp_object_ptr b,
                                                 _qdbp_label_t op) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(_qdbp_is_boxed_int(a));
    assert(op >= EQ && op < MAX_OP);
  }
  int64_t a_int = _qdbp_get_boxed_int(a);
  int64_t b_int;
  if (_qdbp_is_unboxed_int(b)) {
    b_int = _qdbp_get_unboxed_int(b);
    _qdbp_drop(b, 1);
  } else if (_qdbp_is_boxed_int(b)) {
    b_int = b->data.boxed_int->value;
    _qdbp_drop(b, 1);
  } else {
    qdbp_object_ptr b_val = _qdbp_invoke_1(b, VAL, b);
    if (_QDBP_DYNAMIC_TYPECHECK) {
      assert(_qdbp_get_kind(b_val) == QDBP_INT);
    }
    b_int = b_val->data.i;
    _qdbp_drop(b_val, 1);
  }
  _qdbp_drop(a, 1);
#define MK_CMP_CASE(n, op) \
  case n:                  \
    return a_int op b_int ? _qdbp_true() : _qdbp_false();
  switch (op) {
    MK_CMP_SWITCH
    default:
      assert(false);
      __builtin_unreachable();
  }
#undef MK_CMP_CASE
}
qdbp_object_ptr _qdbp_boxed_binary_op(qdbp_object_ptr a, qdbp_object_ptr b,
                                      _qdbp_label_t op) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(_qdbp_is_boxed_int(a));
    assert(op < MAX_OP);
  }
  if (op < EQ) {
    return _qdbp_boxed_binary_arith_op(a, b, op);
  } else {
    return _qdbp_boxed_binary_cmp_op(a, b, op);
  }
}

qdbp_object_ptr _qdbp_box(qdbp_object_ptr obj, _qdbp_label_t label, void *code,
                          qdbp_object_arr captures, size_t captures_size) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(_qdbp_is_unboxed_int(obj));
  }
  int64_t value = _qdbp_get_unboxed_int(obj);
  qdbp_object_ptr ret = _qdbp_malloc_obj();
  _qdbp_set_refcount(ret, 1);
  _qdbp_set_tag(ret, QDBP_BOXED_INT);
  ret->data.boxed_int = _qdbp_malloc_boxed_int();
  ret->data.boxed_int->value = value;
  ret->data.boxed_int->other_labels.labels = NULL;
  struct _qdbp_field field = {0};
  field.method.code = code;
  field.method.captures = _qdbp_make_captures(captures, captures_size);
  field.method.captures_size = captures_size;
  field.label = label;
  _qdbp_label_add(&(ret->data.boxed_int->other_labels), label, &field, 1);
  return ret;
}

qdbp_object_ptr _qdbp_box_extend(qdbp_object_ptr obj, _qdbp_label_t label,
                                 void *code, qdbp_object_arr captures,
                                 size_t captures_size) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(_qdbp_is_boxed_int(obj));
  }
  if (_qdbp_is_unique(obj) && _QDBP_REUSE_ANALYSIS) {
    struct _qdbp_field field;
    field.method.code = code;
    field.method.captures = _qdbp_make_captures(captures, captures_size);
    field.method.captures_size = captures_size;
    field.label = label;
    _qdbp_label_add(&obj->data.boxed_int->other_labels, label, &field, 1);
    return obj;
  } else {
    qdbp_object_ptr ret = _qdbp_malloc_obj();
    _qdbp_set_refcount(ret, 1);
    _qdbp_set_tag(ret, QDBP_BOXED_INT);
    ret->data.boxed_int = _qdbp_malloc_boxed_int();
    ret->data.boxed_int->value = obj->data.boxed_int->value;
    ret->data.boxed_int->other_labels.labels = NULL;
    _qdbp_duplicate_labels(&obj->data.boxed_int->other_labels,
                           &ret->data.boxed_int->other_labels);
    struct _qdbp_field field;
    field.method.code = code;
    field.method.captures = _qdbp_make_captures(captures, captures_size);
    field.method.captures_size = captures_size;
    field.label = label;
    _qdbp_label_add(&ret->data.boxed_int->other_labels, label, &field, 1);
    _qdbp_drop(obj, 1);
    return ret;
  }
}

qdbp_object_ptr _qdbp_boxed_int_replace(qdbp_object_ptr obj,
                                        _qdbp_label_t label, void *code,
                                        qdbp_object_arr captures,
                                        size_t captures_size) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(_qdbp_is_boxed_int(obj));
  }
  if (_qdbp_is_unique(obj) && _QDBP_REUSE_ANALYSIS) {
    qdbp_field_ptr field = _qdbp_label_get(&(obj->data.prototype), label);
    _qdbp_del_method(&(field->method));
    field->method = (struct _qdbp_method){
        .code = code, .captures = captures, .captures_size = captures_size};
    return obj;
  } else {
    qdbp_object_ptr ret = _qdbp_malloc_obj();
    _qdbp_set_refcount(ret, 1);
    _qdbp_set_tag(ret, QDBP_BOXED_INT);
    ret->data.boxed_int = _qdbp_malloc_boxed_int();
    ret->data.boxed_int->value = obj->data.boxed_int->value;
    ret->data.boxed_int->other_labels.labels = NULL;
    _qdbp_duplicate_labels(&obj->data.boxed_int->other_labels,
                           &ret->data.boxed_int->other_labels);
    struct _qdbp_field field;
    field.method.code = code;
    field.method.captures = captures;
    field.method.captures_size = captures_size;
    field.label = label;
    _qdbp_label_add(&ret->data.boxed_int->other_labels, label, &field, 1);
    _qdbp_drop(obj, 1);
    return ret;
  }
}

qdbp_object_ptr _qdbp_boxed_unary_op(qdbp_object_ptr arg0,
                                     _qdbp_label_t label) {
  if (_QDBP_DYNAMIC_TYPECHECK) {
    assert(_qdbp_is_boxed_int(arg0));
  }
  int64_t i = arg0->data.boxed_int->value;
  _qdbp_drop(arg0, 1);
  switch (label) {
    case PRINT:
      printf("%lld\n", i);
      return _qdbp_empty_prototype();
      break;
    case VAL:
      return _qdbp_int(i);
      break;
    default:
      assert(false);
      __builtin_unreachable();
  }
}

#endif
#endif