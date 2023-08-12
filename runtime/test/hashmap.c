#include "runtime.h"
#include "test.h"

static void code() {}
TEST_SUITE(hashtable)

TEST_CASE_NOLEAK(sequential_insert) {
  size_t capacity_lg2 = 0;
  for (size_t initial_capacity = 1; initial_capacity < 200;
       initial_capacity *= 2) {
    _qdbp_hashtable_t *ht = _qdbp_ht_new(initial_capacity, capacity_lg2);
    for (size_t elem = 0; elem < 100; elem++) {
      struct _qdbp_field fld;
      fld.label = elem;
      fld.method.captures = NULL;
      fld.method.code = (void *)code;
      fld.method.num_captures = elem + 3;
      ht = _qdbp_ht_insert(ht, &fld);
      _qdbp_hashtable_t *ht2 = _qdbp_ht_duplicate(ht);
      CHECK_INT_CMP(==, _qdbp_ht_size(ht), elem + 1);
      for (size_t i = 0; i <= elem; i++) {
        _qdbp_field_ptr found = _qdbp_ht_find(ht, i);
        CHECK_INT_CMP(==, found->label, i);
        CHECK_INT_CMP(==, found->method.num_captures, i + 3);
        _qdbp_field_ptr found2 = _qdbp_ht_find(ht2, i);
        CHECK_INT_CMP(==, found2->label, i);
        CHECK_INT_CMP(==, found2->method.num_captures, i + 3);
        CHECK(found2 == _qdbp_ht_find_opt(ht2, i));
      }
      for(int j = elem + 1; j < 100; j++) {
        CHECK(_qdbp_ht_find_opt(ht, j) == NULL);
        CHECK(_qdbp_ht_find_opt(ht2, j) == NULL);
      }
      _qdbp_ht_del(ht2);
    }
    _qdbp_ht_del(ht);
  capacity_lg2++;
  }
}

TEST_CASE_NOLEAK(random_insert) {
  size_t capacity_lg2 = 0;
  for (size_t initial_capacity = 1; initial_capacity < 200;
       initial_capacity *= 2) {
    _qdbp_hashtable_t *ht = _qdbp_ht_new(initial_capacity, capacity_lg2);
    for (size_t elem = 0; elem < 100; elem++) {
      struct _qdbp_field fld;
      _qdbp_label_t label = randomU32();
      fld.label = label;
      fld.method.captures = NULL;
      fld.method.code = (void *)code;
      fld.method.num_captures = label + 3;
      ht = _qdbp_ht_insert(ht, &fld);

      _QDBP_HT_ITER(ht, fld2, tmp) {
        CHECK_INT_CMP(==, fld2->label, _qdbp_ht_find(ht, fld2->label)->label);
      }
    }
    _qdbp_ht_del(ht);
  capacity_lg2++;
  }
}

TEST_SUITE_END()
