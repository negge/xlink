#include <string.h>
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
