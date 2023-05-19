#include "runtime.h"
void label_add(qdbp_prototype_ptr proto, label_t label, qdbp_field_ptr field) {
  Word_t *PValue;
  if (DYNAMIC_TYPECHECK) {
    void *get;
    JLG(get, proto->labels, label);
    if (get != NULL) {
      printf("Label %llu already exists in prototype\n", label);
      assert(false);
    }
  }
  JLI(PValue, proto->labels, label);
  *PValue = (uintptr_t)field;
}
void label_replace(qdbp_prototype_ptr proto, label_t label,
                   qdbp_field_ptr field) {
  if (DYNAMIC_TYPECHECK) {
    void *get;
    JLG(get, proto->labels, label);
    if (get == NULL) {
      printf("Label %llu does not exist in prototype\n", label);
      assert(false);
    }
  }
  int rc;
  JLD(rc, proto->labels, label);
  if (DYNAMIC_TYPECHECK) {
    assert(rc);
  }
  Word_t *PValue;
  JLI(PValue, proto->labels, label);
  *PValue = (uintptr_t)field;
}
qdbp_field_ptr label_get(qdbp_prototype_ptr proto, label_t label) {
  Word_t *PValue;
  if (DYNAMIC_TYPECHECK) {
    assert(proto);
    assert(proto->labels);
  }
  JLG(PValue, proto->labels, label);
  qdbp_field_ptr field = *(qdbp_field_ptr *)PValue;
  if (DYNAMIC_TYPECHECK) {
    assert(proto->labels);
    assert(field != NULL);
  }
  return field;
}

void copy_captures_except(qdbp_prototype_ptr new_prototype, label_t except) {
  // Copy all the capture arrays except the new one
  Word_t label = 0;
  qdbp_field_ptr *PValue;
  JLF(PValue, new_prototype->labels, label);
  while (PValue != NULL) {
    qdbp_field_ptr field = *PValue;
    if (label != except) {
      qdbp_object_arr original = field->method.captures;
      field->method.captures = make_capture_arr(field->method.captures_size);
      qdbp_memcpy(field->method.captures, original,
                  sizeof(qdbp_object_ptr) * field->method.captures_size);
    }
    JLN(PValue, new_prototype->labels, label);
  }
}

static struct qdbp_prototype
raw_prototype_replace(const qdbp_prototype_ptr src,
                      const qdbp_field_ptr new_field, label_t new_label) {
  size_t src_size = proto_size(src);
  if (DYNAMIC_TYPECHECK) {
    assert(src_size);
  }
  struct qdbp_prototype new_prototype = {.labels = NULL};

  Word_t label = 0;
  qdbp_field_ptr *PValue;
  JLF(PValue, src->labels, label);
  while (PValue != NULL) {
    qdbp_field_ptr new_field_ptr = qdbp_malloc_field();
    label_add(&new_prototype, label, new_field_ptr);
    *new_field_ptr = **PValue;
    JLN(PValue, src->labels, label);
  }

  // overwrite the field
  *label_get(&new_prototype, new_label) = *new_field;
  copy_captures_except(&new_prototype, new_label);
  return new_prototype;
}

static struct qdbp_prototype
raw_prototype_extend(const qdbp_prototype_ptr src,
                     const qdbp_field_ptr new_field, size_t new_label) {
  // copy src
  size_t src_size = proto_size(src);
  struct qdbp_prototype new_prototype = {.labels = NULL};

  Word_t label = 0;
  qdbp_field_ptr *PValue;
  JLF(PValue, src->labels, label);
  while (PValue != NULL) {
    qdbp_field_ptr new_field_ptr = qdbp_malloc_field();
    *new_field_ptr = **PValue;
    label_add(&new_prototype, label, new_field_ptr);
    JLN(PValue, src->labels, label);
  }
  qdbp_field_ptr new_field_ptr = qdbp_malloc_field();
  *new_field_ptr = *new_field;
  label_add(&new_prototype, new_label, new_field_ptr);
  copy_captures_except(&new_prototype, new_label);
  return new_prototype;
}

qdbp_object_arr get_method(qdbp_object_ptr obj, label_t label,
                           void **code_ptr /*output param*/) {
  assert_obj_kind(obj, QDBP_PROTOTYPE);
  struct qdbp_method m = label_get(&(obj->data.prototype), label)->method;
  dup_captures(&m);
  *code_ptr = m.code;
  qdbp_object_arr ret = m.captures;
  return ret;
}

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

__attribute__((always_inline)) qdbp_object_ptr extend(qdbp_object_ptr obj,
                                                      label_t label, void *code,
                                                      qdbp_object_arr captures,
                                                      size_t captures_size) {

  if (is_unboxed_int(obj)) {
    printf("Haven't implemented replace for unboxed int\n");
    assert(false);
  }
  struct qdbp_field f = {
      .method = {.captures = make_captures(captures, captures_size),
                 .captures_size = captures_size,
                 .code = code}};
  assert_obj_kind(obj, QDBP_PROTOTYPE);
  if (!REUSE_ANALYSIS || !is_unique(obj)) {
    qdbp_prototype_ptr prototype = &(obj->data.prototype);
    struct qdbp_prototype new_prototype =
        raw_prototype_extend(prototype, &f, label);
    qdbp_object_ptr new_obj = make_object(
        QDBP_PROTOTYPE, (union qdbp_object_data){.prototype = new_prototype});
    dup_prototype_captures(prototype);
    drop(obj, 1);
    return new_obj;
  } else {
    qdbp_field_ptr new_field = qdbp_malloc_field();
    *new_field = f;
    label_add(&(obj->data.prototype), label, new_field);
    return obj;
  }
}
__attribute__((always_inline)) qdbp_object_ptr
replace(qdbp_object_ptr obj, label_t label, void *code,
        qdbp_object_arr captures, size_t captures_size) {
  if (is_unboxed_int(obj)) {
    printf("Haven't implemented replace for unboxed int\n");
    assert(false);
  }
  assert_obj_kind(obj, QDBP_PROTOTYPE);
  struct qdbp_field f = {
      .method = {.captures = make_captures(captures, captures_size),
                 .captures_size = captures_size,
                 .code = code}};
  if (!REUSE_ANALYSIS || !is_unique(obj)) {
    struct qdbp_prototype new_prototype =
        raw_prototype_replace(&(obj->data.prototype), &f, label);
    dup_prototype_captures_except(&(obj->data.prototype), label);

    qdbp_object_ptr new_obj = make_object(
        QDBP_PROTOTYPE, (union qdbp_object_data){.prototype = new_prototype});

    drop(obj, 1);
    return new_obj;
  } else {
    // find the index to replace
    qdbp_field_ptr field = label_get(&(obj->data.prototype), label);
    // reuse the field
    del_method(&(field->method));
    *field = f;
    return obj;
  }
}

size_t proto_size(qdbp_prototype_ptr proto) {
  size_t ret;
  JLC(ret, proto->labels, 0, -1);
  return ret;
}
qdbp_object_ptr invoke_1(qdbp_object_ptr receiver, label_t label,
                         qdbp_object_ptr arg0) {
  if (is_unboxed_int(arg0)) {
    return unboxed_unary_op(arg0, label);
  } else {
    void *code;
    qdbp_object_arr captures = get_method(receiver, label, &code);
    return ((qdbp_object_ptr(*)(qdbp_object_arr, qdbp_object_ptr))code)(
        captures, arg0);
  }
}

qdbp_object_ptr invoke_2(qdbp_object_ptr receiver, label_t label,
                         qdbp_object_ptr arg0, qdbp_object_ptr arg1) {
  if(is_unboxed_int(arg0)) {
    return unboxed_binary_op(arg0, arg1, label);
  }
  void *code;
  qdbp_object_arr captures = get_method(receiver, label, &code);
  return ((qdbp_object_ptr(*)(qdbp_object_arr, qdbp_object_ptr,
                              qdbp_object_ptr))code)(captures, arg0, arg1);
}
