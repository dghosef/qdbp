#ifndef HASHTABLE_C
#define HASHTABLE_C
#include <string.h>

#include "runtime.h"

// Compute `x % n` assuming that `n` is a power of 2.
static size_t fast_mod(size_t x, size_t n) {
  _qdbp_assert((n & (n - 1)) == 0);
  return x & (n - 1);
}

_qdbp_hashtable* _qdbp_ht_new(size_t capacity) {
  _qdbp_hashtable* table =
      (_qdbp_hashtable*)_qdbp_malloc((capacity + 1) * sizeof(_qdbp_hashtable));
  for (size_t i = 1; i <= capacity; i++) {
    table[i].field.method.code = NULL;
  }
  table->header = (struct _qdbp_hashtable_header){
      .size = 0,
      .capacity = capacity,
      .directory = _qdbp_malloc(capacity * sizeof(size_t))};
  return table;
}

void _qdbp_ht_del(_qdbp_hashtable* table) {
  _qdbp_free(table->header.directory);
  _qdbp_free(table);
}

_qdbp_hashtable* _qdbp_ht_duplicate(_qdbp_hashtable* table) {
  _qdbp_hashtable* new_table =
      _qdbp_malloc(sizeof(_qdbp_hashtable) +
                   (sizeof(_qdbp_hashtable) * table->header.capacity));
  _qdbp_memcpy(new_table, table,
               sizeof(_qdbp_hashtable) +
                   (sizeof(_qdbp_hashtable) * (table->header.capacity)));
  new_table->header.directory =
      _qdbp_malloc(sizeof(size_t) * (table->header.capacity));
  _qdbp_memcpy(new_table->header.directory, table->header.directory,
               sizeof(size_t) * table->header.size);
  return new_table;
}

_qdbp_field_ptr _qdbp_ht_find(_qdbp_hashtable* table, _qdbp_label_t label) {
  size_t index = fast_mod(label, table->header.capacity);
  _qdbp_assert(index <= table->header.capacity);
  _qdbp_hashtable* fields = table + 1;
  while (fields[index].field.label != label) {
    index = fast_mod(index + 1, table->header.capacity);
    _qdbp_assert(fields[index].field.method.code != NULL);
    _qdbp_assert(index < table->header.capacity);
  }
  return &(fields[index].field);
}

static void ht_insert_no_resize(_qdbp_hashtable* table,
                                const _qdbp_field_ptr fld) {
  size_t index = fast_mod(fld->label, table->header.capacity);
  _qdbp_hashtable* fields = table + 1;
  _qdbp_assert(index <= table->header.capacity);
  while (fields[index].field.method.code != NULL) {
    index = fast_mod(index + 1, table->header.capacity);
    _qdbp_assert(index <= table->header.capacity);
  }
  fields[index].field = *fld;
  table->header.directory[table->header.size] = index + 1;
  table->header.size++;
}

_qdbp_hashtable* _qdbp_ht_insert(_qdbp_hashtable* table,
                                 const _qdbp_field_ptr fld) {
  _qdbp_assert(fld->method.code != NULL);
  // resize the ht
  if (table->header.size * _QDBP_LOAD_FACTOR_NUM >=
      table->header.capacity * _QDBP_LOAD_FACTOR_DEN) {
    _qdbp_hashtable* new_table = _qdbp_ht_new(table->header.capacity * 2);
    size_t tmp;
    _qdbp_field_ptr f;
    _QDBP_HT_ITER(table, f, tmp) { ht_insert_no_resize(new_table, f); }
    _qdbp_ht_del(table);
    table = new_table;
  }
  ht_insert_no_resize(table, fld);
  return table;
}

size_t _qdbp_ht_size(_qdbp_hashtable* table) {
  _qdbp_assert(table);
  return table->header.size;
}
#endif
