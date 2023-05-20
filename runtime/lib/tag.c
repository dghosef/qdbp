#include "runtime.h"
enum qdbp_object_kind get_kind(qdbp_object_ptr obj) {
  if (DYNAMIC_TYPECHECK) {
    assert(!is_unboxed_int(obj));
    assert(obj != qdbp_true());
    assert(obj != qdbp_false());
    assert(obj->metadata.tag != QDBP_VARIANT);
  }
  if (obj->metadata.tag > QDBP_VARIANT) {
    return QDBP_VARIANT;
  } else {
    return obj->metadata.tag;
  }
}

void set_tag(qdbp_object_ptr o, tag_t t) { o->metadata.tag = t; }
tag_t get_tag(qdbp_object_ptr o) {
  return o->metadata.tag;
}
qdbp_object_ptr variant_create(tag_t tag, qdbp_object_ptr value) {
  if(tag == 21 && value == NULL) {
    // must keep up to date with basic_objs and namesToInts
    // and the function below
    return qdbp_true();
  } else if(tag == 20 && value == NULL) {
    return qdbp_false();
  }
  assert_refcount(value);
  qdbp_object_ptr new_obj =
      make_object(tag, (union qdbp_object_data){.variant = {.value = value}});
  return new_obj;
}
/* Variant Pattern Matching
  - dup the variant payload
  - drop the variant
  - execute the variant stuff
*/
void decompose_variant(qdbp_object_ptr obj, tag_t *tag,
                       qdbp_object_ptr *payload) {
    // must keep up to date with basic_objs and namesToInts
    // and the function above
  if(obj == qdbp_true()) {
    *tag = 21;
    *payload = NULL;
    return;
  } else if(obj == qdbp_false()) {
    *tag = 20;
    *payload = NULL;
    return;
  }
  assert_obj_kind(obj, QDBP_VARIANT);
  qdbp_object_ptr value = obj->data.variant.value;
  *tag = get_tag(obj);
  // return value, tag
  *payload = value;
}
