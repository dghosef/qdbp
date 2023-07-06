#include <stdio.h>

#include "runtime.h"

enum _qdbp_object_kind _qdbp_get_kind(_qdbp_object_ptr obj) {
  _qdbp_assert(!_qdbp_is_unboxed_int(obj));
  _qdbp_assert(obj != _qdbp_true());
  _qdbp_assert(obj != _qdbp_false());
  _qdbp_assert(obj->metadata.tag != QDBP_VARIANT);
  if (obj->metadata.tag > QDBP_VARIANT) {
    return QDBP_VARIANT;
  } else {
    return obj->metadata.tag;
  }
}

void _qdbp_set_tag(_qdbp_object_ptr o, _qdbp_tag_t t) { o->metadata.tag = t; }

_qdbp_tag_t _qdbp_get_tag(_qdbp_object_ptr o) { return o->metadata.tag; }

_qdbp_object_ptr _qdbp_variant_create(_qdbp_tag_t tag, _qdbp_object_ptr value) {
  if (tag == 21 && value == NULL) {
    // must keep up to date with basic_objs and namesToInts
    // and the function below
    return _qdbp_true();
  } else if (tag == 20 && value == NULL) {
    return _qdbp_false();
  }
  _qdbp_check_refcount(value);
  _qdbp_object_ptr new_obj = _qdbp_make_object(
      tag, (union _qdbp_object_data){.variant = {.value = value}});
  return new_obj;
}

/* Variant Pattern Matching
  - dup the variant payload
  - _qdbp_drop the variant
  - execute the variant stuff
*/
void _qdbp_decompose_variant(_qdbp_object_ptr obj, _qdbp_tag_t *tag,
                             _qdbp_object_ptr *payload) {
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
  _qdbp_assert_kind(obj, QDBP_VARIANT);
  _qdbp_object_ptr value = obj->data.variant.value;
  *tag = _qdbp_get_tag(obj);
  // return value, tag
  *payload = value;
}
