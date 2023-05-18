
#include "runtime.h"
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

qdbp_object_ptr match_failed() {
  assert(false);
  __builtin_unreachable();
}

// Keep track of objects for testing
qdbp_object_ptr make_object(tag_t tag, union qdbp_object_data data) {
  qdbp_object_ptr new_obj = qdbp_malloc_obj();
  set_refcount(new_obj, 1);
  set_tag(new_obj, tag);
  new_obj->data = data;
  return new_obj;
}

qdbp_object_ptr empty_prototype() {
  qdbp_object_ptr obj = make_object(
      QDBP_PROTOTYPE, (union qdbp_object_data){.prototype = {.labels = NULL}});
  return obj;
}
qdbp_object_ptr qdbp_true() {
  qdbp_object_ptr o = make_object(
      21, // MUST keep up to date w. namesToInts.ml
      (union qdbp_object_data){.variant = {.value = empty_prototype()}});
  return o;
}
qdbp_object_ptr qdbp_false() {
  qdbp_object_ptr o = make_object(
      20, (union qdbp_object_data){.variant = {.value = empty_prototype()}});
  return o;
}
