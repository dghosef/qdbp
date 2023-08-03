#include <math.h>
#include <stdio.h>
#include <string.h>

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
    default:
      _qdbp_assert(false);
      __builtin_unreachable();
  }
}

_qdbp_object_ptr _qdbp_string_binary_op(_qdbp_object_ptr l, _qdbp_object_ptr r,
                                        enum _QDBP_ARITH_OP op) {
  _qdbp_assert(false);
  assert(false);
  _qdbp_drop(l, 1);
  _qdbp_drop(r, 1);
}