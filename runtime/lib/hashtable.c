#ifndef HASHTABLE_C
#define HASHTABLE_C
#include <string.h>

#include "runtime.h"

__attribute__((always_inline)) size_t _qdbp_hashtable_size(
    _qdbp_hashtable *table) {
  _qdbp_assert(table);
  return table->header.size;
}

__attribute__((always_inline)) _qdbp_hashtable *_qdbp_new_ht(size_t capacity) {
  // FIXME: Put into freelist
  _qdbp_hashtable *table =
      (_qdbp_hashtable *)_qdbp_malloc((capacity + 1) * sizeof(_qdbp_hashtable));
  for (size_t i = 1; i <= capacity; i++) {
    table[i].field.method.code = NULL;
  }
  table->header.size = 0;
  table->header.capacity = capacity;
  table->header.directory = _qdbp_malloc(capacity * sizeof(size_t));
  return table;
}

__attribute__((always_inline)) void _qdbp_del_ht(_qdbp_hashtable *table) {
  _qdbp_free(table->header.directory);
  _qdbp_free(table);
}

__attribute__((always_inline)) _qdbp_hashtable *_qdbp_ht_duplicate(
    _qdbp_hashtable *table) {
  // TODO: Use realloc
  _qdbp_hashtable *new_table =
      _qdbp_malloc(sizeof(_qdbp_hashtable) +
                   (sizeof(_qdbp_hashtable) * table->header.capacity));
  memcpy(new_table, table,
         sizeof(_qdbp_hashtable) +
             (sizeof(_qdbp_hashtable) * (table->header.capacity)));
  new_table->header.directory =
      _qdbp_malloc(sizeof(size_t) * (table->header.capacity));
  // FIXME: Only memcpy the table->header.size components
  memcpy(new_table->header.directory, table->header.directory,
         sizeof(size_t) * table->header.size);
  return new_table;
}

__attribute__((always_inline))
// n must be a power of 2
static size_t
fast_mod(size_t x, size_t n) {
  // check that n is a power of 2
  _qdbp_assert((n & (n - 1)) == 0);
  return x & (n - 1);
}

__attribute__((always_inline)) _qdbp_field_ptr _qdbp_ht_find(
    _qdbp_hashtable *table, _qdbp_label_t label) {
  size_t index = fast_mod(label, table->header.capacity);
  // FIXME: Assert that label != 0 and then remove second condition
  _qdbp_assert(index <= table->header.capacity);
  _qdbp_hashtable *fields = table + 1;
  if (fields[index].field.label == label) {
    return &(fields[index].field);
  }
  index = fast_mod(index + 1, table->header.capacity);
  while (fields[index].field.label != label) {
    _qdbp_assert(fields[index].field.method.code != NULL);
    index = fast_mod(index + 1, table->header.capacity);
    _qdbp_assert(index <= table->header.capacity);
  }
  return &(fields[index].field);
}

__attribute__((always_inline)) static void ht_insert_raw(
    _qdbp_hashtable *table, const _qdbp_field_ptr fld) {
  size_t index = fast_mod(fld->label, table->header.capacity);
  _qdbp_hashtable *fields = table + 1;
  _qdbp_assert(index <= table->header.capacity);
  while (fields[index].field.method.code != NULL) {
    index = fast_mod(index + 1, table->header.capacity);
    _qdbp_assert(index <= table->header.capacity);
  }
  fields[index].field = *fld;
  table->header.directory[table->header.size] = index + 1;
  table->header.size++;
}

__attribute__((always_inline)) _qdbp_hashtable *_qdbp_ht_insert(
    _qdbp_hashtable *table, const _qdbp_field_ptr fld) {
  _qdbp_assert(fld->method.code != NULL);
  if (table->header.size * _QDBP_LOAD_FACTOR_NUM >=
      table->header.capacity * _QDBP_LOAD_FACTOR_DEN) {
    _qdbp_hashtable *new_table = _qdbp_malloc((table->header.capacity * 2 + 1) *
                                              sizeof(_qdbp_hashtable));
    for (size_t i = 1; i < table->header.capacity * 2 + 1; i++) {
      new_table[i].field.method.code = NULL;
    }
    new_table->header.size = 0;
    new_table->header.capacity = table->header.capacity * 2;
    new_table->header.directory =
        _qdbp_malloc(sizeof(size_t) * (new_table->header.capacity));
    size_t tmp;
    _qdbp_field_ptr f;
    _QDBP_HT_ITER(table, f, tmp) { ht_insert_raw(new_table, f); }
    _qdbp_free(table->header.directory);
    _qdbp_free(table);
    table = new_table;
  }
  ht_insert_raw(table, fld);
  return table;
}

#endif
