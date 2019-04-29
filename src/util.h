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

typedef struct xlink_entry xlink_entry;

struct xlink_entry {
  unsigned int hash;
  int down;
};

typedef unsigned int (*xlink_hash_code_func)(const void *);
typedef int (*xlink_equals_func)(const void *, const void *);

typedef struct xlink_set xlink_set;

struct xlink_set {
  xlink_hash_code_func hash_code;
  xlink_equals_func equals;
  int capacity;
  float load;
  int *table;
  int size;
  xlink_list entries;
  xlink_list values;
};

void xlink_set_init(xlink_set *set, xlink_hash_code_func hash_code,
 xlink_equals_func equals, size_t value_size, int capacity, float load);
void xlink_set_clear(xlink_set *set);
void xlink_set_reset(xlink_set *set);
void xlink_set_resize(xlink_set *set, int capacity);
void *xlink_set_add(xlink_set *set, const void *value);
int xlink_set_remove(xlink_set *set, const void *key);
void *xlink_set_get(xlink_set *set, void *key);
void *xlink_set_put(xlink_set *set, void *value);

#endif
