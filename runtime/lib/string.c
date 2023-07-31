#include <math.h>
#include <stdio.h>
#include <string.h>

#include "runtime.h"

static char *_qdbp_c_str_concat(const char *a, const char *b) {
  int lena = strlen(a);
  int lenb = strlen(b);
  char *con = (char *)_qdbp_malloc(lena + lenb + 1);
  // copy & concat (including string termination)
  _qdbp_memcpy(con, a, lena);
  _qdbp_memcpy(con + lena, b, lenb + 1);
  return con;
}

// concat_string
_qdbp_object_ptr _qdbp_concat_string(_qdbp_object_ptr a, _qdbp_object_ptr b) {
  _qdbp_assert_kind(a, _QDBP_STRING);
  _qdbp_assert_kind(b, _QDBP_STRING);
  const char *a_str = a->data.string;
  const char *b_str = b->data.string;
  _qdbp_drop(a, 1);
  _qdbp_drop(b, 1);
  return _qdbp_make_object(
      _QDBP_STRING,
      (union _qdbp_object_data){.string = _qdbp_c_str_concat(a_str, b_str)});
}

_qdbp_object_ptr _qdbp_empty_string() {
  char *s = (char *)_qdbp_malloc(1);
  s[0] = '\0';
  return _qdbp_make_object(_QDBP_STRING,
                           (union _qdbp_object_data){.string = s});
}
