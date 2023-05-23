#include "runtime.h"
void label_add(qdbp_prototype_ptr proto, label_t label, qdbp_field_ptr field) {
  if (!proto->labels) {
    proto->labels = new_ht();
  }
  proto->labels = ht_insert(proto->labels, field);
}
qdbp_field_ptr label_get(qdbp_prototype_ptr proto, label_t label) {
  if (DYNAMIC_TYPECHECK) {
    assert(proto);
    assert(proto->labels);
  }
  return ht_find(proto->labels, label);
}

void copy_captures_except(qdbp_prototype_ptr new_prototype, label_t except) {
  // Copy all the capture arrays except the new one
  qdbp_field_ptr field;
  size_t tmp;
  HT_ITER(new_prototype->labels, field, tmp) {
    if (field->label != except) {
      qdbp_object_arr original = field->method.captures;
      field->method.captures = make_capture_arr(field->method.captures_size);
      qdbp_memcpy(field->method.captures, original,
                  sizeof(qdbp_object_ptr) * field->method.captures_size);
    }
  }
}

static struct qdbp_prototype
raw_prototype_replace(const qdbp_prototype_ptr src,
                      const qdbp_field_ptr new_field, label_t new_label) {
  size_t src_size = proto_size(src);
  if (DYNAMIC_TYPECHECK) {
    assert(src_size);
    assert(new_field->label == new_label);
  }
  struct qdbp_prototype new_prototype = {.labels = NULL};

  // FIXME: Have the overwriting done in duplicate_labels
  // to avoid extra work of inserting twice
  if (DYNAMIC_TYPECHECK) {
    assert(new_prototype.labels == NULL);
  }
  duplicate_labels(src, &new_prototype);

  *(ht_find(new_prototype.labels, new_label)) = *new_field;
  copy_captures_except(&new_prototype, new_label);
  if (DYNAMIC_TYPECHECK) {
    assert(new_prototype.labels);
  }
  return new_prototype;
}

static struct qdbp_prototype
raw_prototype_extend(const qdbp_prototype_ptr src,
                     const qdbp_field_ptr new_field, size_t new_label) {
  // copy src
  size_t src_size = proto_size(src);
  struct qdbp_prototype new_prototype = {.labels = NULL};

  duplicate_labels(src, &new_prototype);
  label_add(&new_prototype, new_label, new_field);
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
    qdbp_object_arr out = (qdbp_object_arr)qdbp_malloc(
        sizeof(struct qdbp_object *) * size, "captures");
    qdbp_memcpy(out, captures, sizeof(struct qdbp_object *) * size);
    return out;
  }
}

qdbp_object_ptr extend(qdbp_object_ptr obj,
                                                      label_t label, void *code,
                                                      qdbp_object_arr captures,
                                                      size_t captures_size) {

  if (!obj) {
    obj = make_object(QDBP_PROTOTYPE,
                      (union qdbp_object_data){.prototype = {.labels = NULL}});
  } else if (is_unboxed_int(obj)) {
    return box(obj, label, code, captures, captures_size);
  } else if (is_boxed_int(obj)) {
    return box_extend(obj, label, code, captures, captures_size);
  }
  struct qdbp_field f = {
      .label = label,
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
    label_add(&(obj->data.prototype), label, &f);
    return obj;
  }
}
qdbp_object_ptr
replace(qdbp_object_ptr obj, label_t label, void *code,
        qdbp_object_arr captures, size_t captures_size) {
  if (!obj) {
    obj = make_object(QDBP_PROTOTYPE,
                      (union qdbp_object_data){.prototype = {.labels = NULL}});
  } else if (is_unboxed_int(obj)) {
    obj = make_int_proto(get_unboxed_int(obj));
    assert_obj_kind(obj, QDBP_PROTOTYPE);
  } else if (is_boxed_int(obj) && label < NUM_OP_CNT) {
    int64_t i = get_boxed_int(obj);
    drop(obj, 1);
    obj = make_int_proto(i);
    assert_obj_kind(obj, QDBP_PROTOTYPE);
  } else if (is_boxed_int(obj) && label >= NUM_OP_CNT) {
    return boxed_int_replace(obj, label, code, captures, captures_size);
  }

  assert_obj_kind(obj, QDBP_PROTOTYPE);
  struct qdbp_field f = {
      .label = label,
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
    assert(field->label == label);
    // reuse the field
    del_method(&(field->method));
    field->label = f.label;
    field->method = f.method;
    return obj;
  }
}

size_t proto_size(qdbp_prototype_ptr proto) {
  if (DYNAMIC_TYPECHECK) {
    assert(proto);
  }
  return hashtable_size(proto->labels);
}
qdbp_object_ptr invoke_1(qdbp_object_ptr receiver, label_t label,
                         qdbp_object_ptr arg0) {
  if (is_unboxed_int(arg0)) {
    return unboxed_unary_op(arg0, label);
  } else if (is_boxed_int(arg0)) {
    if (label < ADD) {
      return boxed_unary_op(arg0, label);
    } else {
      qdbp_field_ptr field =
          label_get(&(arg0->data.boxed_int->other_labels), label);
      dup_captures(&(field->method));
      return ((qdbp_object_ptr(*)(qdbp_object_arr, qdbp_object_ptr))
                  field->method.code)(field->method.captures, arg0);
    }
  } else {
    void *code;
    qdbp_object_arr captures = get_method(receiver, label, &code);
    return ((qdbp_object_ptr(*)(qdbp_object_arr, qdbp_object_ptr))code)(
        captures, arg0);
  }
}

qdbp_object_ptr invoke_2(qdbp_object_ptr receiver, label_t label,
                         qdbp_object_ptr arg0, qdbp_object_ptr arg1) {
  // UB UB
  if (is_unboxed_int(arg0) && is_unboxed_int(arg1)) {
    return unboxed_binary_op(get_unboxed_int(arg0), get_unboxed_int(arg1),
                             label);
  }
  // UB B
  else if (is_unboxed_int(arg0) && is_boxed_int(arg1)) {
    qdbp_object_ptr result =
        unboxed_binary_op(get_unboxed_int(arg0), get_boxed_int(arg1), label);
    drop(arg1, 1);
    return result;
  }
  // UB BB
  else if (is_unboxed_int(arg0) && get_kind(arg1) == QDBP_PROTOTYPE) {
    int64_t a = get_unboxed_int(arg0);
    arg1 = invoke_1(arg1, VAL, arg1);
    if (DYNAMIC_TYPECHECK) {
      assert(get_kind(arg1) == QDBP_INT);
    }
    int64_t b = arg1->data.i;
    drop(arg1, 1);
    return unboxed_binary_op(a, b, label);
  }
  // B UB
  // B B
  // B BB
  else if (is_boxed_int(arg0)) {
    if (label < NUM_OP_CNT) {
      // print label
      return boxed_binary_op(arg0, arg1, label);
    } else {
      // get the method from the binary op
      qdbp_field_ptr field =
          label_get(&arg0->data.boxed_int->other_labels, label);
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
    if (DYNAMIC_TYPECHECK) {
      assert(get_kind(arg0) == QDBP_PROTOTYPE);
    }
    void *code;
    qdbp_object_arr captures = get_method(receiver, label, &code);
    return ((qdbp_object_ptr(*)(qdbp_object_arr, qdbp_object_ptr,
                                qdbp_object_ptr))code)(captures, arg0, arg1);
  }
}
