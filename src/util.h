#ifndef _XLINK_util_h
#define _XLINK_util_h

typedef struct xlink_list xlink_list;

struct xlink_list {
  unsigned char *data;
  size_t size;
  int length;
  int capacity;
};

void xlink_list_init(xlink_list *list, size_t size, int capacity);
void xlink_list_clear(xlink_list *list);
int xlink_list_length(const xlink_list *list);
void xlink_list_empty(xlink_list *list);
void xlink_list_expand_capacity(xlink_list *list, int capacity);
void *xlink_list_get(const xlink_list *list, int index);
void xlink_list_set(xlink_list *list, int index, const void *element);
int xlink_list_add(xlink_list *list, const void *element);
int xlink_list_add_all(xlink_list *list, const void *elements, int length);
int xlink_list_append(xlink_list *list, const xlink_list *elements);
void xlink_list_insert(xlink_list *list, int index, const void *element);
void xlink_list_remove(xlink_list *list, int index);
void xlink_list_swap(xlink_list *list, int i, int j);
void xlink_list_reverse(xlink_list *list, int start, int end);
void xlink_list_sort(xlink_list *list, int (*cmp)(const void *, const void *));

#endif
