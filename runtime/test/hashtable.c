#include "runtime.h"
#include "test.h"
TEST_SUITE(hashtable)

TEST_CASE(eq) {
  for (size_t initial_capacity = 1; initial_capacity < 1000;
       initial_capacity *= 2) {
    _qdbp_hashtable *ht = _qdbp_new_ht(initial_capacity);
    for (size_t elem = 0; elem < 1000; elem++) {
      struct _qdbp_field fld;
      fld.label = elem;
      fld.method.captures = NULL;
      fld.method.code = NULL;
      fld.method.captures_size = elem + 3;
      ht = _qdbp_ht_insert(ht, &fld);
      for (size_t i = 0; i <= elem; i++) {
        _qdbp_field_ptr found = _qdbp_ht_find(ht, i);
        CHECK_INT_CMP(==, found->label, i);
        CHECK_INT_CMP(==, found->method.captures_size, i + 3);
      }
    }
    _qdbp_del_ht(ht);
  }
}

TEST_SUITE_END()
