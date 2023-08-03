#include <string.h>

#include "runtime.h"

// Compute `x % n` assuming that `n` is a power of 2.
static size_t fast_mod(size_t x, size_t n) {
  _qdbp_assert((n & (n - 1)) == 0);
  return x & (n - 1);
}

_qdbp_hashtable_t* _qdbp_ht_new(size_t capacity) {
  _qdbp_hashtable_t* table =
      (_qdbp_hashtable_t*)_qdbp_malloc((capacity + 1) * sizeof(_qdbp_hashtable_t));
  for (size_t i = 1; i <= capacity; i++) {
    table[i].field.method.code = NULL;
  }
  table->header = (struct _qdbp_hashtable_header){
      .size = 0,
      .capacity = capacity,
      .directory = _qdbp_malloc(capacity * sizeof(size_t))};
  return table;
}

void _qdbp_ht_del(_qdbp_hashtable_t* table) {
  if(table) {
    _qdbp_free(table->header.directory);
    _qdbp_free(table);
  }
}

_qdbp_hashtable_t* _qdbp_ht_duplicate(_qdbp_hashtable_t* table) {
  if (!table) {
    return NULL;
  }
  _qdbp_hashtable_t* new_table =
      _qdbp_malloc(sizeof(_qdbp_hashtable_t) +
                   (sizeof(_qdbp_hashtable_t) * table->header.capacity));
  _qdbp_memcpy(new_table, table,
               sizeof(_qdbp_hashtable_t) +
                   (sizeof(_qdbp_hashtable_t) * (table->header.capacity)));
  new_table->header.directory =
      _qdbp_malloc(sizeof(size_t) * (table->header.capacity));
  _qdbp_memcpy(new_table->header.directory, table->header.directory,
               sizeof(size_t) * table->header.size);
  return new_table;
}

_qdbp_field_ptr _qdbp_ht_find(_qdbp_hashtable_t* table, _qdbp_label_t label) {
  _qdbp_assert(table);
  size_t index = fast_mod(label, table->header.capacity);
  _qdbp_assert(index <= table->header.capacity);
  _qdbp_hashtable_t* fields = table + 1;
  while (fields[index].field.label != label) {
    index = fast_mod(index + 1, table->header.capacity);
    _qdbp_assert(fields[index].field.method.code != NULL);
    _qdbp_assert(index < table->header.capacity);
  }
  return &(fields[index].field);
}

_qdbp_field_ptr _qdbp_ht_find_opt(_qdbp_hashtable_t* table, _qdbp_label_t label) {
  if (!table) {
    return NULL;
  }
  size_t start_index = fast_mod(label, table->header.capacity);
  size_t index = start_index;
  _qdbp_assert(index <= table->header.capacity);
  _qdbp_hashtable_t* fields = table + 1;
  while (fields[index].field.label != label) {
    index = fast_mod(index + 1, table->header.capacity);
    if (fields[index].field.method.code == NULL || index == start_index) {
      return NULL;
    }
    _qdbp_assert(index < table->header.capacity);
  }
  return fields[index].field.method.code == NULL ? NULL : &(fields[index].field);
}

static void ht_insert_no_resize(_qdbp_hashtable_t* table,
                                const _qdbp_field_ptr fld) {
  _qdbp_assert(table);
  size_t index = fast_mod(fld->label, table->header.capacity);
  _qdbp_hashtable_t* fields = table + 1;
  _qdbp_assert(index <= table->header.capacity);
  while (fields[index].field.method.code != NULL) {
    index = fast_mod(index + 1, table->header.capacity);
    _qdbp_assert(index <= table->header.capacity);
  }
  fields[index].field = *fld;
  table->header.directory[table->header.size] = index + 1;
  table->header.size++;
}

_qdbp_hashtable_t* _qdbp_ht_insert(_qdbp_hashtable_t* table,
                                 const _qdbp_field_ptr fld) {
  if (!table) {
    table = _qdbp_ht_new(_QDBP_HT_DEFAULT_CAPACITY);
  }
  _qdbp_assert(fld->method.code != NULL);
  // resize the ht
  if (table->header.size * _QDBP_LOAD_FACTOR_NUM >=
      table->header.capacity * _QDBP_LOAD_FACTOR_DEN) {
    _qdbp_hashtable_t* new_table = _qdbp_ht_new(table->header.capacity * 2);
    _QDBP_HT_ITER(table, f, tmp) { ht_insert_no_resize(new_table, f); }
    _qdbp_ht_del(table);
    table = new_table;
  }
  ht_insert_no_resize(table, fld);
  return table;
}

size_t _qdbp_ht_size(_qdbp_hashtable_t* table) {
  if (!table) {
    return 0;
  }
  return table->header.size;
}
