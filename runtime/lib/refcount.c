#include "runtime.h"
void incref(qdbp_object_ptr obj, refcount_t amount) {
  obj->metadata.rc += amount;
}
void decref(qdbp_object_ptr obj, refcount_t amount) {
  obj->metadata.rc -= amount;
}
void set_refcount(qdbp_object_ptr obj, refcount_t refcount) {
  obj->metadata.rc = refcount;
}
refcount_t get_refcount(qdbp_object_ptr obj) { return obj->metadata.rc; }

bool is_unique(qdbp_object_ptr obj);

bool drop(qdbp_object_ptr obj, refcount_t cnt) {
  assert_refcount(obj);
  if (VERIFY_REFCOUNTS) {
    assert(get_refcount(obj) >= cnt);
  }
  decref(obj, cnt);
  if (get_refcount(obj) == 0) {
    del_obj(obj);
    return true;
  }
  return false;
}

void obj_dup(qdbp_object_ptr obj, refcount_t cnt) {
  assert_refcount(obj);
  incref(obj, cnt);
}

void dup_captures(qdbp_method_ptr method) {
  for (size_t i = 0; i < method->captures_size; i++) {
    obj_dup((method->captures[i]), 1);
  }
}

void dup_prototype_captures(qdbp_prototype_ptr proto) {
  Word_t label = 0;
  qdbp_field_ptr *PValue;
  JLF(PValue, proto->labels, label);
  while (PValue != NULL) {
    dup_captures(&((*PValue)->method));
    JLN(PValue, proto->labels, label);
  }
}
void dup_prototype_captures_except(qdbp_prototype_ptr proto, label_t except) {
  Word_t label = 0;
  qdbp_field_ptr *PValue;
  JLF(PValue, proto->labels, label);
  while (PValue != NULL) {
    if (label != except) {
      dup_captures(&((*PValue)->method));
    }
    JLN(PValue, proto->labels, label);
  }
}

bool is_unique(qdbp_object_ptr obj) {
  assert(obj);
  if (VERIFY_REFCOUNTS) {
    assert(get_refcount(obj) >= 0);
  }
  return get_refcount(obj) == 1;
}
