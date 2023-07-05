#include "runtime.h"
#include "test.h"

static void code() {}
TEST_SUITE(hashtable)

TEST_CASE_NOLEAK(sequential_insert) {
  for (size_t initial_capacity = 1; initial_capacity < 1000;
       initial_capacity *= 2) {
    _qdbp_hashtable *ht = _qdbp_new_ht(initial_capacity);
    for (size_t elem = 0; elem < 1000; elem++) {
      struct _qdbp_field fld;
      fld.label = elem;
      fld.method.captures = NULL;
      fld.method.code = (void *)code;
      fld.method.captures_size = elem + 3;
      ht = _qdbp_ht_insert(ht, &fld);
      _qdbp_hashtable *ht2 = _qdbp_ht_duplicate(ht);
      CHECK_INT_CMP(==, _qdbp_hashtable_size(ht), elem + 1);
      for (size_t i = 0; i <= elem; i++) {
        _qdbp_field_ptr found = _qdbp_ht_find(ht, i);
        CHECK_INT_CMP(==, found->label, i);
        CHECK_INT_CMP(==, found->method.captures_size, i + 3);
        _qdbp_field_ptr found2 = _qdbp_ht_find(ht2, i);
        CHECK_INT_CMP(==, found2->label, i);
        CHECK_INT_CMP(==, found2->method.captures_size, i + 3);
      }
      _qdbp_del_ht(ht2);
    }
    _qdbp_del_ht(ht);
  }
}

TEST_CASE(random_insert) {
  for (size_t initial_capacity = 1; initial_capacity < 1000;
       initial_capacity *= 2) {
    _qdbp_hashtable *ht = _qdbp_new_ht(initial_capacity);
    for (size_t elem = 0; elem < 1000; elem++) {
      struct _qdbp_field fld;
      _qdbp_label_t label = randomU32();
      fld.label = label;
      fld.method.captures = NULL;
      fld.method.code = (void *)code;
      fld.method.captures_size = label + 3;
      ht = _qdbp_ht_insert(ht, &fld);

      _qdbp_field_ptr fld2;
      size_t tmp;
      _QDBP_HT_ITER(ht, fld2, tmp) {
        CHECK_INT_CMP(==, fld2->label, _qdbp_ht_find(ht, fld2->label)->label);
      }
    }
    _qdbp_del_ht(ht);
  }
}

TEST_SUITE_END()
