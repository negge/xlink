#include <string.h>
#include <stdlib.h>
#include "internal.h"
#include "util.h"

void xlink_list_init(xlink_list *list, size_t size, int capacity) {
  memset(list, 0, sizeof(xlink_list));
  list->size = size;
  list->capacity = capacity;
  list->data = xlink_malloc(list->size*list->capacity);
}

void xlink_list_clear(xlink_list *list) {
  free(list->data);
}

int xlink_list_length(const xlink_list *list) {
  return list->length;
}

void xlink_list_empty(xlink_list *list) {
  list->length = 0;
}

void xlink_list_expand_capacity(xlink_list *list, int capacity) {
  if (list->capacity < capacity) {
    list->capacity = XLINK_MAX(capacity,
     XLINK_MAX(4, XLINK_MIN(list->capacity << 1, list->capacity + 1024)));
    list->data = xlink_realloc(list->data, list->capacity*list->size);
  }
}

void *xlink_list_get(const xlink_list *list, int index) {
  XLINK_ERROR(index < 0 || index >= list->length,
   ("Cannot get element at position %i, length = %i", index, list->length));
  return &list->data[index*list->size];
}

void xlink_list_set(xlink_list *list, int index, const void *element) {
  XLINK_ERROR(index < 0 || index >= list->length,
   ("Cannot set element at position %i, length = %i", index, list->length));
  memcpy(xlink_list_get(list, index), element, list->size);
}

int xlink_list_add(xlink_list *list, const void *element) {
  xlink_list_expand_capacity(list, list->length + 1);
  list->length++;
  xlink_list_set(list, list->length - 1, element);
  return list->length;
}

int xlink_list_add_all(xlink_list *list, const void *elements, int length) {
  xlink_list_expand_capacity(list, list->length + length);
  memcpy(&list->data[list->length*list->size], elements, length*list->size);
  list->length += length;
  return list->length;
}

int xlink_list_append(xlink_list *list, const xlink_list *elements) {
  XLINK_ERROR(list->size != elements->size,
   ("Cannot append elements with different size list = %i elements = %i",
   list->size, elements->size));
  return xlink_list_add_all(list, elements->data, elements->length);
}

void xlink_list_insert(xlink_list *list, int index, const void *element) {
  XLINK_ERROR(index < 0 || index > list->length,
   ("Cannot insert element at position %i, length = %i", index, list->length));
  xlink_list_expand_capacity(list, list->length + 1);
  list->length++;
  if (list->length - 1 > index) {
    memmove(xlink_list_get(list, index + 1), xlink_list_get(list, index),
     list->size*(list->length - 1 - index));
  }
  xlink_list_set(list, index, element);
}

void xlink_list_remove(xlink_list *list, int index) {
  XLINK_ERROR(index < 0 || index >= list->length,
   ("Cannot remove element at position %i, length = %i", index, list->length));
  list->length--;
  if (list->length > index) {
    memmove(xlink_list_get(list, index), xlink_list_get(list, index + 1),
     list->size*(list->length - index));
  }
}

void xlink_list_swap(xlink_list *list, int i, int j) {
  XLINK_ERROR(i < 0 || i >= list->length || j < 0 || j >= list->length,
   ("Cannot swap elements at %i and %i, length = %i", i, j, list->length));
  if (i != j) {
    void *a, *b, *c;
    xlink_list_expand_capacity(list, list->length + 1);
    a = xlink_list_get(list, i);
    b = xlink_list_get(list, j);
    c = &list->data[list->length*list->size];
    memcpy(c, a, list->size);
    memcpy(a, b, list->size);
    memcpy(b, c, list->size);
  }
}

void xlink_list_reverse(xlink_list *list, int start, int end) {
  int i, j;
  for (i = start, j = end; i < j; i++, j--) {
    xlink_list_swap(list, i, j);
  }
}

void xlink_list_sort(xlink_list *list, int (*cmp)(const void *, const void *)) {
  qsort(list->data, list->length, list->size, cmp);
}

void xlink_set_init(xlink_set *set, xlink_hash_code_func hash_code,
 xlink_equals_func equals, size_t value_size, int capacity, float load) {
  set->hash_code = hash_code;
  set->equals = equals;
  set->capacity = XLINK_MAX(4, capacity);
  set->load = load;
  set->table = xlink_malloc(set->capacity*sizeof(int));
  xlink_list_init(&set->entries, sizeof(xlink_entry), capacity);
  xlink_list_init(&set->values, value_size, capacity);
  xlink_set_reset(set);
}

void xlink_set_clear(xlink_set *set) {
  free(set->table);
  xlink_list_clear(&set->entries);
  xlink_list_clear(&set->values);
}

void xlink_set_reset(xlink_set *set) {
  memset(set->table, 0, set->capacity*sizeof(int));
  set->entries.length = set->values.length = 0;
  set->size = 0;
}

static int xlink_set_index(xlink_set *set, unsigned int hash) {
  return hash%set->capacity;
}

void xlink_set_resize(xlink_set *set, int capacity) {
  if (set->size == 0) {
    if (capacity < set->capacity) {
      set->capacity = capacity;
    }
    else {
      set->table = xlink_realloc(set->table, capacity*sizeof(int));
      memset(set->table, 0, capacity*sizeof(int));
      set->capacity = capacity;
    }
  }
  else {
    int *table;
    int i;
    table = xlink_malloc(capacity*sizeof(int));
    memset(table, 0, capacity*sizeof(int));
    i = set->capacity;
    set->capacity = capacity;
    while (i-- > 0) {
      int entry_index;
      entry_index = set->table[i];
      while (entry_index != 0) {
        xlink_entry *entry;
        int down;
        int index;
        entry = xlink_list_get(&set->entries, entry_index - 1);
        down = entry->down;
        index = xlink_set_index(set, entry->hash);
        entry->down = table[index];
        table[index] = entry_index;
        entry_index = down;
      }
    }
    free(set->table);
    set->table = table;
  }
}

void *xlink_set_add(xlink_set *set, const void *value) {
  xlink_entry entry;
  int index;
  if (set->size + 1 > set->capacity*set->load) {
    xlink_set_resize(set, set->capacity << 1);
  }
  entry.hash = set->hash_code(value);
  index = xlink_set_index(set, entry.hash);
  entry.down = set->table[index];
  XLINK_ERROR(
   xlink_list_length(&set->entries) != xlink_list_length(&set->values),
   ("Invalid xlink_set state, entries length = %i but values length = %i\n",
   xlink_list_length(&set->entries), xlink_list_length(&set->values)));
  xlink_list_add(&set->entries, &entry);
  xlink_list_add(&set->values, value);
  set->table[index] = xlink_list_length(&set->entries);
  set->size++;
  return xlink_list_get(&set->values, set->table[index] - 1);
}

int xlink_set_remove(xlink_set *set, const void *key) {
  unsigned int hash;
  int *index;
  hash = set->hash_code(key);
  index = &set->table[xlink_set_index(set, hash)];
  while (*index != 0) {
    xlink_entry *entry;
    void *value;
    entry = xlink_list_get(&set->entries, *index - 1);
    value = xlink_list_get(&set->values, *index - 1);
    if (set->equals == NULL ||
     (entry->hash == hash && set->equals(key, value))) {
      int old;
      int last;
      /* Move the entry / value from the end into the empty slot and reindex */
      old = *index - 1;
      *index = entry->down;
      XLINK_ERROR(
       xlink_list_length(&set->entries) != xlink_list_length(&set->values),
       ("Invalid xlink_set state, entries = %i but values = %i\n",
       xlink_list_length(&set->entries), xlink_list_length(&set->values)));
      last = xlink_list_length(&set->entries) - 1;
      if (old != last) {
        xlink_list_set(&set->entries, old, xlink_list_get(&set->entries, last));
        xlink_list_set(&set->values, old, xlink_list_get(&set->values, last));
        index = &set->table[xlink_set_index(set, entry->hash)];
        XLINK_ERROR(*index == 0,
         ("Invalid xlink_set state, index = 0 but should contain an entry"));
        while (*index != 0) {
          if (*index == last + 1) {
            *index = old + 1;
            break;
          }
          index = &entry->down;
        }
        XLINK_ERROR(*index == 0,
         ("Invalid xlink_set state, cound not find last entry to reindex"));
      }
      set->entries.length--;
      set->values.length--;
      set->size--;
      return EXIT_SUCCESS;
    }
    index = &entry->down;
  }
  return EXIT_FAILURE;
}

void *xlink_set_get(xlink_set *set, void *key) {
  unsigned int hash;
  int index;
  hash = set->hash_code(key);
  index = set->table[xlink_set_index(set, hash)];
  while (index != 0) {
    xlink_entry *entry;
    void *value;
    entry = xlink_list_get(&set->entries, index - 1);
    value = xlink_list_get(&set->values, index - 1);
    if (set->equals == NULL ||
     (entry->hash == hash && set->equals(key, value))) {
      return value;
    }
    index = entry->down;
  }
  return NULL;
}

void *xlink_set_put(xlink_set *set, void *value) {
  xlink_set_remove(set, value);
  return xlink_set_add(set, value);
}
