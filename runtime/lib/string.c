#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "runtime.h"

_qdbp_object_ptr _qdbp_string_unary_op(_qdbp_object_ptr obj,
                                       enum _QDBP_ARITH_OP op) {
  _qdbp_assert_kind(obj, _QDBP_STRING);
  switch (op) {
    case _QDBP_PRINT:
      printf("%s", obj->data.string->value);
      _qdbp_drop(obj, 1);
      return _qdbp_empty_prototype();
      break;
    case _QDBP_EXEC:
      // Execute the string as a command
      system(obj->data.string->value);
      _qdbp_drop(obj, 1);
      return _qdbp_empty_prototype();
      break;
    default:
      _qdbp_assert(false);
      __builtin_unreachable();
  }
}

// TODO: This is missing reuse analysis
_qdbp_object_ptr _qdbp_string_binary_op(_qdbp_object_ptr l, _qdbp_object_ptr r,
                                        enum _QDBP_ARITH_OP op) {
  _qdbp_assert_kind(l, _QDBP_STRING);
  _qdbp_assert_kind(r, _QDBP_STRING);
  switch (op) {
    case _QDBP_ADD: {
      // Concatenate strings
      size_t new_length = l->data.string->length + r->data.string->length;
      char* new_value = _qdbp_malloc(new_length + 1);
      if (!new_value) { _qdbp_abort(); }
      memcpy(new_value, l->data.string->value, l->data.string->length);
      memcpy(new_value + l->data.string->length, r->data.string->value,
             r->data.string->length);
      new_value[new_length] = '\0';  // Null-terminate the new string

      _qdbp_object_ptr result = _qdbp_make_string(new_value, new_length);
      _qdbp_free(new_value);  // Free the temporary string buffer
      _qdbp_drop(l, 1);
      _qdbp_drop(r, 1);
      return result;
    }
    default:
      _qdbp_assert(false);
  }
  _qdbp_assert(false);
  assert(false);
  _qdbp_drop(l, 1);
  _qdbp_drop(r, 1);
}