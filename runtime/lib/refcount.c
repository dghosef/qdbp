#include "runtime.h"
void incref(qdbp_object_ptr obj, refcount_t amount) {
  if(DYNAMIC_TYPECHECK && is_unboxed_int(obj)) {
    assert(false);
  }
  obj->metadata.rc += amount;
}
void decref(qdbp_object_ptr obj, refcount_t amount) {
  if(DYNAMIC_TYPECHECK && is_unboxed_int(obj)) {
    assert(false);
  }
  obj->metadata.rc -= amount;
}
void set_refcount(qdbp_object_ptr obj, refcount_t refcount) {
  if(DYNAMIC_TYPECHECK && is_unboxed_int(obj)) {
    assert(false);
  }
  obj->metadata.rc = refcount;
}
refcount_t get_refcount(qdbp_object_ptr obj) {
  if(DYNAMIC_TYPECHECK && is_unboxed_int(obj)) {
    assert(false);
  }
  return obj->metadata.rc;
}

void drop(qdbp_object_ptr obj, refcount_t cnt) {
  if (is_unboxed_int(obj)) {
    return;
  }
  else {
    assert_refcount(obj);
    if (VERIFY_REFCOUNTS) {
      assert(get_refcount(obj) >= cnt);
    }
    decref(obj, cnt);
    if (get_refcount(obj) == 0) {
      del_obj(obj);
      return;
    }
    return;
  }
}

void obj_dup(qdbp_object_ptr obj, refcount_t cnt) {
  if (is_unboxed_int(obj)) {
    return;
  } else {
    assert_refcount(obj);
    incref(obj, cnt);
  }
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
  if (is_unboxed_int(obj)) {
    return true;
  } else {
    assert(obj);
    if (VERIFY_REFCOUNTS) {
      assert(get_refcount(obj) >= 0);
    }
    return get_refcount(obj) == 1;
  }
}
