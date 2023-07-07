// This file gets included first in every qdbp progrram

#ifndef QDBP_RUNTIME_H
#define QDBP_RUNTIME_H
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
// config
// You could also add fsanitize=undefined
// Also could adjust inlining
#ifdef QDBP_DEBUG
const bool _QDBP_REFCOUNT = true;
const bool _QDBP_REUSE_ANALYSIS = true;
const bool _QDBP_OBJ_FREELIST = true;
const bool _QDBP_BOX_FREELIST = true;
const bool _QDBP_MEMORY_SANITIZE = true;
const bool _QDBP_ASSERTS_ENABLED = true;
#elif defined(QDBP_RELEASE)
const bool _QDBP_REFCOUNT = true;
const bool _QDBP_REUSE_ANALYSIS = true;
const bool _QDBP_OBJ_FREELIST = true;
const bool _QDBP_BOX_FREELIST = true;
const bool _QDBP_MEMORY_SANITIZE = false;
const bool _QDBP_ASSERTS_ENABLED = false;
#else
extern const bool _QDBP_REFCOUNT;
extern const bool _QDBP_REUSE_ANALYSIS;
extern const bool _QDBP_OBJ_FREELIST;
extern const bool _QDBP_BOX_FREELIST;
extern const bool _QDBP_MEMORY_SANITIZE;
extern const bool _QDBP_ASSERTS_ENABLED;
// Hashtable settings
static const size_t _QDBP_LOAD_FACTOR_NUM = 1;
static const size_t _QDBP_LOAD_FACTOR_DEN = 1;
#endif

#define _qdbp_assert(x)          \
  do {                           \
    if (_QDBP_ASSERTS_ENABLED) { \
      assert(x);                 \
    }                            \
  } while (0)

#define _QDBP_FREELIST_SIZE 1000

/*
    ====================================================
    Types
    ====================================================
*/
typedef uint32_t _qdbp_label_t;
typedef uint32_t _qdbp_tag_t;
typedef uint32_t _qdbp_refcount_t;

struct _qdbp_object;

struct _qdbp_method {
  struct _qdbp_object** captures;
  void* code;
  uint32_t num_captures;
};

struct _qdbp_field {
  struct _qdbp_method method;
  _qdbp_label_t label;
};

struct _qdbp_hashtable_header {
  size_t capacity;
  size_t size;
  size_t* directory;
};

typedef union {
  struct _qdbp_hashtable_header header;
  struct _qdbp_field field;
} _qdbp_hashtable;

struct _qdbp_prototype {
  _qdbp_hashtable* labels;
};

struct _qdbp_variant {
  struct _qdbp_object* value;
};

struct _qdbp_boxed_int {
  uint64_t value;
  struct _qdbp_prototype other_labels;
};

union _qdbp_object_data {
  struct _qdbp_prototype prototype;
  uint64_t i;
  double f;
  char* s;
  struct _qdbp_boxed_int* _qdbp_boxed_int;
  struct _qdbp_variant variant;
};

enum _qdbp_object_kind {
  QDBP_INT,
  QDBP_FLOAT,
  QDBP_STRING,
  QDBP_PROTOTYPE,
  QDBP_BOXED_INT,
  QDBP_VARIANT  // Must be last
};

struct __attribute__((packed)) _qdbp_object_metadata {
  _qdbp_refcount_t rc;
  _qdbp_tag_t tag;
};

struct _qdbp_object {
  struct _qdbp_object_metadata metadata;
  union _qdbp_object_data data;
};
typedef struct _qdbp_object* _qdbp_object_ptr;
typedef struct _qdbp_object** _qdbp_object_arr;
typedef struct _qdbp_variant* _qdbp_variant_ptr;
typedef struct _qdbp_prototype* _qdbp_prototype_ptr;
typedef struct _qdbp_method* _qdbp_method_ptr;
typedef struct _qdbp_field* _qdbp_field_ptr;
// Reference counting
bool _qdbp_is_unique(_qdbp_object_ptr obj);
void _qdbp_incref(_qdbp_object_ptr obj, _qdbp_refcount_t amount);
void _qdbp_decref(_qdbp_object_ptr obj, _qdbp_refcount_t amount);
void _qdbp_set_refcount(_qdbp_object_ptr obj, _qdbp_refcount_t refcount);
_qdbp_refcount_t _qdbp_get_refcount(_qdbp_object_ptr obj);
void _qdbp_drop(_qdbp_object_ptr obj, _qdbp_refcount_t cnt);
void _qdbp_dup(_qdbp_object_ptr obj, _qdbp_refcount_t cnt);
void _qdbp_dup_method_captures(_qdbp_method_ptr method);
void _qdbp_dup_prototype_captures(_qdbp_prototype_ptr proto);
void _qdbp_dup_prototype_captures_except(_qdbp_prototype_ptr proto,
                                         _qdbp_label_t except);

#define _qdbp_check_refcount(obj)                            \
  do {                                                       \
    if (_QDBP_ASSERTS_ENABLED && obj) {                      \
      _qdbp_assert(!_qdbp_is_unboxed_int(obj));              \
      _qdbp_assert((obj));                                   \
      if (_qdbp_get_refcount(obj) <= 0) {                    \
        printf("refcount of %u\n", _qdbp_get_refcount(obj)); \
        _qdbp_assert(false);                                 \
      };                                                     \
    }                                                        \
  } while (0);

// Hash table
_qdbp_hashtable* _qdbp_ht_new(size_t capacity);
void _qdbp_ht_del(_qdbp_hashtable* table);
_qdbp_hashtable* _qdbp_ht_duplicate(_qdbp_hashtable* table);
_qdbp_field_ptr _qdbp_ht_find(_qdbp_hashtable* table, _qdbp_label_t label);
__attribute__((warn_unused_result)) _qdbp_hashtable* _qdbp_ht_insert(
    _qdbp_hashtable* table, const _qdbp_field_ptr fld);
size_t _qdbp_ht_size(_qdbp_hashtable* table);
#define _QDBP_HT_ITER(ht, fld, tmp)                                       \
  for (tmp = 0; tmp < (ht)->header.size &&                                \
                (fld = &((ht)[(ht)->header.directory[tmp]].field), true); \
       tmp++)
// checked math
int64_t _qdbp_sign_extend(uint64_t a);
uint64_t _qdbp_checked_add(uint64_t a, uint64_t b);
uint64_t _qdbp_checked_sub(uint64_t a, uint64_t b);
uint64_t _qdbp_checked_mul(uint64_t a, uint64_t b);
uint64_t _qdbp_checked_div(uint64_t a, uint64_t b);
uint64_t _qdbp_checked_mod(uint64_t a, uint64_t b);
#define _QDBP_COMPARE(op, a, b) ((_qdbp_sign_extend(a))op(_qdbp_sign_extend(b)))
// Memory
#define _QDBP_STR_INTERNAL(x) #x
#define _QDBP_STR(x) _QDBP_STR_INTERNAL(x)

void* _qdbp_malloc_internal(size_t size, const char* message);
#define _qdbp_malloc(size) \
  _qdbp_malloc_internal(size, __FILE__ ":" _QDBP_STR(__LINE__))
void _qdbp_free(void* ptr);
void _qdbp_memcpy(void* dest, const void* src, size_t n);
bool _qdbp_check_mem();
void _qdbp_init();

// _qdbp_malloc_<type> allocates the physical memory for the object
struct _qdbp_boxed_int* _qdbp_malloc_boxed_int();
_qdbp_object_ptr _qdbp_malloc_obj();
_qdbp_object_arr _qdbp_malloc_capture_arr(size_t size);
// _qdbp_free_<type> free the physical memory of the object
void _qdbp_free_boxed_int(struct _qdbp_boxed_int* i);
void _qdbp_free_obj(_qdbp_object_ptr obj);
void _qdbp_free_capture_arr(_qdbp_object_arr arr, size_t size);
// _qdbp_del_<type> recursively drops the object's children
// then free the physical memory of the object
void _qdbp_del_fields(_qdbp_prototype_ptr proto);
void _qdbp_del_prototype(_qdbp_prototype_ptr proto);
void _qdbp_del_variant(_qdbp_variant_ptr variant);
void _qdbp_del_method(_qdbp_method_ptr method);
void _qdbp_del_obj(_qdbp_object_ptr obj);

// Object creation
// Each of these functions creates the object in memory and initializes its
// refcount to 1 and its value accordingly
_qdbp_object_ptr _qdbp_make_object(_qdbp_tag_t tag,
                                   union _qdbp_object_data data);
_qdbp_object_ptr _qdbp_make_boxed_int(uint64_t i);
_qdbp_object_ptr _qdbp_empty_prototype();
_qdbp_object_ptr _qdbp_true();
_qdbp_object_ptr _qdbp_false();
_qdbp_object_ptr _qdbp_int(uint64_t i);
_qdbp_object_ptr _qdbp_string(const char* src);
_qdbp_object_ptr _qdbp_float(double f);
_qdbp_object_ptr _qdbp_abort();
_qdbp_object_ptr _qdbp_match_failed();
_qdbp_object_ptr _qdbp_int_prototype(uint64_t i);
// Object initialization
// Each of these functions initializes the object's value
void _qdbp_init_field(_qdbp_field_ptr field, _qdbp_label_t label,
                      struct _qdbp_object** captures, void* code,
                      uint32_t num_captures);
// Prototypes
size_t _qdbp_prototype_size(_qdbp_prototype_ptr proto);
void _qdbp_label_add(_qdbp_prototype_ptr proto, _qdbp_label_t label,
                     _qdbp_field_ptr field, size_t default_capacity);
_qdbp_field_ptr _qdbp_label_get(_qdbp_prototype_ptr proto, _qdbp_label_t label);
void _qdbp_copy_prototype(_qdbp_prototype_ptr src, _qdbp_prototype_ptr dest);
// For each label in `new_prototype`, re-malloc the capture array and copy the
// captures from `old_prototype` to `new_prototype` except for the label
// `except`
void _qdbp_make_fresh_captures_except(_qdbp_prototype_ptr new_prototype,
                                      _qdbp_label_t except);
_qdbp_object_arr _qdbp_duplicate_captures(_qdbp_object_arr captures,
                                          size_t size);
// returns a pointer to the method's capture array and stores its fn pointer in
// `code_ptr`
_qdbp_object_arr _qdbp_get_method(_qdbp_object_ptr obj, _qdbp_label_t label,
                                  void** code /*output param*/);
_qdbp_object_ptr _qdbp_extend(_qdbp_object_ptr obj, _qdbp_label_t label,
                              void* code, _qdbp_object_arr captures,
                              size_t captures_size, size_t default_capacity);

_qdbp_object_ptr _qdbp_replace(_qdbp_object_ptr obj, _qdbp_label_t label,
                               void* code, _qdbp_object_arr captures,
                               size_t captures_size);

// invoke the `label` method of `receiver` with `arg0` and `arg1` as arguments
_qdbp_object_ptr _qdbp_invoke_1(_qdbp_object_ptr receiver, _qdbp_label_t label,
                                _qdbp_object_ptr arg0);
_qdbp_object_ptr _qdbp_invoke_2(_qdbp_object_ptr receiver, _qdbp_label_t label,
                                _qdbp_object_ptr arg0, _qdbp_object_ptr arg1);

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
bool _qdbp_is_unboxed_int(_qdbp_object_ptr obj);
_qdbp_object_ptr _qdbp_make_unboxed_int(uint64_t value);
uint64_t _qdbp_get_unboxed_int(_qdbp_object_ptr obj);
_qdbp_object_ptr _qdbp_unboxed_int_unary_op(_qdbp_object_ptr obj,
                                            _qdbp_label_t op);
_qdbp_object_ptr _qdbp_unboxed_int_binary_op(uint64_t a, uint64_t b,
                                             _qdbp_label_t op);
bool _qdbp_is_boxed_int(_qdbp_object_ptr obj);
uint64_t _qdbp_get_boxed_int(_qdbp_object_ptr obj);
_qdbp_object_ptr _qdbp_boxed_int_unary_op(_qdbp_object_ptr arg0,
                                          _qdbp_label_t op);
_qdbp_object_ptr _qdbp_boxed_int_binary_op(_qdbp_object_ptr a,
                                           _qdbp_object_ptr b,
                                           _qdbp_label_t op);
_qdbp_object_ptr _qdbp_box_unboxed_int_and_extend(_qdbp_object_ptr obj,
                                       _qdbp_label_t label, void* code,
                                       _qdbp_object_arr captures,
                                       size_t captures_size);
_qdbp_object_ptr _qdbp_boxed_int_extend(_qdbp_object_ptr obj,
                                        _qdbp_label_t label, void* code,
                                        _qdbp_object_arr captures,
                                        size_t captures_size);
_qdbp_object_ptr _qdbp_boxed_int_replace(_qdbp_object_ptr obj,
                                         _qdbp_label_t label, void* code,
                                         _qdbp_object_arr captures,
                                         size_t captures_size);
// Tags and Variants
enum _qdbp_object_kind _qdbp_get_kind(_qdbp_object_ptr obj);
void _qdbp_set_tag(_qdbp_object_ptr o, _qdbp_tag_t t);
_qdbp_tag_t _qdbp_get_tag(_qdbp_object_ptr o);
_qdbp_object_ptr _qdbp_variant_create(_qdbp_tag_t tag, _qdbp_object_ptr value);
void _qdbp_decompose_variant(_qdbp_object_ptr obj, _qdbp_tag_t* tag,
                             _qdbp_object_ptr* payload);
#define _qdbp_assert_kind(obj, k)             \
  do {                                        \
    if (obj && _QDBP_ASSERTS_ENABLED) {       \
      _qdbp_check_refcount(obj);              \
      _qdbp_assert(_qdbp_get_kind(obj) == k); \
    }                                         \
  } while (0);

// Macros for the various kinds of expressions
#define _QDBP_DROP(v, cnt, expr) (_qdbp_drop((v), (cnt)), (expr))
#define _QDBP_DUP(v, cnt, expr) (obj_dup((v), (cnt)), (expr))
#define _QDBP_LET(lhs, rhs, in) \
  ((lhs = (rhs)), (in))  // assume lhs has been declared already
#define _QDBP_MATCH(tag1, tag2, arg, ifmatch, ifnomatch) \
  ((tag1) == (tag2) ? (_QDBP_LET((arg), (payload), (ifmatch))) : (ifnomatch))

#endif
