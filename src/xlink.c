#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <getopt.h>
#include <math.h>
#include <float.h>

typedef enum {
  OMF_THEADR  = 0x80,  // Translator Header Record
  OMF_LHEADR  = 0x82,  // Library Module Header Record
  OMF_MODEND  = 0x8A,  // (0x8B) Module End Record
  OMF_EXTDEF  = 0x8C,  // External Names Definition Record
  OMF_PUBDEF  = 0x90,  // (0x91) Public Names Definition Record
  OMF_FIXUPP  = 0x9C,  // (0x9D) Fixup Record
  OMF_GRPDEF  = 0x9A,  // Group Definition Record
  OMF_LNAMES  = 0x96,  // List of Names Record
  OMF_SEGDEF  = 0x98,  // (0x99) Segment Definition Record
  OMF_LEDATA  = 0xA0,  // (0xA1) Logical Enumerated Data Record
  OMF_LIDATA  = 0xA2,  // (0xA3) Logical Iterated Data Record
  OMF_COMDEF  = 0xB0,  // Communal Names Definition Record
  OMF_LEXTDEF = 0xB4,  // Local External Names Definition Record
  OMF_LPUBDEF = 0xB6,  // (0xB7) Local Public Names Definition Record
  OMF_CEXTDEF = 0xBC,  // COMDAT External Names Definition Record
  OMF_COMDAT  = 0xC2,  // (0xC3) Initialized Communal Data Record
  OMF_LLNAMES = 0xCA,  // Local Logical Names Definition Record
  OMF_LIBHDR  = 0xF0,  // Library Header Record
  OMF_LIBEND  = 0xF1,  // Library End Record
} xlink_omf_record_type;

typedef enum {
  OMF_DATA_TYPE_FAR  = 0x61,
  OMF_DATA_TYPE_NEAR = 0x62
} xlink_omf_data_type;

typedef enum {
  OMF_FRAME_SEG  = 0,
  OMF_FRAME_GRP  = 1,
  OMF_FRAME_EXT  = 2,
  OMF_FRAME_ABS  = 3,
  OMF_FRAME_LOC  = 4,
  OMF_FRAME_TARG = 5,
  OMF_FRAME_FLAT = 6,
} xlink_omf_frame;

typedef enum {
  OMF_TARGET_SEG_DISP = 0,  // Segment index with displacement
  OMF_TARGET_GRP_DISP = 1,  // Group index with displacement
  OMF_TARGET_EXT_DISP = 2,  // Extern index with displacement
  OMF_TARGET_ABS_DISP = 3,  // Absolute frame number with displacement
  OMF_TARGET_SEG      = 4,  // Segment index, no displacement
  OMF_TARGET_GRP      = 5,  // Group index, no displacement
  OMF_TARGET_EXT      = 6,  // Extern index, no displacement
  OMF_TARGET_ABS      = 7,  // Absolute frame number, no displacement
} xlink_omf_target;

typedef enum {
  OMF_LOCATION_8BIT          =  0,  // 8-bit or low byte of 16-bit offset
  OMF_LOCATION_16BIT         =  1,  // 16-bit offset
  OMF_LOCATION_SEGMENT       =  2,  // 16-bit base
  OMF_LOCATION_FAR           =  3,  // 16-bit base + 16 big segment
  OMF_LOCATION_8BIT_HIGH     =  4,  // 8-bit or high byte 16-bit offset
  OMF_LOCATION_16BIT_LOADER  =  5,  // 16-bit, loader resolved
  OMF_LOCATION_48BIT_PHARLAP =  6,  // 16-bit base + 32-bit offset, pharlap
  OMF_LOCATION_32BIT         =  9,  // 32-bit offset
  OMF_LOCATION_48BIT         = 11,  // 16-bit base + 32-bit offset
  OMF_LOCATION_32BIT_LOADER  = 13,  // 32-bit, loader resolved
} xlink_omf_location;

typedef enum {
  OMF_MODE_RELATIVE = 0,
  OMF_MODE_EXPLICIT = 1
} xlink_omf_relocation_mode;

typedef enum {
  OMF_SEGMENT_CODE,
  OMF_SEGMENT_DATA,
  OMF_SEGMENT_BSS
} xlink_segment_class;

typedef enum {
  OMF_SEGMENT_ABS,
  OMF_SEGMENT_BYTE,
  OMF_SEGMENT_WORD,
  OMF_SEGMENT_PARA,
  OMF_SEGMENT_PAGE,
  OMF_SEGMENT_DWORD
} xlink_segment_align;

typedef enum {
  OMF_SEGMENT_USE16,
  OMF_SEGMENT_USE32
} xlink_segment_use;

static const char *XLINK_SEGMENT_CLASS_NAME[] = {
  "CODE",
  "DATA",
  "BSS",
};

typedef union xlink_omf_attribute xlink_omf_attribute;

union xlink_omf_attribute {
  uint8_t b;
  struct {
    /* Packed byte with segment attributes */
    uint8_t proc:1, big:1, combine:3, align:3;
  };
};

typedef union xlink_omf_fixup_thread xlink_omf_fixup_thread;

union xlink_omf_fixup_thread {
  uint8_t b;
  struct {
    /* Packed byte with FIXUP thread data, high = 0 always */
    uint8_t thread:2, method:3, is_frame:1, high:1;
  };
};

typedef union xlink_omf_fixup_locat xlink_omf_fixup_locat;

union xlink_omf_fixup_locat {
  struct {
    uint8_t b1;
    uint8_t b0;
  };
  struct {
    /* Packed byte with FIXUP location data, high = 1 always */
    uint16_t offset:10, location:4, mode:1, high:1;
  };
};

typedef union xlink_omf_fixup_fixdata xlink_omf_fixup_fixdata;

union xlink_omf_fixup_fixdata {
  uint8_t b;
  struct {
    /* Packed byte with FIXUP fixdata data */
    uint8_t target:2, no_disp:1, th_target:1, frame:3, th_frame:1;
  };
};

static const char *OMF_SEGDEF_ALIGN[] = {
  "ABS",
  "BYTE",
  "WORD",
  "PARA",
  "PAGE",
  "DWORD",
  "UNSUPPORTED",
  "UNDEFINED"
};

static const char *OMF_SEGDEF_COMBINE[] = {
  "private",
  "reserved",
  "public",
  "reserved",
  "public (4)",
  "stack",
  "common",
  "public (7)"
};

static const char *OMF_SEGDEF_USE[] = {
  "USE16",
  "USE32"
};

static const char *OMF_FIXUP_MODE[] = {
  "Relative",
  "Explicit"
};

static const char *OMF_FIXUP_LOCATION[] = {
  "8-bit",
  "16-bit",
  "16-bit (base)",
  "16:16 far",
  "8-bit (high)",
  "16-bit (loader)",
  "16:32 far (pharlap)",
  "UNDEFINED",
  "UNDEFINED",
  "32-bit",
  "UNDEFINED",
  "16:32 far",
  "UNDEFINED",
  "32-bit (loader)"
};

static const int OMF_FIXUP_SIZE[] = {
  1, 2, 2, 4, 1, 2, 6, 0, 0, 4, 0, 6, 0, 4
};

static const char *xlink_omf_record_get_name(xlink_omf_record_type type) {
  switch (type & ~1) {
    case OMF_THEADR : return "THEADR";
    case OMF_LHEADR : return "LHEADR";
    case OMF_MODEND : return "MODEND";
    case OMF_EXTDEF : return "EXTDEF";
    case OMF_PUBDEF : return "PUBDEF";
    case OMF_FIXUPP : return "FIXUPP";
    case OMF_GRPDEF : return "GRPDEF";
    case OMF_LNAMES : return "LNAMES";
    case OMF_SEGDEF : return "SEGDEF";
    case OMF_LEDATA : return "LEDATA";
    case OMF_LIDATA : return "LIDATA";
    case OMF_COMDEF : return "COMDEF";
    case OMF_LEXTDEF : return "LEXTDEF";
    case OMF_LPUBDEF : return "LPUBDEF";
    case OMF_CEXTDEF : return "CEXTDEF";
    case OMF_COMDAT : return "COMDAT";
    case OMF_LLNAMES : return "LLNAMES";
    default : return "??????";
  }
}

static const char *xlink_omf_record_get_desc(xlink_omf_record_type type) {
  switch (type & ~1) {
    case OMF_THEADR : return "Translator Header";
    case OMF_LHEADR : return "Library Module Header";
    case OMF_MODEND : return "Module End";
    case OMF_EXTDEF : return "External Names Definition";
    case OMF_PUBDEF : return "Public Names Definition";
    case OMF_FIXUPP : return "Fixup";
    case OMF_GRPDEF : return "Group Definition";
    case OMF_LNAMES : return "List of Names";
    case OMF_SEGDEF : return "Segment Definition";
    case OMF_LEDATA : return "Logical Enumerated Data";
    case OMF_LIDATA : return "Logical Iterated Data";
    case OMF_COMDEF : return "Communal Names Definition";
    case OMF_LEXTDEF : return "Local External Names Definition";
    case OMF_LPUBDEF : return "Local Public Names";
    case OMF_CEXTDEF : return "COMDAT External Names Definition";
    case OMF_COMDAT : return "Initialized Communal Data";
    case OMF_LLNAMES : return "Local Logical Names Definition";
    default : return "Unknown or Unsupported";
  }
}

typedef struct xlink_list xlink_list;

struct xlink_list {
  unsigned char *data;
  size_t size;
  int length;
  int capacity;
};

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

typedef struct xlink_file xlink_file;

struct xlink_file {
  const char *name;
  unsigned int size;
  unsigned int pos;
  const unsigned char *buf;
};

#include "stubs.h"

typedef struct xlink_omf_record xlink_omf_record;

struct xlink_omf_record {
  const unsigned char *buf;
  int idx;
  unsigned char type;
  unsigned short size;
};

typedef char xlink_string[256];

typedef struct xlink_module xlink_module;

typedef struct xlink_name xlink_name;

struct xlink_name {
  int index;
  xlink_module *module;
  xlink_string str;
};

typedef struct xlink_group xlink_group;
typedef struct xlink_public xlink_public;
typedef struct xlink_data xlink_data;
typedef struct xlink_reloc xlink_reloc;

#define SEG_HAS_DATA 0x1

typedef struct xlink_segment xlink_segment;

struct xlink_segment {
  int index;
  xlink_module *module;
  xlink_omf_attribute attrib;
  unsigned int frame;
  unsigned int offset;
  int length;
  xlink_name *name;
  xlink_name *class;
  unsigned int info;
  unsigned char *data;
  unsigned char *mask;
  xlink_group *group;
  xlink_public **publics;
  int npublics;
  xlink_data **datas;
  int ndatas;
  xlink_reloc **relocs;
  int nrelocs;
  int start;
};

#define CEIL2(len, bits) (((len) + ((1 << (bits)) - 1)) >> (bits))

#define XLINK_SET_BIT(buf, i, b) ((buf)[(i) >> 3] = \
 (((buf)[(i) >> 3] & ~(1 << ((i) & 7))) | (!!(b) << ((i) & 7))))
#define XLINK_GET_BIT(buf, i)    ((buf)[(i) >> 3] >> ((i) & 7) & 1)

#define ALIGN2(len, bits) (((len) + ((1 << (bits)) - 1)) & ~((1 << (bits)) - 1))

struct xlink_group {
  int index;
  xlink_module *module;
  xlink_name *name;
  xlink_segment **segments;
  int nsegments;
};

struct xlink_public {
  int index;
  xlink_module *module;
  xlink_group *group;
  xlink_segment *segment;
  int base_frame;
  xlink_string name;
  int offset;
  int type_idx;
  int is_local;
};

typedef struct xlink_extern xlink_extern;

struct xlink_extern {
  int index;
  xlink_module *module;
  xlink_string name;
  int type_idx;
  int is_local;
  xlink_public *public;
};

struct xlink_data {
  int index;
  xlink_module *module;
  xlink_segment *segment;
  unsigned int offset;
  int is_iterated;
  int length;
  unsigned char *data;
  unsigned char *mask;
};

typedef struct xlink_addend xlink_addend;

struct xlink_addend {
  unsigned short segment;
  int offset;
};

struct xlink_reloc {
  int index;
  xlink_module *module;
  xlink_segment *segment;
  unsigned int offset;
  int mode;
  xlink_omf_location location;
  xlink_omf_frame frame;
  xlink_omf_target target;
  int frame_idx;
  int target_idx;
  unsigned int disp;
  xlink_addend addend;
};

typedef struct xlink_omf xlink_omf;

struct xlink_omf {
  xlink_omf_record *records;
  int nrecords;
};

typedef struct xlink_library xlink_library;

typedef struct xlink_binary xlink_binary;

struct xlink_module {
  const char *filename;
  int index;
  union {
    xlink_binary *binary;
    xlink_library *library;
  };
  xlink_string source;
  xlink_name **names;
  int nnames;
  xlink_segment **segments;
  int nsegments;
  xlink_group **groups;
  int ngroups;
  xlink_public **publics;
  int npublics;
  xlink_extern **externs;
  int nexterns;
  xlink_data **datas;
  int ndatas;
  xlink_reloc **relocs;
  int nrelocs;
};

struct xlink_library {
  const char *filename;
  int index;
  xlink_binary *binary;
  int page_size;
  xlink_module **modules;
  int nmodules;
};

struct xlink_binary {
  char *output;
  char *entry;
  char *init;
  int hash_table_memory;
  char *map;
  xlink_module **modules;
  int nmodules;
  xlink_library **librarys;
  int nlibrarys;
  xlink_segment **segments;
  int nsegments;
  xlink_extern **externs;
  int nexterns;
  /* Flag indicating that the main program is 32-bit */
  int is_32bit;
};

typedef unsigned int xlink_prob;

typedef struct xlink_model xlink_model;

struct xlink_model {
  unsigned char mask;
  int weight;
  unsigned int state;
};

typedef struct xlink_match xlink_match;

struct xlink_match {
  /* Used in the match_simple_hash_code() function */
  unsigned int salt;
  unsigned char partial;
  unsigned char mask;
  unsigned char buf[8];
  unsigned char counts[2];
};

typedef struct xlink_context xlink_context;

struct xlink_context {
  unsigned char buf[8];
  xlink_list *models;
  xlink_set matches;
};

typedef struct xlink_bitstream xlink_bitstream;

struct xlink_bitstream {
  xlink_list bytes;
  int bits;
};

typedef unsigned char xlink_counts[256][2];

typedef struct xlink_modeler xlink_modeler;

struct xlink_modeler {
  xlink_list bytes;
  xlink_list counts;
  xlink_set matches;
};

void xlink_log(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  fprintf(stderr, "Error: ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
}

#define XLINK_LOG(err) xlink_log err

#define XLINK_ERROR(cond, err) \
  do { \
    if (cond) { \
      XLINK_LOG(err); \
      exit(-1); \
    } \
  } \
  while (0)

void *xlink_malloc(size_t size) {
  void *ptr;
  ptr = malloc(size);
  XLINK_ERROR(ptr == NULL, ("Insufficient memory for %i byte malloc", size));
  memset(ptr, 0, size);
  return ptr;
}

void *xlink_realloc(void *ptr, size_t size) {
  ptr = realloc(ptr, size);
  XLINK_ERROR(ptr == NULL, ("Insufficient memory for %i byte realloc", size));
  return ptr;
}

#define XLINK_MIN(a, b) ((a) <= (b) ? (a) : (b))
#define XLINK_MAX(a, b) ((a) >= (b) ? (a) : (b))

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

void xlink_set_reset(xlink_set *set) {
  memset(set->table, 0, set->capacity*sizeof(int));
  set->entries.length = set->values.length = 0;
  set->size = 0;
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

int xlink_set_index(xlink_set *set, unsigned int hash) {
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

void xlink_omf_init(xlink_omf *omf) {
  memset(omf, 0, sizeof(xlink_omf));
}

void xlink_omf_clear(xlink_omf *omf) {
  free(omf->records);
}

#define XLINK_LIST_FUNCS(parent, child)                                       \
int xlink_##parent##_add_##child(xlink_##parent *p, xlink_##child *c) {       \
  p->n##child##s++;                                                           \
  p->child##s =                                                               \
   xlink_realloc(p->child##s, p->n##child##s*sizeof(xlink_##child *));        \
  p->child##s[p->n##child##s - 1] = c;                                        \
  return p->n##child##s;                                                      \
}                                                                             \
                                                                              \
xlink_##child *xlink_##parent##_get_##child(xlink_##parent *p, int index) {   \
  XLINK_ERROR(index < 1 || index > p->n##child##s,                            \
   ("Could not get " #child " %i, n" #child " = %i", index, p->n##child##s)); \
  return p->child##s[index - 1];                                              \
}                                                                             \
                                                                              \
int xlink_##parent##_has_##child(xlink_##parent *p, xlink_##child *c) {       \
  int i;                                                                      \
  for (i = 0; i < p->n##child##s; i++) {                                      \
    if (p->child##s[i] == c) {                                                \
      return 1;                                                               \
    }                                                                         \
  }                                                                           \
  return 0;                                                                   \
}

#define XLINK_LIST_ADD(parent, child, p, c)                                   \
  (c)->index = (p)->n##child##s + 1;                                          \
  (c)->parent = p;                                                            \
  xlink_##parent##_add_##child(p, c);

XLINK_LIST_FUNCS(segment, public);
XLINK_LIST_FUNCS(segment, data);
XLINK_LIST_FUNCS(segment, reloc);

void xlink_segment_clear(xlink_segment *segment) {
  free(segment->data);
  free(segment->mask);
  free(segment->publics);
  free(segment->datas);
  free(segment->relocs);
  memset(segment, 0, sizeof(xlink_segment));
}

const char *xlink_segment_get_name(xlink_segment *seg) {
  static char name[256];
  sprintf(name, "%s:%i", seg->name->str, seg->index);
  return name;
}

const char *xlink_segment_get_class_name(xlink_segment *seg) {
  static char name[256];
  sprintf(name, "'%s'", seg->class->str);
  return name;
}

int xlink_segment_get_alignment(xlink_segment *seg) {
  switch (seg->attrib.align) {
    case OMF_SEGMENT_BYTE : {
      return 0;
    }
    case OMF_SEGMENT_WORD : {
      return 1;
    }
    case OMF_SEGMENT_DWORD : {
      return 2;
    }
    case OMF_SEGMENT_PARA : {
      return 4;
    }
    case OMF_SEGMENT_PAGE : {
      return 12;
    }
    default : {
      XLINK_ERROR(1,
       ("Unsupported alignment %s:%i for segment %s",
       OMF_SEGDEF_ALIGN[seg->attrib.align], seg->attrib.align,
       xlink_segment_get_name(seg)));
    }
  }
}

void xlink_segment_reset_mask(xlink_segment *seg) {
  memset(seg->mask, 0, CEIL2(seg->length, 3));
}

XLINK_LIST_FUNCS(group, segment);

void xlink_group_clear(xlink_group *grp) {
  free(grp->segments);
  memset(grp, 0, sizeof(xlink_group));
}

const char *xlink_group_get_name(xlink_group *grp) {
  static char name[256];
  sprintf(name, "%s:%i", grp->name->str, grp->index);
  return name;
}

const char *xlink_extern_get_name(xlink_extern *ext) {
  static char name[256];
  sprintf(name, "%s:%i", ext->name, ext->index);
  return name;
}

void xlink_data_clear(xlink_data *dat) {
  free(dat->data);
  free(dat->mask);
}

void xlink_data_reset_mask(xlink_data *dat) {
  memset(dat->mask, 0, CEIL2(dat->length, 3));
}

const char *xlink_reloc_get_addend(xlink_reloc *rel) {
  static char str[256];
  switch (rel->location) {
    case OMF_LOCATION_8BIT :
    case OMF_LOCATION_8BIT_HIGH : {
      sprintf(str, "0x%02x", (uint8_t)rel->addend.offset);
      break;
    }
    case OMF_LOCATION_16BIT :
    case OMF_LOCATION_SEGMENT :
    case OMF_LOCATION_16BIT_LOADER : {
      sprintf(str, "0x%04x", (uint16_t)rel->addend.offset);
      break;
    }
    case OMF_LOCATION_32BIT :
    case OMF_LOCATION_32BIT_LOADER : {
      sprintf(str, "0x%08x", (uint32_t)rel->addend.offset);
      break;
    }
    case OMF_LOCATION_FAR :
    case OMF_LOCATION_48BIT :
    case OMF_LOCATION_48BIT_PHARLAP : {
      sprintf(str, "0x%04x:0x%08x", (uint16_t)rel->addend.segment,
       (uint32_t)rel->addend.offset);
      break;
    }
    default : {
      XLINK_ERROR(1, ("Unsupported location %i", rel->location));
    }
  }
  return str;
}

void xlink_module_clear(xlink_module *mod) {
  int i;
  for (i = 0; i < mod->nnames; i++) {
    free(mod->names[i]);
  }
  free(mod->names);
  for (i = 0; i < mod->nsegments; i++) {
    xlink_segment_clear(mod->segments[i]);
    free(mod->segments[i]);
  }
  free(mod->segments);
  for (i = 0; i < mod->ngroups; i++) {
    xlink_group_clear(mod->groups[i]);
    free(mod->groups[i]);
  }
  free(mod->groups);
  for (i = 0; i < mod->npublics; i++) {
    free(mod->publics[i]);
  }
  free(mod->publics);
  for (i = 0; i < mod->nexterns; i++) {
    free(mod->externs[i]);
  }
  free(mod->externs);
  for (i = 0; i < mod->ndatas; i++) {
    xlink_data_clear(mod->datas[i]);
    free(mod->datas[i]);
  }
  free(mod->datas);
  for (i = 0; i < mod->nrelocs; i++) {
    free(mod->relocs[i]);
  }
  free(mod->relocs);
  memset(mod, 0, sizeof(xlink_module));
}

void xlink_library_clear(xlink_library *lib) {
  int i;
  for (i = 0; i < lib->nmodules; i++) {
    xlink_module_clear(lib->modules[i]);
    free(lib->modules[i]);
  }
  free(lib->modules);
}

void xlink_binary_init(xlink_binary *bin) {
  memset(bin, 0, sizeof(xlink_binary));
  bin->entry = "main_";
  bin->hash_table_memory = 12*1024*1024;
}

void xlink_binary_clear(xlink_binary *bin) {
  int i;
  for (i = 0; i < bin->nmodules; i++) {
    xlink_module_clear(bin->modules[i]);
    free(bin->modules[i]);
  }
  for (i = 0; i < bin->nlibrarys; i++) {
    xlink_library_clear(bin->librarys[i]);
    free(bin->librarys[i]);
  }
  free(bin->modules);
  free(bin->librarys);
  free(bin->segments);
  free(bin->externs);
  memset(bin, 0, sizeof(xlink_binary));
}

int xlink_omf_add_record(xlink_omf *omf, xlink_omf_record *rec) {
  XLINK_ERROR(
   omf->nrecords == 0 && rec->type != OMF_THEADR && rec->type != OMF_LHEADR,
   ("%s not the first record in module", xlink_omf_record_get_name(rec->type)));
  omf->nrecords++;
  omf->records =
   xlink_realloc(omf->records, omf->nrecords*sizeof(xlink_omf_record));
  omf->records[omf->nrecords - 1] = *rec;
  return omf->nrecords;
}

XLINK_LIST_FUNCS(module, name);
XLINK_LIST_FUNCS(module, segment);
XLINK_LIST_FUNCS(module, group);
XLINK_LIST_FUNCS(module, public);
XLINK_LIST_FUNCS(module, data);
XLINK_LIST_FUNCS(module, extern);
XLINK_LIST_FUNCS(module, reloc);

xlink_segment *xlink_module_find_segment(xlink_module *mod, const char *name) {
  xlink_segment *ret;
  int i;
  ret = NULL;
  for (i = 1; i <= mod->nsegments; i++) {
    xlink_segment *seg;
    seg = xlink_module_get_segment(mod, i);
    if (strcmp(name, seg->name->str) == 0) {
      XLINK_ERROR(ret != NULL,
       ("Duplicate segment definition found for name %s in %s", name,
       mod->filename));
      ret = seg;
    }
  }
  XLINK_ERROR(ret == NULL,
   ("Could not find segment definition %s in %s", name, mod->filename));
  return ret;
}

xlink_public *xlink_module_find_public(xlink_module *mod, const char *symb) {
  xlink_public *ret;
  int i;
  ret = NULL;
  for (i = 1; i <= mod->npublics; i++) {
    xlink_public *pub;
    pub = xlink_module_get_public(mod, i);
    /* TODO: Why was this check added? */
    if (/*pub->is_local &&*/ strcmp(symb, pub->name) == 0) {
      XLINK_ERROR(ret != NULL,
       ("Duplicate local public definition found for symbol %s in %s", symb,
       mod->filename));
      ret = pub;
    }
  }
  XLINK_ERROR(ret == NULL,
   ("Could not find local public definition %s in %s", symb, mod->filename));
  return ret;
}

xlink_extern *xlink_module_find_extern(xlink_module *mod, const char *name) {
  xlink_extern *ret;
  int i;
  ret = NULL;
  for (i = 1; i <= mod->nexterns; i++) {
    xlink_extern *ext;
    ext = xlink_module_get_extern(mod, i);
    if (!ext->is_local && strcmp(name, ext->name) == 0) {
      XLINK_ERROR(ret != NULL,
       ("Duplicate extern definition found for name %s in %s", name,
       mod->filename));
      ret = ext;
    }
  }
  XLINK_ERROR(ret == NULL,
   ("Could not find extern definition %s in %s", name, mod->filename));
  return ret;
}

XLINK_LIST_FUNCS(library, module);

XLINK_LIST_FUNCS(binary, module);
XLINK_LIST_FUNCS(binary, library);

xlink_public *xlink_binary_find_public(xlink_binary *bin, const char *symb) {
  xlink_public *ret;
  int i, j, k;
  ret = NULL;
  for (i = 1; i <= bin->nmodules; i++) {
    xlink_module *mod;
    mod = xlink_binary_get_module(bin, i);
    for (j = 1; j <= mod->npublics; j++) {
      xlink_public *pub;
      pub = xlink_module_get_public(mod, j);
      if (!pub->is_local && strcmp(symb, pub->name) == 0) {
        XLINK_ERROR(ret != NULL,
         ("Duplicate public definition found for symbol %s in %s and %s",
         symb, ret->module->filename, mod->filename));
        ret = pub;
      }
    }
  }
  for (i = 1; i <= bin->nlibrarys; i++) {
    xlink_library *lib;
    lib = xlink_binary_get_library(bin, i);
    for (j = 1; j <= lib->nmodules; j++) {
      xlink_module *mod;
      mod = xlink_library_get_module(lib, j);
      for (k = 1; k <= mod->npublics; k++) {
        xlink_public *pub;
        pub = xlink_module_get_public(mod, k);
        if (!pub->is_local && strcmp(symb, pub->name) == 0) {
          XLINK_ERROR(ret != NULL,
           ("Duplicate public definition found for symbol %s in %s and %s",
           symb, ret->module->filename, mod->filename));
          ret = pub;
        }
      }
    }
  }
  XLINK_ERROR(ret == NULL, ("Could not find public definition %s", symb));
  return ret;
}

XLINK_LIST_FUNCS(binary, segment);
XLINK_LIST_FUNCS(binary, extern);

void xlink_binary_process_segment(xlink_binary *bin, xlink_segment *segment) {
  int i;
  if (xlink_binary_has_segment(bin, segment)) {
    return;
  }
  xlink_binary_add_segment(bin, segment);
  for (i = 1; i <= segment->nrelocs; i++) {
    xlink_reloc *rel;
    rel = xlink_segment_get_reloc(segment, i);
    switch (rel->target) {
      case OMF_TARGET_EXT : {
        xlink_extern *ext;
        ext = xlink_module_get_extern(segment->module, rel->target_idx);
        if (!xlink_binary_has_extern(bin, ext)) {
          xlink_binary_add_extern(bin, ext);
        }
        break;
      }
      case OMF_TARGET_SEG : {
        xlink_segment *seg;
        seg = xlink_module_get_segment(segment->module, rel->target_idx);
        xlink_binary_process_segment(bin, seg);
        break;
      }
      default : {
        XLINK_ERROR(1, ("Unsupported target T%i", rel->target));
      }
    }
  }
}

void xlink_binary_print_map(xlink_binary *bin, FILE *out) {
  int i, j;
  fprintf(out, "Segment layout:\n");
  for (i = 1; i <= bin->nsegments; i++) {
    xlink_segment *seg;
    seg = xlink_binary_get_segment(bin, i);
    fprintf(out, " %5x %8s segment %s %s %s %6s %08x bytes%s\n",
     seg->start, xlink_segment_get_name(seg),
     OMF_SEGDEF_ALIGN[seg->attrib.align], OMF_SEGDEF_USE[seg->attrib.proc],
     OMF_SEGDEF_COMBINE[seg->attrib.combine], xlink_segment_get_class_name(seg),
     seg->length, seg->attrib.big ? ", big" : "");
    for (j = 1; j <= seg->npublics; j++) {
      xlink_public *pub;
      int size;
      pub = xlink_segment_get_public(seg, j);
      size = seg->length - pub->offset;
      if (j < seg->npublics) {
        size = xlink_segment_get_public(seg, j + 1)->offset - pub->offset;
      }
      fprintf(out, " %12x (%08x bytes) %8s\n", pub->offset, size, pub->name);
    }
  }
}

void xlink_segment_apply_relocations(xlink_segment *segment) {
  int i;
  for (i = 1; i <= segment->nrelocs; i++) {
    xlink_reloc *rel;
    unsigned char *data;
    int offset;
    int target;
    rel = xlink_segment_get_reloc(segment, i);
    data = segment->data + rel->offset;
    offset = segment->start + rel->offset;
    switch (rel->target) {
      case OMF_TARGET_EXT : {
        xlink_public *pub;
        pub = xlink_module_get_extern(segment->module, rel->target_idx)->public;
        target = pub->offset;
        if (pub->segment != NULL) {
          target += pub->segment->start;
        }
        break;
      }
      case OMF_TARGET_SEG : {
        xlink_segment *seg;
        seg = xlink_module_get_segment(segment->module, rel->target_idx);
        target = seg->start;
        break;
      }
      default : {
        XLINK_ERROR(1, ("Unsupported target T%i", rel->target));
      }
    }
    switch (rel->location) {
      case OMF_LOCATION_16BIT : {
        if (rel->mode == OMF_MODE_EXPLICIT) {
          *(uint16_t *)data = target + rel->addend.offset;
        }
        else {
          *(uint16_t *)data = target + rel->addend.offset - offset - 2;
        }
        break;
      }
      case OMF_LOCATION_32BIT : {
        if (rel->mode == OMF_MODE_EXPLICIT) {
          *(uint32_t *)data = target + rel->addend.offset;
        }
        else {
          *(uint32_t *)data = target + rel->addend.offset - offset - 4;
        }
        break;
      }
      default : {
        XLINK_ERROR(1, ("Unsupported location %i", rel->location));
      }
    }
  }
}

unsigned char *xlink_file_init(xlink_file *file, const char *name) {
  FILE *fp;
  int size;
  unsigned char *buf;
  fp = fopen(name, "rb");
  XLINK_ERROR(fp == NULL, ("Could not open OMF file %s", name));
  file->name = name;
  fseek(fp, 0, SEEK_END);
  file->size = ftell(fp);
  buf = xlink_malloc(file->size);
  file->buf = buf;
  fseek(fp, 0, SEEK_SET);
  size = fread(buf, 1, file->size, fp);
  XLINK_ERROR(size != file->size,
   ("Problem reading OMF file, got %i of %i bytes", size, file->size));
  fclose(fp);
  file->pos = 0;
  return buf;
}

static void xlink_omf_record_reset(xlink_omf_record *rec) {
  rec->idx = 3;
}

static int xlink_omf_record_data_left(xlink_omf_record *rec) {
  return 3 + rec->size - 1 - rec->idx;
}

static int xlink_omf_record_has_data(xlink_omf_record *rec) {
  return xlink_omf_record_data_left(rec) > 0;
}

static uint8_t xlink_omf_record_read_byte(xlink_omf_record *rec) {
  return rec->buf[rec->idx++];
}

static uint16_t xlink_omf_record_read_word(xlink_omf_record *rec) {
  uint16_t ret;
  ret = *(uint16_t *)(rec->buf + rec->idx);
  rec->idx += 2;
  return ret;
}

static uint32_t xlink_omf_record_read_dword(xlink_omf_record *rec) {
  uint32_t ret;
  ret = *(uint32_t *)(rec->buf + rec->idx);
  rec->idx += 4;
  return ret;
}

static unsigned int xlink_omf_record_read_index(xlink_omf_record *rec) {
  unsigned char byte;
  byte = xlink_omf_record_read_byte(rec);
  if (byte & 0x80) {
    return ((byte & 0x7f) << 8) | xlink_omf_record_read_byte(rec);
  }
  return byte;
}

unsigned int xlink_omf_record_read_numeric(xlink_omf_record *rec) {
  if (rec->type & 1) {
    return xlink_omf_record_read_dword(rec);
  }
  return xlink_omf_record_read_word(rec);
}

unsigned int xlink_omf_record_read_length(xlink_omf_record *rec) {
  unsigned int length;
  length = xlink_omf_record_read_byte(rec);
  switch (length) {
    case 0x81 : {
      return xlink_omf_record_read_word(rec);
    }
    case 0x84 : {
      length = xlink_omf_record_read_word(rec);
      return (xlink_omf_record_read_byte(rec) << 16) | length;
    }
    case 0x88 : {
      return xlink_omf_record_read_dword(rec);
    }
    default : {
      XLINK_ERROR(length > 0x80, ("Invalid length %i", length));
      return length;
    }
  }
}

const char *xlink_omf_record_read_string(xlink_omf_record *rec) {
  static char str[256];
  int len = xlink_omf_record_read_byte(rec);
  if (len != 0) {
    memcpy(str, &rec->buf[rec->idx], len);
  }
  str[len] = '\0';
  rec->idx += len;
  return str;
}

unsigned int xlink_omf_record_read_lidata_block(xlink_omf_record *rec) {
  unsigned int repeat_count;
  unsigned int block_count;
  unsigned int size;
  repeat_count = xlink_omf_record_read_numeric(rec);
  block_count = xlink_omf_record_read_word(rec);
  printf("%i * ", repeat_count);
  if (block_count) {
    int i;
    printf("(");
    size = 0;
    for (i = 0; i < block_count; i++) {
      size += xlink_omf_record_read_lidata_block(rec);
      if (i > 0) printf(" + ");
    }
    printf(")");
  }
  else {
    size = xlink_omf_record_read_byte(rec);
    rec->idx += size;
    printf("%i", size);
  }
  return repeat_count * size;
}

static void xlink_file_load_omf_record(xlink_file *file,
 xlink_omf_record *rec) {
  unsigned char checksum;
  XLINK_ERROR(file->size < 3,
   ("Need 3 bytes for a record but only have %i", file->size));
  rec->buf = file->buf;
  rec->idx = 0;
  rec->type = xlink_omf_record_read_byte(rec);
  rec->size = xlink_omf_record_read_word(rec);
  XLINK_ERROR(file->size - 3 < rec->size,
   ("Record extends %i bytes past the end of file %s",
   rec->size - file->size + 3, file->name));
  file->buf += 3 + rec->size;
  file->pos += 3 + rec->size;
  file->size -= 3 + rec->size;
  checksum = rec->buf[3 + rec->size - 1];
  if (checksum != 0) {
    int i;
    for (i = 0; i < 3 + rec->size - 1; i++) {
      checksum += rec->buf[i];
    }
  }
  XLINK_ERROR(checksum != 0, ("Invalid checksum, expected 0 got %i", checksum));
}

void xlink_omf_dump_records(xlink_omf *omf) {
  int i;
  xlink_omf_record *rec;
  printf("Summary of records:\n");
  for (i = 0, rec = omf->records; i < omf->nrecords; i++, rec++) {
    printf("  %02Xh %-7s - %-32s%6i bytes%s\n", rec->type,
     xlink_omf_record_get_name(rec->type), xlink_omf_record_get_desc(rec->type),
     3 + rec->size - 1, rec->type & 1 ? " 32-bit" : "");
  }
}

void xlink_omf_dump_lxdata(xlink_omf *omf, xlink_module *mod) {
  int i;
  xlink_omf_record *rec;
  printf("LEDATA, LIDATA, COMDAT and FIXUPP records:\n");
  for (i = 0, rec = omf->records; i < omf->nrecords; i++, rec++) {
    xlink_segment *seg;
    int offset;
    xlink_omf_record_reset(rec);
    switch (rec->type & ~1) {
      case OMF_LEDATA : {
        seg = xlink_module_get_segment(mod, xlink_omf_record_read_index(rec));
        offset = xlink_omf_record_read_numeric(rec);
        printf("  LEDATA: segment %8s, offset 0x%04x, size 0x%04x\n",
         xlink_segment_get_name(seg), offset, 3 + rec->size - 1 - rec->idx);
        break;
      }
      case OMF_LIDATA : {
        seg = xlink_module_get_segment(mod, xlink_omf_record_read_index(rec));
        offset = xlink_omf_record_read_numeric(rec);
        printf("  LIDATA: segment %8s, offset 0x%04x, size ",
         xlink_segment_get_name(seg), offset);
        printf(" = 0x%04x\n", xlink_omf_record_read_lidata_block(rec));
        break;
      }
    }
  }
}

void xlink_module_dump_names(xlink_module *mod) {
  int i;
  if (mod->nnames > 0) {
    printf("Local names:\n");
    for (i = 1; i <= mod->nnames; i++) {
      char buf[256];
      sprintf(buf, "'%s'", xlink_module_get_name(mod, i)->str);
      printf("%2i : %8s\n", i, buf);
    }
  }
}

void xlink_module_dump_symbols(xlink_module *mod) {
  int i;
  if (mod->npublics > 0) {
    printf("Public names:\n");
    for (i = 1; i <= mod->npublics; i++) {
      xlink_public *pub;
      char buf[256];
      pub = xlink_module_get_public(mod, i);
      sprintf(buf, "'%s'", pub->name);
      printf("%2i : %16s %c group %8s, segment %8s, offset 0x%04x, type %i\n",
       i, buf, pub->is_local ? 'L' : ' ',
       pub->group ? xlink_group_get_name(pub->group) : ":0",
       pub->segment ? xlink_segment_get_name(pub->segment) : ":0",
       pub->offset, pub->type_idx);
    }
  }
  if (mod->nexterns > 0) {
    printf("External names:\n");
    for (i = 1; i <= mod->nexterns; i++) {
      xlink_extern *ext;
      char buf[256];
      ext = xlink_module_get_extern(mod, i);
      sprintf(buf, "'%s'", ext->name);
      printf("%2i : %16s %c type %i\n", i, buf, ext->is_local ? 'L' : ' ',
       ext->type_idx);
    }
  }
}

void xlink_module_dump_segments(xlink_module *mod) {
  int i, j;
  printf("Segment records:\n");
  for (i = 1; i <= mod->nsegments; i++) {
    xlink_segment *seg;
    seg = xlink_module_get_segment(mod, i);
    printf("%2i : %6s segment %s %s %s %6s 0x%04x bytes%s\n", i, seg->name->str,
     OMF_SEGDEF_ALIGN[seg->attrib.align], OMF_SEGDEF_USE[seg->attrib.proc],
     OMF_SEGDEF_COMBINE[seg->attrib.combine], xlink_segment_get_class_name(seg),
     seg->length, seg->attrib.big ? ", big" : "");
  }
  for (i = 1; i <= mod->ngroups; i++) {
    xlink_group *grp;
    grp = xlink_module_get_group(mod, i);
    printf("Group: %s\n", xlink_group_get_name(grp));
    for (j = 1; j <= grp->nsegments; j++) {
      printf("%2i : segment %8s\n", j,
       xlink_segment_get_name(xlink_group_get_segment(grp, j)));
    }
  }
}

void xlink_module_dump_relocations(xlink_module *mod) {
  int i;
  for (i = 1; i <= mod->nrelocs; i++) {
    xlink_reloc *rel;
    xlink_segment *seg;
    rel = xlink_module_get_reloc(mod, i);
    seg = rel->segment;
    switch (rel->target) {
      case OMF_TARGET_EXT : {
        xlink_extern *ext;
        ext = xlink_module_get_extern(seg->module, rel->target_idx);
        printf("  FIXUPP: %6s %c seg %s off 0x%03x -> ext %13s addend %s\n",
         OMF_FIXUP_LOCATION[rel->location], rel->mode ? 'E' : 'R',
         xlink_segment_get_name(seg), rel->offset, xlink_extern_get_name(ext),
         xlink_reloc_get_addend(rel));
        break;
      }
      case OMF_TARGET_SEG : {
        xlink_segment *target;
        target = xlink_module_get_segment(seg->module, rel->target_idx);
        printf(
         "  FIXUPP: %6s %c seg %s off 0x%03x -> ",
         OMF_FIXUP_LOCATION[rel->location], rel->mode ? 'E' : 'R',
         xlink_segment_get_name(seg), rel->offset);
        printf("seg %8s off 0x0 addend %s\n",
         xlink_segment_get_name(target), xlink_reloc_get_addend(rel));
        break;
      }
      default : {
        XLINK_ERROR(1, ("Unsupported target T%i", rel->target));
      }
    }
  }
}

void xlink_module_split_segments(xlink_module *mod) {
  int i, j, k, n, m;
  for (i = 0; i < mod->nsegments; i++) {
    xlink_segment *seg;
    seg = mod->segments[i];
    if (seg->npublics > 1) {
      int *offsets;
      xlink_segment **segs;
      /* Allocate space for the worst case, a segment for every public */
      segs = xlink_malloc(seg->npublics*sizeof(xlink_segment *));
      offsets = xlink_malloc(seg->npublics*sizeof(int));
      /* All publics with offset == 0 stay in the existing segment */
      for (n = 0; n < seg->npublics && seg->publics[n]->offset == 0; n++);
      /* All publics with offset > 0 are split into their own segment */
      for (m = 0, j = seg->npublics - 1; j > n - 1; m++) {
        xlink_public *pub;
        pub = seg->publics[j];
        segs[m] = xlink_malloc(sizeof(xlink_segment));
        offsets[m] = pub->offset;
        *segs[m] = *seg;
        segs[m]->publics = NULL;
        segs[m]->relocs = NULL;
        segs[m]->npublics = segs[m]->nrelocs = 0;
        segs[m]->length = seg->length - offsets[m];
        if (seg->info & SEG_HAS_DATA) {
          segs[m]->data = xlink_malloc(segs[m]->length);
          segs[m]->mask = NULL;
          memcpy(segs[m]->data, &seg->data[offsets[m]], segs[m]->length);
        }
        seg->length -= segs[m]->length;
        /* Add all publics with the same offset to this new segment */
        for (; pub->offset == offsets[m]; j--, pub = seg->publics[j]) {
          pub->offset = 0;
          pub->segment = segs[m];
          xlink_segment_add_public(segs[m], pub);
        }
      }
      seg->npublics = n;
      /* Add the new segments to the module in the original order */
      for (j = m; j-- > 0; ) {
        XLINK_LIST_ADD(module, segment, mod, segs[j]);
      }
      /* All relocs with offset < seg->length stay in the existing segment */
      n = 0;
      for (; n < seg->nrelocs && seg->relocs[n]->offset < seg->length; n++) {
        xlink_reloc *rel;
        rel = seg->relocs[n];
        XLINK_ERROR(seg->length < rel->offset + OMF_FIXUP_SIZE[rel->location],
         ("FIXUP offset %i past segment end %i", rel->offset, seg->length));
      }
      /* Move all relocs with offset >= seg->length to the right segment */
      for (k = m - 1, j = n; j < seg->nrelocs; j++) {
        xlink_reloc *rel;
        rel = seg->relocs[j];
        for (; k >= 0 && offsets[k] + segs[k]->length < rel->offset; k--);
        XLINK_ERROR(k < 0,
         ("Could not find segment for reloc offset %i", rel->offset));
        rel->offset -= offsets[k];
        XLINK_ERROR(
         segs[k]->length < rel->offset + OMF_FIXUP_SIZE[rel->location],
         ("FIXUP offset %i past segment end %i", rel->offset, seg->length));
        rel->segment = segs[k];
        xlink_segment_add_reloc(segs[k], rel);
      }
      seg->nrelocs = n;
      /* If seg was part of a group, insert new segments into that group */
      if (seg->group != NULL) {
        xlink_group *grp;
        grp = seg->group;
        for (n = 0; n < grp->nsegments && grp->segments[n] != seg; n++);
        n++;
        grp->nsegments += m;
        grp->segments =
         xlink_realloc(grp->segments, grp->nsegments*sizeof(xlink_segment *));
        memmove(&grp->segments[n + m], &grp->segments[n],
         (grp->nsegments - (n + m))*sizeof(xlink_segment *));
        for (j = m; j-- > 0; ) {
          grp->segments[n + m - (j + 1)] = segs[j];
        }
      }
      /* If any relocs pointed into the segment, set their index and offset */
      for (j = 0; j < mod->nrelocs; j++) {
        xlink_reloc *rel;
        rel = mod->relocs[j];
        if (rel->target == OMF_TARGET_SEG && rel->target_idx == seg->index ) {
          k = 0;
          while (k < m && offsets[k] + segs[k]->length < rel->addend.offset) {
            k++;
          }
          XLINK_ERROR(k == m,
           ("Could not find segment for reloc offset %i", rel->offset));
          rel->target_idx = segs[k]->index;
          rel->addend.offset -= offsets[k];
        }
      }
      free(segs);
      free(offsets);
    }
  }
}

int pub_comp(const void *a, const void *b) {
  const xlink_public *pub_a;
  const xlink_public *pub_b;
  pub_a = *(xlink_public **)a;
  pub_b = *(xlink_public **)b;
  return pub_a->offset - pub_b->offset;
}

int rel_comp(const void *a, const void *b) {
  const xlink_reloc *rel_a;
  const xlink_reloc *rel_b;
  rel_a = *(xlink_reloc **)a;
  rel_b = *(xlink_reloc **)b;
  return rel_a->offset - rel_b->offset;
}

int xlink_omf_record_load_iterated(xlink_omf_record *rec, unsigned char *data,
 int length) {
  unsigned char *block;
  unsigned int repeat_count;
  unsigned int block_count;
  unsigned int size;
  int i;
  block = data;
  repeat_count = xlink_omf_record_read_numeric(rec);
  block_count = xlink_omf_record_read_word(rec);
  if (block_count) {
    size = 0;
    for (i = 0; i < block_count; i++) {
      int block_size;
      block_size = xlink_omf_record_load_iterated(rec, block, length);
      block += block_size;
      length -= block_size;
      size += block_size;
    }
  }
  else {
    size = xlink_omf_record_read_byte(rec);
    XLINK_ERROR(size > length,
     ("LIDATA wrote past end of segment, size = %i but length = %i", size,
     length));
    memcpy(block, &rec->buf[rec->idx], size);
    block += size;
    rec->idx += size;
  }
  for (i = 1; i < repeat_count; i++) {
    memcpy(block, data, size);
    block += size;
  }
  return repeat_count*size;
}

#define MOD_DUMP  (0x1)
#define MOD_MAP   (0x2)
#define MOD_SPLIT (0x4)
#define MOD_CHECK (0x8)
#define MOD_PACK  (0x10)

xlink_module *xlink_file_load_module(xlink_file *file, unsigned int flags) {
  xlink_module *mod;
  xlink_omf omf;
  xlink_data *dat;
  int done;
  int i, j;
  mod = xlink_malloc(sizeof(xlink_module));
  mod->filename = file->name;
  xlink_omf_init(&omf);
  dat = NULL;
  done = 0;
  while (file->size > 0 && !done) {
    xlink_omf_record rec;
    int i;
    xlink_file_load_omf_record(file, &rec);
    xlink_omf_add_record(&omf, &rec);
    switch (rec.type & ~1) {
      case OMF_THEADR :
      case OMF_LHEADR : {
        strcpy(mod->source, xlink_omf_record_read_string(&rec));
        break;
      }
      case OMF_MODEND : {
        done = 1;
        break;
      }
      case OMF_LNAMES :
      case OMF_LLNAMES : {
        while (xlink_omf_record_has_data(&rec)) {
          xlink_name *name;
          name = xlink_malloc(sizeof(xlink_name));
          strcpy(name->str, xlink_omf_record_read_string(&rec));
          XLINK_LIST_ADD(module, name, mod, name);
        }
        break;
      }
      case OMF_SEGDEF : {
        xlink_segment *seg;
        seg = xlink_malloc(sizeof(xlink_segment));
        seg->attrib.b = xlink_omf_record_read_byte(&rec);
        if (seg->attrib.align == 0) {
          seg->frame = xlink_omf_record_read_word(&rec);
          seg->offset = xlink_omf_record_read_byte(&rec);
        }
        else {
          seg->frame = seg->offset = 0;
        }
        seg->length = xlink_omf_record_read_numeric(&rec);
        XLINK_ERROR(seg->attrib.big && seg->length != 0,
         ("Invalid length for 'big' segment, expected 0 got %i", seg->length));
        seg->name =
         xlink_module_get_name(mod, xlink_omf_record_read_index(&rec));
        seg->class =
         xlink_module_get_name(mod, xlink_omf_record_read_index(&rec));
        /* Read and ignore Overlay Name Index */
        xlink_omf_record_read_index(&rec);
        seg->info = 0;
        seg->data = xlink_malloc(seg->length);
        seg->mask = xlink_malloc(CEIL2(seg->length, 3));
        xlink_segment_reset_mask(seg);
        XLINK_LIST_ADD(module, segment, mod, seg);
        break;
      }
      case OMF_GRPDEF : {
        xlink_group *grp;
        grp = xlink_malloc(sizeof(xlink_group));
        grp->name =
         xlink_module_get_name(mod, xlink_omf_record_read_index(&rec));
        grp->nsegments = 0;
        while (xlink_omf_record_has_data(&rec)) {
          unsigned char index;
          xlink_segment *s;
          index = xlink_omf_record_read_byte(&rec);
          XLINK_ERROR(index != 0xFF, ("Invalid segment index %i", index));
          s = xlink_module_get_segment(mod, xlink_omf_record_read_index(&rec));
          XLINK_ERROR(s->group != NULL,
           ("Cannot add segment %s to group %s, already added to group %s:%i",
           xlink_segment_get_name(s), xlink_group_get_name(grp), s->group->name,
           s->group->index));
          s->group = grp;
          xlink_group_add_segment(grp, s);
        }
        XLINK_LIST_ADD(module, group, mod, grp);
        break;
      }
      case OMF_PUBDEF :
      case OMF_LPUBDEF : {
        int group_idx;
        int segment_idx;
        xlink_public base;
        group_idx = xlink_omf_record_read_index(&rec);
        segment_idx = xlink_omf_record_read_index(&rec);
        base.group = group_idx ? xlink_module_get_group(mod, group_idx) : NULL;
        base.segment =
         segment_idx ? xlink_module_get_segment(mod, segment_idx) : NULL;
        base.base_frame =
         base.segment == NULL ? xlink_omf_record_read_word(&rec) : 0;
        base.is_local = ((rec.type & ~1) == OMF_LPUBDEF);
        while (xlink_omf_record_has_data(&rec)) {
          xlink_public *pub;
          pub = xlink_malloc(sizeof(xlink_public));
          *pub = base;
          strcpy(pub->name, xlink_omf_record_read_string(&rec));
          pub->offset = xlink_omf_record_read_numeric(&rec);
          pub->type_idx = xlink_omf_record_read_index(&rec);
          XLINK_LIST_ADD(module, public, mod, pub);
          if (pub->segment) {
            xlink_segment_add_public(pub->segment, pub);
          }
        }
        break;
      }
      case OMF_EXTDEF :
      case OMF_LEXTDEF : {
        xlink_extern base;
        base.is_local = (rec.type == OMF_LEXTDEF);
        while (xlink_omf_record_has_data(&rec)) {
          xlink_extern *ext;
          ext = xlink_malloc(sizeof(xlink_extern));
          *ext = base;
          strcpy(ext->name, xlink_omf_record_read_string(&rec));
          ext->type_idx = xlink_omf_record_read_index(&rec);
          XLINK_LIST_ADD(module, extern, mod, ext);
        }
        break;
      }
      case OMF_LEDATA :
      case OMF_LIDATA : {
        xlink_segment *seg;
        seg = xlink_module_get_segment(mod, xlink_omf_record_read_index(&rec));
        seg->info |= SEG_HAS_DATA;
        dat = xlink_malloc(sizeof(xlink_data));
        dat->segment = seg;
        dat->offset = xlink_omf_record_read_numeric(&rec);
        dat->is_iterated = rec.type & 0x2;
        dat->length = xlink_omf_record_data_left(&rec);
        dat->data = xlink_malloc(dat->length);
        dat->mask = xlink_malloc(CEIL2(dat->length, 3));
        memcpy(dat->data, &rec.buf[rec.idx], dat->length);
        xlink_data_reset_mask(dat);
        if (!dat->is_iterated) {
          XLINK_ERROR(dat->offset + dat->length > seg->length,
           ("LEDATA wrote past end of segment %s, offset = %i but length = %i",
           xlink_segment_get_name(seg), dat->offset + dat->length,
           seg->length));
          memcpy(&seg->data[dat->offset], &rec.buf[rec.idx], dat->length);
          for (i = 0; i < dat->length; i++) {
            XLINK_ERROR(XLINK_GET_BIT(seg->mask, dat->offset + i),
             ("LEDATA overwrote existing data in segment %s, offset = %i",
             xlink_segment_get_name(seg), dat->offset + i));
            XLINK_SET_BIT(seg->mask, dat->offset + i, 1);
          }
        }
        else {
          int size;
          size = xlink_omf_record_load_iterated(&rec, &seg->data[dat->offset],
           seg->length - dat->offset);
          for (i = 0; i < size; i++) {
            XLINK_ERROR(XLINK_GET_BIT(seg->mask, dat->offset + i),
             ("LIDATA overwrote existing data in segment %s, offset = %i",
             xlink_segment_get_name(seg), dat->offset + i));
            XLINK_SET_BIT(seg->mask, dat->offset + i, 1);
          }
        }
        XLINK_LIST_ADD(module, data, mod, dat);
        xlink_segment_add_data(dat->segment, dat);
        break;
      }
      case OMF_FIXUPP : {
        while (xlink_omf_record_has_data(&rec)) {
          unsigned char byte;
          byte = xlink_omf_record_read_byte(&rec);
          if (byte & 0x80) {
            xlink_segment *seg;
            xlink_reloc *rel;
            xlink_omf_fixup_locat locat;
            xlink_omf_fixup_fixdata fixdata;
            const unsigned char *data;
            XLINK_ERROR(dat == NULL, ("Got FIXUP before LxDATA record"));
            XLINK_ERROR(dat->is_iterated,
             ("FIXUP records for LIDATA not yet supported"));
            seg = dat->segment;
            rel = xlink_malloc(sizeof(xlink_reloc));
            rel->segment = seg;
            locat.b0 = byte;
            locat.b1 = xlink_omf_record_read_byte(&rec);
            XLINK_ERROR(locat.high != 1, ("Expecting FIXUP subrecord"));
            rel->offset = dat->offset + locat.offset;
            rel->mode = locat.mode;
            rel->location = locat.location;
            XLINK_ERROR(
             seg->length < rel->offset + OMF_FIXUP_SIZE[rel->location],
             ("FIXUP offset %i past segment end %i", rel->offset, seg->length));
            for (i = 0; i < OMF_FIXUP_SIZE[rel->location]; i++) {
              XLINK_ERROR(XLINK_GET_BIT(seg->mask, rel->offset + i) == 0,
               ("FIXUP modifies uninitialized data, location %i",
               rel->offset + i));
            }
            fixdata.b = xlink_omf_record_read_byte(&rec);
            if (!fixdata.th_frame) {
              rel->frame = fixdata.frame;
              rel->frame_idx = 0;
              if (fixdata.frame < OMF_FRAME_LOC) {
                rel->frame_idx = xlink_omf_record_read_index(&rec);
                switch (rel->frame) {
                  case OMF_FRAME_SEG : {
                    xlink_module_get_segment(mod, rel->frame_idx);
                    break;
                  }
                  case OMF_FRAME_GRP : {
                    xlink_module_get_group(mod, rel->frame_idx);
                    break;
                  }
                  case OMF_FRAME_EXT : {
                    xlink_module_get_extern(mod, rel->frame_idx);
                    break;
                  }
                }
              }
            }
            else {
              // TODO: Get frame info from thread in xlink_omf struct
            }
            if (!fixdata.th_target) {
              rel->target = (fixdata.no_disp << 2) + fixdata.target;
              rel->target_idx = xlink_omf_record_read_index(&rec);
              switch (rel->target) {
                case OMF_TARGET_SEG :
                case OMF_TARGET_SEG_DISP : {
                  xlink_module_get_segment(mod, rel->target_idx);
                  break;
                }
                case OMF_TARGET_GRP :
                case OMF_TARGET_GRP_DISP : {
                  xlink_module_get_group(mod, rel->target_idx);
                  break;
                }
                case OMF_TARGET_EXT :
                case OMF_TARGET_EXT_DISP : {
                  xlink_module_get_extern(mod, rel->target_idx);
                  break;
                }
              }
            }
            else {
              // TODO: Get target info from thread in xlink_omf struct
            }
            if (!fixdata.no_disp) {
              rel->disp = xlink_omf_record_read_numeric(&rec);
            }
            data = seg->data + rel->offset;
            switch (rel->location) {
              case OMF_LOCATION_8BIT :
              case OMF_LOCATION_8BIT_HIGH : {
                rel->addend.offset = *(int8_t *)data;
                break;
              }
              case OMF_LOCATION_16BIT :
              case OMF_LOCATION_SEGMENT :
              case OMF_LOCATION_16BIT_LOADER : {
                rel->addend.offset = *(int16_t *)data;
                break;
              }
              case OMF_LOCATION_FAR : {
                rel->addend.segment = *(uint16_t *)(data + 2),
                rel->addend.offset = *(int16_t *)data;
                break;
              }
              case OMF_LOCATION_32BIT :
              case OMF_LOCATION_32BIT_LOADER : {
                rel->addend.offset = *(int32_t *)data;
                break;
              }
              case OMF_LOCATION_48BIT :
              case OMF_LOCATION_48BIT_PHARLAP : {
                rel->addend.segment = *(uint16_t *)(data + 4),
                rel->addend.offset = *(int32_t *)data;
                break;
              }
              default : {
                XLINK_ERROR(1, ("Unsupported location %i", rel->location));
              }
            }
            XLINK_LIST_ADD(module, reloc, mod, rel);
            xlink_segment_add_reloc(rel->segment, rel);
          }
          else {
            xlink_omf_fixup_thread th;
            int index;
            XLINK_ERROR(th.high != 0, ("Expecting THREAD subrecord"));
            index = 0;
            if (th.method < OMF_FRAME_LOC) {
              index = xlink_omf_record_read_index(&rec);
            }
          }
        }
      }
    }
  }
  XLINK_ERROR(!done, ("Got EOF before reading MODEND in %s", file->name));
  for (i = 1; i <= mod->nsegments; i++) {
    xlink_segment *seg;
    seg = xlink_module_get_segment(mod, i);
    /* Check that all segments are either fully populated or uninitialized */
    if (seg->info & SEG_HAS_DATA) {
      for (j = 0; j < seg->length; j++) {
        XLINK_ERROR(XLINK_GET_BIT(seg->mask, j) == 0,
         ("Missing data for segment %s, offset = %i",
         xlink_segment_get_name(seg), j));
      }
    }
    else {
      free(seg->data);
      free(seg->mask);
      seg->data = seg->mask = NULL;
    }
    /* Sort the PUBDEF and FIXUPP records by their offset into segment */
    qsort(seg->publics, seg->npublics, sizeof(xlink_public *), pub_comp);
    qsort(seg->relocs, seg->nrelocs, sizeof(xlink_reloc *), rel_comp);
    if ((seg->info & SEG_HAS_DATA) == 0) {
      XLINK_ERROR(seg->nrelocs != 0,
       ("Uninitialized segment %s has %i relocations, should have none",
       xlink_segment_get_name(seg), seg->nrelocs));
    }
  }
  if (flags & MOD_SPLIT) {
    xlink_module_split_segments(mod);
  }
  if (flags & MOD_DUMP) {
    printf("Module: %s\n", mod->filename);
    xlink_omf_dump_records(&omf);
    xlink_module_dump_names(mod);
    xlink_module_dump_symbols(mod);
    xlink_module_dump_segments(mod);
    xlink_omf_dump_lxdata(&omf, mod);
    xlink_module_dump_relocations(mod);
  }
  xlink_omf_clear(&omf);
  return mod;
}

xlink_library *xlink_file_load_library(xlink_file *file, unsigned int flags) {
  xlink_library *lib;
  int done;
  lib = xlink_malloc(sizeof(xlink_library));
  done = 0;
  while (file->size > 0 && !done) {
    switch (file->buf[0]) {
      case OMF_LIBHDR : {
        xlink_omf_record rec;
        xlink_file_load_omf_record(file, &rec);
        lib->page_size = rec.size + 3;
        break;
      }
      case OMF_LIBEND : {
        done = 1;
        break;
      }
      case OMF_THEADR :
      case OMF_LHEADR : {
        int mask;
        int skip;
        xlink_module *mod;
        /* TODO: Find embedded module name in dictionary, set file->name here */
        mod = xlink_file_load_module(file, flags);
        XLINK_LIST_ADD(library, module, lib, mod);
        /* Page size is always a power of two */
        mask = lib->page_size - 1;
        /* Compute the number of bytes to skip to reach the next page */
        skip = ((file->pos + mask) & ~mask) - file->pos;
        file->buf += skip;
        file->pos += skip;
        file->size -= skip;
        break;
      }
      default : {
        XLINK_ERROR(1, ("Unknown record type %02X", file->buf[0]));
      }
    }
  }
  XLINK_ERROR(!done, ("Got EOF before reading LIBEND in %s", file->name));
  return lib;
}

xlink_segment_class xlink_segment_get_class(const xlink_segment *seg) {
  if (strcmp(seg->class->str, "CODE") == 0) {
    return OMF_SEGMENT_CODE;
  }
  if (seg->info & SEG_HAS_DATA) {
    return OMF_SEGMENT_DATA;
  }
  return OMF_SEGMENT_BSS;
}

int seg_comp(const void *a, const void *b) {
  const xlink_segment *seg_a;
  const xlink_segment *seg_b;
  seg_a = *(xlink_segment **)a;
  seg_b = *(xlink_segment **)b;
  if (xlink_segment_get_class(seg_a) == xlink_segment_get_class(seg_b)) {
    if (seg_a->attrib.proc == seg_b->attrib.proc) {
      if (seg_a->module == seg_b->module) {
        return seg_a->index - seg_b->index;
      }
      return seg_a->module->index - seg_b->module->index;
    }
    return seg_a->attrib.proc - seg_b->attrib.proc;
  }
  return xlink_segment_get_class(seg_a) - xlink_segment_get_class(seg_b);
}

/* Link the root segment recursively by walking its externs, resolving each
    to a public symbol, and linking in that public symbol's segment */
void xlink_binary_link_root_segment(xlink_binary *bin, xlink_segment *root) {
  int i;
  xlink_binary_process_segment(bin, root);
  for (i = 1; i <= bin->nexterns; i++) {
    xlink_extern *ext;
    ext = xlink_binary_get_extern(bin, i);
    if (ext->is_local) {
      ext->public = xlink_module_find_public(ext->module, ext->name);
    }
    else {
      ext->public = xlink_binary_find_public(bin, ext->name);
    }
    if (ext->public->segment != NULL) {
      xlink_binary_process_segment(bin, ext->public->segment);
    }
  }
}

/* Sort segments by class (CODE, DATA, BSS) */
void xlink_sort_segments(xlink_segment **segments, int nsegments, int *data_idx,
 int *bss_idx) {
  int i;
  qsort(segments, nsegments, sizeof(xlink_segment *), seg_comp);
  if (data_idx != NULL) {
    for (i = 0; i < nsegments; i++) {
      if (xlink_segment_get_class(segments[i]) == OMF_SEGMENT_DATA) break;
    }
    *data_idx = i;
  }
  if (bss_idx != NULL) {
    for (i = 0; i < nsegments; i++) {
      if (xlink_segment_get_class(segments[i]) == OMF_SEGMENT_BSS) break;
    }
    *bss_idx = i;
  }
}

void xlink_set_first_segment(xlink_segment **segments, xlink_segment *first) {
  int i;
  if (segments[0] != first) {
    for (i = 1; segments[i] != first; i++);
    segments[i] = segments[0];
    segments[0] = first;
  }
}

/* Layout segments in memory with proper alignment starting at offset */
void xlink_binary_layout_segments(xlink_binary *bin, int index, int offset) {
  int i;
  for (i = index + 1; i <= bin->nsegments; i++) {
    xlink_segment *seg;
    seg = xlink_binary_get_segment(bin, i);
    /* If this is a 32-bit program, move all 32-bit BSS above the first 64k */
    if (bin->is_32bit && seg->attrib.proc == OMF_SEGMENT_USE32 &&
     xlink_segment_get_class(seg) == OMF_SEGMENT_BSS && offset < 0x10000) {
      offset = 0x10000;
    }
    offset = ALIGN2(offset, xlink_segment_get_alignment(seg));
    seg->start = offset;
    offset += seg->length;
  }
  XLINK_ERROR(!bin->is_32bit && offset > 65536,
   ("Address space exceeds 65536 bytes, %i", offset));
}

void xlink_apply_relocations(xlink_segment **segments, int nsegments) {
  int i;
  for (i = 0; i < nsegments; i++) {
    xlink_segment_apply_relocations(segments[i]);
  }
}

int xlink_binary_extract_class(xlink_binary *bin, int index, int offset,
 xlink_segment_class class, xlink_list *bytes) {
  int i;
  for (i = index + 1; i <= bin->nsegments; i++) {
    xlink_segment *seg;
    seg = xlink_binary_get_segment(bin, i);
    if (xlink_segment_get_class(seg) != class) break;
    XLINK_ERROR(seg->info & SEG_HAS_DATA == 0,
     ("Error segment %s has no data", xlink_segment_get_name(seg)));
    if (offset != seg->start) {
      unsigned char buf[4096];
      memset(buf, 0, 4096);
      xlink_list_add_all(bytes, buf, seg->start - offset);
      offset = seg->start;
    }
    xlink_list_add_all(bytes, seg->data, seg->length);
    offset += seg->length;
  }
  return offset;
}

void xlink_model_init(xlink_model *model, unsigned char mask) {
  int i;
  model->mask = mask;
  /* For now just compute the weight as the number of bits set in mask */
  model->weight = 0;
  for (i = 0; i < 8; i++) {
    model->weight += !!(mask & (1 << i));
  }
}

int mod_comp(const void *a, const void *b) {
  const xlink_model *mod_a;
  const xlink_model *mod_b;
  int mask_a;
  int mask_b;
  int i;
  mod_a = (xlink_model *)a;
  mod_b = (xlink_model *)b;
  /* Compute the bit-reversal of the mask */
  mask_a = mask_b = 0;
  for (i = 0; i < 8; i++) {
    mask_a <<= 1;
    mask_b <<= 1;
    mask_a |= !!(mod_a->mask & (1 << i));
    mask_b |= !!(mod_b->mask & (1 << i));
  }
  /* Sort on weights ascending, then bit-reversed mask */
  if (mod_a->weight == mod_b->weight) {
    return mask_a - mask_b;
  }
  return mod_a->weight - mod_b->weight;
}

int match_comp(const void *a, const void *b) {
  const xlink_match *mat_a;
  const xlink_match *mat_b;
  mat_a = (xlink_match *)a;
  mat_b = (xlink_match *)b;
  int i;
  if (mat_a->mask == mat_b->mask) {
    if (mat_a->partial == mat_b->partial) {
      for (i = 0; i < 8; i++) {
        if (mat_a->mask & (1 << (7 - i))) {
          if (mat_a->buf[i] == mat_b->buf[i]) {
            continue;
          }
          return mat_a->buf[i] - mat_b->buf[i];
        }
      }
      return 0;
    }
    return mat_a->partial - mat_b->partial;
  }
  return mat_a->mask - mat_b->mask;
}

unsigned int match_hash_code(const void *m) {
  const xlink_match *mat;
  unsigned int hash;
  int i;
  mat = (xlink_match *)m;
  hash = 0x0;
  /* Combine the mask */
  hash += mat->mask;
  hash *= 0x6f;
  hash ^= mat->mask;
  /* Combine the partial */
  hash += mat->partial;
  hash *= 0x6f;
  hash ^= mat->partial;
  /* Combine the history */
  for (i = 0; i < 8; i++) {
    if (mat->mask & (1 << (7 - i))) {
      hash += mat->buf[i];
      hash *= 0x6f;
      hash ^= mat->buf[i];
    }
  }
  /* Extra hashing for good measure */
  hash ^= (hash >> 20) ^ (hash >> 12);
  return hash ^ (hash >> 7) ^ (hash >> 4);
}

unsigned int match_simple_hash_code(const void *m) {
  const xlink_match *mat;
  unsigned int hash;
  unsigned char byte;
  int i;
  mat = (xlink_match *)m;
  /* Use current weight state to salt the hash */
  hash = mat->salt;
  /* Combine the mask */
  hash = (hash & 0xffffff00) | mat->mask;
  /* Combine the partial */
  byte = (hash & 0x000000ff);
  hash = (hash & 0xffffff00) | (byte ^ mat->partial);
  hash = ((int)hash)*0x6f;
  byte = (hash & 0x000000ff);
  hash = (hash & 0xffffff00) | ((byte + mat->partial) & 0x000000ff);
  hash--;
  /* Combine the history */
  for (i = 0; i < 8; i++) {
    if (mat->mask & (1 << (7 - i))) {
      byte = (hash & 0x000000ff);
      hash = (hash & 0xffffff00) | (byte ^ mat->buf[i]);
      hash = ((int)hash)*0x6f;
      byte = (hash & 0x000000ff);
      hash = (hash & 0xffffff00) | ((byte + mat->buf[i]) & 0x000000ff);
      hash--;
    }
  }
  return hash;
}

int match_equals(const void *a, const void *b) {
  return match_comp(a, b) == 0;
}

void xlink_context_reset(xlink_context *ctx) {
  memset(ctx->buf, 0, sizeof(ctx->buf));
  xlink_set_reset(&ctx->matches);
}

void xlink_context_init(xlink_context *ctx, xlink_list *models) {
  ctx->models = models;
  xlink_set_init(&ctx->matches, match_simple_hash_code, match_equals,
   sizeof(xlink_match), 0, 0.75);
  xlink_context_reset(ctx);
}

void xlink_context_clear(xlink_context *ctx) {
  xlink_set_clear(&ctx->matches);
}

void xlink_context_set_capacity(xlink_context *ctx, int capacity) {
  xlink_set_reset(&ctx->matches);
  xlink_set_resize(&ctx->matches, capacity);
}

void xlink_context_get_counts(xlink_context *ctx, unsigned char partial,
 unsigned int counts[2]) {
  xlink_match key;
  int i;
  memcpy(key.buf, ctx->buf, sizeof(ctx->buf));
  key.partial = partial;
  counts[0] = counts[1] = 2;
  for (i = 0; i < xlink_list_length(ctx->models); i++) {
    xlink_model *model;
    xlink_match *match;
    model = xlink_list_get(ctx->models, i);
    key.mask = model->mask;
    key.salt = model->state;
    match = xlink_set_get(&ctx->matches, &key);
    if (match != NULL) {
      counts[0] += ((unsigned int)match->counts[0]) << model->weight;
      counts[1] += ((unsigned int)match->counts[1]) << model->weight;
    }
  }
}

#define xlink_list_get_model(list, i) ((xlink_model *)xlink_list_get(list, i))

void xlink_context_update_bit(xlink_context *ctx, unsigned char partial,
 int bit) {
  xlink_match key;
  int i;
  key.partial = partial;
  memcpy(key.buf, ctx->buf, sizeof(ctx->buf));
  for (i = 0; i < xlink_list_length(ctx->models); i++) {
    xlink_model *model;
    xlink_match *match;
    model = xlink_list_get(ctx->models, i);
    key.mask = model->mask;
    key.salt = model->state;
    match = xlink_set_get(&ctx->matches, &key);
    if (match == NULL) {
      memset(key.counts, 0, sizeof(key.counts));
      match = xlink_set_put(&ctx->matches, &key);
    }
    match->counts[bit] = XLINK_MIN(255, match->counts[bit] + 1);
    if (match->counts[1 - bit] > 1) {
      match->counts[1 - bit] >>= 1;
    }
  }
}

void xlink_bitstream_init(xlink_bitstream *bs) {
  memset(bs, 0, sizeof(xlink_bitstream));
  xlink_list_init(&bs->bytes, sizeof(unsigned char), 0);
}

void xlink_bitstream_clear(xlink_bitstream *bs) {
  xlink_list_clear(&bs->bytes);
}

void xlink_bitstream_write_byte(xlink_bitstream *bs, unsigned char byte) {
  xlink_list_add(&bs->bytes, &byte);
}

#define xlink_list_add_byte(list, byte) \
  do { \
    unsigned char b_; \
    b_ = byte; \
    xlink_list_add(list, &b_); \
  } \
  while (0)

#define xlink_list_get_byte(list, i) ((unsigned char *)xlink_list_get(list, i))

int xlink_bitstream_copy_bits(xlink_bitstream *bs, unsigned char *dst,
 int pos) {
  int i;
  /* Intentionally skip the first bit */
  for (i = 1; i < bs->bits; i++, pos++) {
    XLINK_SET_BIT(dst, pos, XLINK_GET_BIT(bs->bytes.data, i));
  }
  return pos;
}

void xlink_modeler_init(xlink_modeler *mod, int bytes) {
  xlink_list_init(&mod->bytes, sizeof(unsigned char), bytes);
  xlink_list_init(&mod->counts, sizeof(xlink_counts), 8*bytes);
  xlink_set_init(&mod->matches, match_hash_code, match_equals,
   sizeof(xlink_match), 256*8*bytes, 0.75);
}

void xlink_modeler_clear(xlink_modeler *mod) {
  xlink_list_clear(&mod->bytes);
  xlink_list_clear(&mod->counts);
  xlink_set_clear(&mod->matches);
}

void xlink_modeler_load_binary(xlink_modeler *mod, xlink_list *bytes) {
  unsigned char buf[8];
  int i, j, k;
  printf("Scanning %i bytes for counts... ", xlink_list_length(bytes));
  fflush(stdout);
  memset(buf, 0, sizeof(buf));
  for (k = 0; k < xlink_list_length(bytes); k++) {
    unsigned char byte;
    unsigned char partial;
    xlink_match key;
    byte = *xlink_list_get_byte(bytes, k);
    partial = 1;
    memcpy(key.buf, buf, sizeof(buf));
    for (i = 8; i-- > 0; ) {
      int bit;
      xlink_counts counts;
      key.partial = partial;
      bit = !!(byte & (1 << i));
      for (j = 0; j < 256; j++) {
        xlink_match *match;
        key.mask = j;
        match = xlink_set_get(&mod->matches, &key);
        if (match == NULL) {
          memset(key.counts, 0, sizeof(key.counts));
          match = xlink_set_put(&mod->matches, &key);
        }
        XLINK_ERROR(match == NULL,
         ("Null pointer for match, should point to allocated xlink_match"));
        counts[j][0] = match->counts[0];
        counts[j][1] = match->counts[1];
        match->counts[bit] = XLINK_MIN(255, match->counts[bit] + 1);
        if (match->counts[1 - bit] > 1) {
          match->counts[1 - bit] >>= 1;
        }
      }
      xlink_list_add(&mod->counts, &counts);
      partial <<= 1;
      partial |= bit;
    }
    for (i = 8; i-- > 1; ) {
      buf[i] = buf[i - 1];
    }
    buf[0] = byte;
    xlink_list_add(&mod->bytes, &byte);
  }
  printf("done\n");
}

double xlink_modeler_get_entropy(xlink_modeler *mod, xlink_list *models) {
  XLINK_ERROR(
   xlink_list_length(&mod->counts) != 8*xlink_list_length(&mod->bytes),
   ("Counts model does not match input bytes, counts = %i and bytes = %i",
   xlink_list_length(&mod->counts), xlink_list_length(&mod->bytes)));
  if (xlink_list_length(models) == 0) {
    return DBL_MAX;
  }
  else {
    double entropy;
    int i, j, k;
    /* Include 4 bytes for the weights + 1 byte for each model */
    entropy = 8*(4 + xlink_list_length(models));
    for (j = 0; j < xlink_list_length(&mod->bytes); j++) {
      unsigned char byte;
      byte = *xlink_list_get_byte(&mod->bytes, j);
      for (i = 8; i-- > 0; ) {
        int bit;
        xlink_counts *counts;
        unsigned int c0, c1;
        bit = !!(byte & (1 << i));
        counts = xlink_list_get(&mod->counts, j*8 + (7 - i));
        c0 = c1 = 2;
        for (k = 0; k < xlink_list_length(models); k++) {
          xlink_model *model;
          model = xlink_list_get(models, k);
          c0 += ((unsigned int)(*counts)[model->mask][0]) << model->weight;
          c1 += ((unsigned int)(*counts)[model->mask][1]) << model->weight;
        }
        if (bit) {
          entropy -= M_LOG2E*log(((double)c1)/(c0 + c1));
        }
        else {
          entropy -= M_LOG2E*log(((double)c0)/(c0 + c1));
        }
      }
    }
    return entropy;
  }
}

unsigned int xlink_compute_packed_weights(xlink_list *models) {
  unsigned int packed;
  int i, j;
  int last;
  packed = 0xffffffffu;
  i = 0;
  last = 0;
  for (j = 0; j < xlink_list_length(models); j++, i++) {
    xlink_model *model;
    model = xlink_list_get(models, j);
    i += model->weight - last;
    XLINK_ERROR(i > 31, ("Invalid list of models, i = %i", i));
    packed &= ~(1 << (31 - i));
    last = model->weight;
  }
  return packed;
}

#define XLINK_RATIO(packed, bytes) (100*(1 - (((double)(packed))/(bytes))))

void xlink_modeler_print(xlink_modeler *mod, xlink_list *models) {
  if (xlink_list_length(models) > 0) {
    double entropy;
    int bytes;
    int i;
    entropy = xlink_modeler_get_entropy(mod, models);
    entropy -= 8*(4 + xlink_list_length(models));
    bytes = ceil(entropy/8);
    printf("Found context: %i model(s), %0.3lf bits, %i bytes\n",
     xlink_list_length(models), entropy, bytes);
    for (i = 0; i < xlink_list_length(models); i++) {
      xlink_model *model;
      model = xlink_list_get(models, i);
      printf("(%02x, %i) ", model->mask, model->weight);
    }
    printf("\n");
    printf("Packed weights = %08x\n", xlink_compute_packed_weights(models));
  }
  else {
    printf("No context found.\n");
  }
}

void xlink_modeler_search(xlink_modeler *mod, xlink_list *models) {
  int contains[256];
  double best;
  int add_index;
  int del_index;
  printf("Searching for best context... ");
  fflush(stdout);
  xlink_list_empty(models);
  memset(contains, 0, sizeof(contains));
  best = DBL_MAX;
  do {
    int i;
    xlink_model model;
    add_index = del_index = -1;
    /* Try to add a model to the working set */
    for (i = 0; i < 256; i++) {
      if (!contains[i]) {
        double entropy;
        xlink_model_init(&model, i);
        xlink_list_add(models, &model);
        entropy = xlink_modeler_get_entropy(mod, models);
        xlink_list_remove(models, xlink_list_length(models) - 1);
        if (entropy < best) {
          best = entropy;
          add_index = i;
        }
      }
    }
    if (add_index != -1) {
      xlink_model_init(&model, add_index);
      xlink_list_add(models, &model);
      contains[add_index] = 1;
    }
    /* Try to remove a model from the working set */
    for (i = 0; i < models->length; i++) {
      double entropy;
      xlink_list_swap(models, i, models->length - 1);
      models->length--;
      entropy = xlink_modeler_get_entropy(mod, models);
      models->length++;
      xlink_list_swap(models, models->length - 1, i);
      if (entropy < best) {
        best = entropy;
        del_index = i;
      }
    }
    if (del_index != -1) {
      contains[xlink_list_get_model(models, del_index)->mask] = 0;
      xlink_list_remove(models, del_index);
    }
  }
  while (add_index != -1 || del_index != -1);
  printf("done\n");
  qsort(models->data, xlink_list_length(models), sizeof(xlink_model), mod_comp);
  /* Compute the weight state for each model in order */
  {
    unsigned int state;
    int weight;
    int i;
    state = xlink_compute_packed_weights(models);
    weight = 0;
    for (i = 0; state != 0; i++) {
      int test;
      xlink_model *model;
      weight--;
      do {
        weight++;
        test = state >= 0x80000000u;
        state += state;
      }
      while (test);
      if (state == 0) break;
      model = xlink_list_get(models, i);
      XLINK_ERROR(model->weight != weight,
       ("Mismatch when decoding model %02x weight: got %i expected %i",
       model->mask, weight, model->weight));
      model->state = state;
    }
  }
  xlink_modeler_print(mod, models);
}

#if 0

typedef struct xlink_ans_encoder xlink_ans_encoder;

struct xlink_ans_encoder {
  xlink_context *ctx;
  xlink_list bytes;
};

typedef struct xlink_ans_decoder xlink_ans_decoder;

struct xlink_ans_decoder {
  xlink_context *ctx;
  const xlink_list *bytes;
  int pos;
  unsigned int state;
};

#define PROB_BITS (8)
#define PROB_MAX (1 << PROB_BITS)
#define ANS_BITS (15)
#define ANS_BASE (1 << ANS_BITS)
#define IO_BITS (8)
#define IO_BASE (1 << IO_BITS)

#define FLOOR_DIV(x, y) ((x)/(y))
#define CEIL_DIV(x, y) ((x)/(y) + ((x) % (y) != 0))

void xlink_ans_encoder_init(xlink_ans_encoder *enc, xlink_context *ctx) {
  enc->ctx = ctx;
  xlink_list_init(&enc->bytes, sizeof(unsigned char), 0);
}

void xlink_ans_encoder_clear(xlink_ans_encoder *enc) {
  xlink_list_clear(&enc->bytes);
}

void xlink_ans_encoder_write_bytes(xlink_ans_encoder *enc, xlink_list *bytes) {
  xlink_list_append(&enc->bytes, bytes);
}

#define xlink_list_add_prob(list, prob) \
  do { \
    xlink_prob p_; \
    p_ = prob; \
    xlink_list_add(list, &p_); \
  } \
  while (0)

#define xlink_list_get_prob(list, i) ((xlink_prob *)xlink_list_get(list, i))

void xlink_ans_encoder_finalize(xlink_ans_encoder *enc, xlink_bitstream *bs) {
  xlink_list probs;
  int i, j;
  unsigned int state;
  /* ANS requires we precompute all probabilities used to encode bitstream. */
  xlink_list_init(&probs, sizeof(xlink_prob), 8*xlink_list_length(&enc->bytes));
  for (j = 0; j < xlink_list_length(&enc->bytes); j++) {
    unsigned char byte;
    unsigned char partial;
    byte = *xlink_list_get_byte(&enc->bytes, j);
    partial = 1;
    /* Build partially seen byte from high bit to low bit to match decoder. */
    for (i = 8; i-- > 0; ) {
      unsigned int counts[2];
      xlink_prob prob;
      xlink_context_get_counts(enc->ctx, partial, counts);
      prob = (xlink_prob)(1 + counts[0]*255UL/(counts[0] + counts[1]));
      XLINK_ERROR(prob == 0, ("Invalid probability, cannot be zero"));
      xlink_list_add_prob(&probs, prob);
      partial <<= 1;
      partial |= !!(byte & (1 << i));
    }
    XLINK_ERROR(partial != byte,
     ("Mismatch between partial %02x and byte %02x", partial, byte));
    xlink_context_update(enc->ctx, byte);
  }
  /* Reserve space to store the state */
  xlink_list_expand_capacity(&bs->bytes, 4);
  bs->bytes.length = 4;
  /* Encode the bitstream by processing bytes in reverse order. */
  state = ANS_BASE;
  for (j = xlink_list_length(&enc->bytes); j-- > 0; ) {
    unsigned char byte;
    byte = *xlink_list_get_byte(&enc->bytes, j);
    /* Add bits from low to high, the opposite order they are decoded. */
    for (i = 0; i < 8; i++) {
      int bit;
      xlink_prob prob;
      bit = !!(byte & (1 << i));
      prob = *xlink_list_get_prob(&probs, j*8 + (7 - i));
      if (bit) {
        prob = PROB_MAX - prob;
      }
      XLINK_ERROR(state >= ANS_BASE * IO_BASE || state < ANS_BASE,
       ("Encoder state %x invalid at symbol %i", state, i));
      if (state >= (prob << (ANS_BITS - PROB_BITS + IO_BITS))) {
        xlink_bitstream_write_byte(bs, state & (IO_BASE - 1));
        state >>= IO_BITS;
      }
      XLINK_ERROR(state >= (prob << (ANS_BITS - PROB_BITS + IO_BITS)),
       ("Need to serialize more state %i at symbol %i", state, i));
      if (bit) {
        state = CEIL_DIV((state + 1) << PROB_BITS, prob) - 1;
      }
      else {
        state = FLOOR_DIV(state << PROB_BITS, prob);
      }
    }
  }
  xlink_list_clear(&probs);
  *((unsigned int *)bs->bytes.data) = state;
  /* Reverse byte order of the bitstream. */
  xlink_list_reverse(&bs->bytes, 4, xlink_list_length(&bs->bytes) - 1);
  bs->bits = 8*xlink_list_length(&bs->bytes);
}

void xlink_ans_decoder_init(xlink_ans_decoder *dec, xlink_context *ctx,
 const xlink_bitstream *bs) {
  memset(dec, 0, sizeof(xlink_ans_decoder));
  XLINK_ERROR(xlink_list_length(&bs->bytes) < 4,
   ("Bitstream does not include initial 4 byte state, length = %i\n",
   xlink_list_length(&bs->bytes)));
  dec->ctx = ctx;
  dec->bytes = &bs->bytes;
  dec->state = *((unsigned int *)dec->bytes->data);
  dec->pos = 4;
}

void xlink_ans_decoder_clear(xlink_ans_decoder *dec) {
}

unsigned char xlink_ans_decoder_read_byte(xlink_ans_decoder *dec) {
  unsigned char byte;
  int i;
  byte = 1;
  for (i = 8; i-- > 0; ) {
    unsigned int counts[2];
    xlink_prob prob;
    int s, t;
    xlink_context_get_counts(dec->ctx, byte, counts);
    prob = (xlink_prob)(1 + counts[0]*255UL/(counts[0] + counts[1]));
    XLINK_ERROR(dec->state >= ANS_BASE * IO_BASE || dec->state < ANS_BASE,
     ("Decoder state %x invalid at position %i", dec->state, dec->pos));
    s = dec->state*(PROB_MAX - prob);
    t = s >> PROB_BITS;
    byte <<= 1;
    dec->state -= t;
    if ((s & (PROB_MAX - 1)) >= prob) {
      dec->state = t;
      byte++;
    }
    if (dec->state < ANS_BASE) {
      XLINK_ERROR(dec->pos >= xlink_list_length(dec->bytes),
       ("Underflow reading byte, pos = %i but size = %i", dec->pos,
       xlink_list_length(dec->bytes)));
      dec->state <<= IO_BITS;
      dec->state |= *xlink_list_get_byte(dec->bytes, dec->pos);
      dec->pos++;
    }
    XLINK_ERROR(dec->state < ANS_BASE,
     ("Need to deserialize more state %i at symbol %i", dec->state, i));
  }
  xlink_context_update(dec->ctx, byte);
  return byte;
}

typedef xlink_ans_encoder xlink_encoder;
#define xlink_encoder_init xlink_ans_encoder_init
#define xlink_encoder_clear xlink_ans_encoder_clear
#define xlink_encoder_write_bytes xlink_ans_encoder_write_bytes
#define xlink_encoder_finalize xlink_ans_encoder_finalize
typedef xlink_ans_decoder xlink_decoder;
#define xlink_decoder_init xlink_ans_decoder_init
#define xlink_decoder_clear xlink_ans_decoder_clear
#define xlink_decoder_read_byte xlink_ans_decoder_read_byte

#else

typedef uint32_t xlink_word;
typedef uint64_t xlink_dword;

typedef struct xlink_range_encoder xlink_range_encoder;

struct xlink_range_encoder {
  xlink_context *ctx;
  xlink_list bytes;
  int bits;
  xlink_word low;
  xlink_word range;
  int zero;
  int ones;
};

typedef struct xlink_range_decoder xlink_range_decoder;

struct xlink_range_decoder {
  xlink_context *ctx;
  const xlink_list *bytes;
  int bits;
  int pos;
  xlink_word low;
  xlink_word range;
  xlink_word value;
};

#define EC_BITS (31)
#define EC_BASE (1U << EC_BITS)
#define EC_HALF (EC_BASE >> 1)
#define EC_MASK (EC_BASE + (EC_BASE - 1))

#define xlink_range_encoder_store(enc, bit) \
  do { \
    xlink_list_expand_capacity(&enc->bytes, (enc->bits & ~7) + 1); \
    XLINK_SET_BIT(enc->bytes.data, enc->bits, bit); \
    enc->bits++; \
  } \
  while (0)

void xlink_range_encoder_emit(xlink_range_encoder *enc, int bit) {
  XLINK_ERROR(bit == 1 && enc->zero == 0,
   ("Got a carry, but have not seen a zero to propogate it into yet"));
  if (bit == 0) {
    if (enc->zero) {
      xlink_range_encoder_store(enc, 0);
    }
    while (enc->ones > 0) {
      xlink_range_encoder_store(enc, 1);
      enc->ones--;
    }
    enc->zero = 1;
  }
  else {
    xlink_range_encoder_store(enc, 1);
    enc->zero = enc->ones > 0;
    while (enc->ones > 1) {
      xlink_range_encoder_store(enc, 0);
      enc->ones--;
    }
    enc->ones = 0;
  }
  enc->bytes.length = (enc->bits + 7)/8;
}

void xlink_range_encoder_write_bit(xlink_range_encoder *enc, xlink_word c0,
 xlink_word c1, int bit) {
  xlink_word s;
  XLINK_ERROR(c0 == 0 || c1 == 0 || (c0 > EC_MASK - c1),
   ("Error invalid counts, c0 = %i and c1 = %i", c0, c1));
  s = ((xlink_dword)enc->range)*c1/(c0 + c1);
  enc->range = bit ? s : enc->range - s;
  if (!bit) {
    /* Handle carry */
    if (s > EC_MASK - enc->low) {
      xlink_range_encoder_emit(enc, 1);
    }
    enc->low += s;
  }
  /* Normalize */
  while (enc->range < EC_BASE) {
    if (enc->low & EC_BASE) {
      enc->ones++;
    }
    else {
      xlink_range_encoder_emit(enc, 0);
    }
    enc->low <<= 1;
    enc->range <<= 1;
  }
}

void xlink_range_encoder_write_bytes(xlink_range_encoder *enc,
 xlink_list *bytes) {
  int i, j;
  for (j = 0; j < xlink_list_length(bytes); j++) {
    unsigned char byte;
    unsigned char partial;
    byte = *xlink_list_get_byte(bytes, j);
    partial = 1;
    /* Build partially seen byte from high bit to low bit to match decoder. */
    for (i = 8; i-- > 0; ) {
      unsigned int counts[2];
      int bit;
      xlink_context_get_counts(enc->ctx, partial, counts);
      bit = !!(byte & (1 << i));
      xlink_range_encoder_write_bit(enc, counts[0], counts[1], bit);
      xlink_context_update_bit(enc->ctx, partial, bit);
      partial <<= 1;
      partial |= bit;
    }
    XLINK_ERROR(partial != byte,
     ("Mismatch between partial %02x and byte %02x", partial, byte));
    for (i = 8; i-- > 1; ) {
      enc->ctx->buf[i] = enc->ctx->buf[i - 1];
    }
    enc->ctx->buf[0] = byte;
  }
}

void xlink_range_encoder_finalize(xlink_range_encoder *enc,
 xlink_bitstream *bs) {
  xlink_word m;
  int s;
  int i;
  m = EC_BASE - 1;
  /* Count how many bits must be output to ensure that the symbols encoded thus
      far will decode correctly regardless of the bits that follow. */
  for (s = 1; ((enc->low + m) | m) - enc->low >= enc->range; s++, m >>= 1);
  XLINK_ERROR(s > 2,
   ("At most two bits need to be written to finalize range coder"));
  if (m > EC_MASK - enc->low) {
    /* Handle carry */
    xlink_range_encoder_emit(enc, 1);
  }
  for (i = 0; i < s; i++) {
    if ((enc->low + m) & (EC_BASE >> i)) {
      enc->ones++;
    }
    else {
      xlink_range_encoder_emit(enc, 0);
    }
  }
  /* Flush any pending bits */
  xlink_range_encoder_emit(enc, 0);
  /* Copy finalized bits to bitstream */
  bs->bits = enc->bits;
  bs->bytes.length = 0;
  xlink_list_append(&bs->bytes, &enc->bytes);
}

void xlink_range_encoder_init(xlink_range_encoder *enc, xlink_context *ctx) {
  enc->ctx = ctx;
  xlink_list_init(&enc->bytes, sizeof(unsigned char), 0);
  enc->bits = 0;
  enc->low = 0;
  enc->range = EC_BASE;
  enc->zero = 0;
  enc->ones = 0;
  /* Start by writing a 1 bit, this is necessary for decoder implementation */
  xlink_range_encoder_write_bit(enc, 1, 1, 1);
  /* Update the context with 1 bit */
  xlink_context_update_bit(enc->ctx, 0, 1);
}

void xlink_range_encoder_clear(xlink_range_encoder *enc) {
  xlink_list_clear(&enc->bytes);
}

int xlink_range_decoder_read_bit(xlink_range_decoder *dec, unsigned int c0,
 unsigned int c1) {
  int bit;
  xlink_word s;
  XLINK_ERROR(c0 == 0 || c1 == 0 || (c0 > EC_MASK - c1),
   ("Error invalid counts, c0 = %i and c1 = %i", c0, c1));
  /* Fill value with bits until EC_BASE <= range < 2*EC_BASE */
  while (dec->range < EC_BASE) {
    dec->low <<= 1;
    dec->range <<= 1;
    dec->value <<= 1;
    dec->value |=
     dec->pos >= dec->bits ? 0 : XLINK_GET_BIT(dec->bytes->data, dec->pos);
    dec->pos++;
  }
  s = ((xlink_dword)dec->range)*c1/(c0 + c1);
  XLINK_ERROR(s == 0 || s >= dec->range, ("Invalid scale value s = %02x", s));
  bit = dec->value < s;
  if (!bit) {
    dec->low += s;
    dec->range -= s;
    dec->value -= s;
  }
  else {
    dec->range = s;
  }
  return bit;
}

unsigned char xlink_range_decoder_read_byte(xlink_range_decoder *dec) {
  unsigned char byte;
  int i;
  byte = 1;
  for (i = 8; i-- > 0; ) {
    unsigned int counts[2];
    int bit;
    xlink_context_get_counts(dec->ctx, byte, counts);
    bit = xlink_range_decoder_read_bit(dec, counts[0], counts[1]);
    xlink_context_update_bit(dec->ctx, byte, bit);
    byte <<= 1;
    byte |= bit;
  }
  for (i = 8; i-- > 1; ) {
    dec->ctx->buf[i] = dec->ctx->buf[i - 1];
  }
  dec->ctx->buf[0] = byte;
  return byte;
}

void xlink_range_decoder_init(xlink_range_decoder *dec, xlink_context *ctx,
 xlink_bitstream *bs) {
  dec->ctx = ctx;
  dec->bytes = &bs->bytes;
  dec->bits = bs->bits;
  dec->pos = 1;
  dec->low = 0;
  dec->range = 1;
  dec->value = 0;
  /* Discard first bit */
  XLINK_ERROR(xlink_range_decoder_read_bit(dec, 1, 1) == 0,
   ("Expected first decoded bit to be 1"));
  /* Update the context with 1 bit */
  xlink_context_update_bit(dec->ctx, 0, 1);
}

void xlink_range_decoder_clear(xlink_range_decoder *dec) {
}

typedef xlink_range_encoder xlink_encoder;
#define xlink_encoder_init xlink_range_encoder_init
#define xlink_encoder_clear xlink_range_encoder_clear
#define xlink_encoder_write_bytes xlink_range_encoder_write_bytes
#define xlink_encoder_finalize xlink_range_encoder_finalize
typedef xlink_range_decoder xlink_decoder;
#define xlink_decoder_init xlink_range_decoder_init
#define xlink_decoder_clear xlink_range_decoder_clear
#define xlink_decoder_read_byte xlink_range_decoder_read_byte

#endif

void xlink_bitstream_from_context(xlink_bitstream *bs, xlink_context *ctx,
 xlink_list *bytes) {
  xlink_encoder enc;
  xlink_decoder dec;
  int i;
  bs->bytes.length = 0;
  /* Reset the context */
  xlink_context_reset(ctx);
  /* Encode bytes with the context */
  xlink_encoder_init(&enc, ctx);
  xlink_encoder_write_bytes(&enc, bytes);
  /* Finalize the bitstream */
  xlink_encoder_finalize(&enc, bs);
  xlink_encoder_clear(&enc);
  /* Reset the context */
  xlink_context_reset(ctx);
  /* Initialize the decoder with the context and bitstream */
  xlink_decoder_init(&dec, ctx, bs);
  /* Test that decoded bytes match original input */
  for (i = 0; i < xlink_list_length(bytes); i++) {
    unsigned char orig;
    unsigned char byte;
    orig = *xlink_list_get_byte(bytes, i);
    byte = xlink_decoder_read_byte(&dec);
    XLINK_ERROR(byte != orig,
     ("Decoder mismatch %02x != %02x at pos = %i", byte, orig, i));
  }
  xlink_decoder_clear(&dec);
}

void xlink_binary_load_stub_modules(xlink_binary *bin) {
  int i;
  for (i = 0; i < sizeof(XLINK_STUB_MODULES)/sizeof(xlink_file); i++) {
    xlink_file file;
    xlink_module *mod;
    file = XLINK_STUB_MODULES[i];
    mod = xlink_file_load_module(&file, 0);
    XLINK_LIST_ADD(binary, module, bin, mod);
  }
}

xlink_segment *xlink_binary_find_segment_by_public(xlink_binary *bin,
 const char *public, xlink_segment_class class) {
  xlink_segment *seg;
  seg = xlink_binary_find_public(bin, public)->segment;
  XLINK_ERROR(xlink_segment_get_class(seg) != class,
   ("Public %s found in segment %s, but class %s not '%s'", public,
   xlink_segment_get_name(seg), xlink_segment_get_class_name(seg),
   XLINK_SEGMENT_CLASS_NAME[class]));
  return seg;
}

void xlink_binary_write_com(xlink_binary *bin, int nsegments) {
  FILE *out;
  int i;
  int offset;
  out = fopen(bin->output, "wb");
  XLINK_ERROR(out == NULL, ("Unable to open output file '%s'", bin->output));
  for (offset = 0x100, i = 1; i <= nsegments; i++) {
    xlink_segment *seg;
    seg = xlink_binary_get_segment(bin, i);
    if (seg->info & SEG_HAS_DATA) {
      if (offset != seg->start) {
        unsigned char buf[4096];
        memset(buf, 0, 4096);
        fwrite(buf, 1, seg->start - offset, out);
        offset = seg->start;
      }
      fwrite(seg->data, 1, seg->length, out);
      offset += seg->length;
    }
  }
  fclose(out);
}

void xlink_binary_link(xlink_binary *bin, unsigned int flags) {
  int bss_idx;
  xlink_segment *start;
  xlink_segment *main;
  xlink_segment *prog;
  int s;
  int i;
  int offset;
  FILE *out;
  /* Stage -1: Load all modules */
  xlink_binary_load_stub_modules(bin);
  /* Stage 0: Find the entry point segment */
  main = xlink_binary_find_segment_by_public(bin, bin->entry, OMF_SEGMENT_CODE);
  bin->is_32bit = main->attrib.proc == OMF_SEGMENT_USE32;
  /* Stage 0a: Set the 16-bit starting segment */
  if (!bin->is_32bit) {
    start = main;
    XLINK_ERROR(bin->init != NULL,
     ("Cannot set 16-bit initializer function when linking 16-bit COM file"));
    /* TODO: Support packing 16-bit programs */
    XLINK_ERROR(flags & MOD_PACK,
     ("Only 32-bit programs can be packed, %s is 16-bit", bin->entry));
  }
  else {
    char *stub;
    if (flags & MOD_PACK) {
      /* Load the 32-bit unpacking stub */
      XLINK_ERROR(bin->init,
       ("Packed executables with 16-bit initializers not supported yet"));
      stub = "stub32c_";
    }
    else {
      /* If there is a 16-bit initialization function, use STUB32I */
      stub = bin->init ? "stub32i_" : "stub32_";
    }
    /* Find the stub segment */
    start = xlink_binary_find_segment_by_public(bin, stub, OMF_SEGMENT_CODE);
    /* If there is an external init_ function, find and rewrite it */
    if (bin->init) {
      strcpy(xlink_module_find_extern(start->module, "init_")->name, bin->init);
    }
    if (flags & MOD_PACK) {
      prog =
       xlink_binary_find_segment_by_public(bin, "ec_bits", OMF_SEGMENT_BSS);
    }
    else {
      /* The stub code calls an external main_ function, find and rewrite it */
      strcpy(xlink_module_find_extern(start->module, "main_")->name, bin->entry);
    }
  }
  /* Stage 1: Resolve all symbol references, starting from start */
  xlink_binary_link_root_segment(bin, start);
  /* Stage 2: Sort segments by class (CODE, DATA, BSS) */
  xlink_sort_segments(bin->segments, bin->nsegments, NULL, &bss_idx);
  /* Stage 2a: Set _MAIN as the first CODE segment */
  xlink_set_first_segment(&bin->segments[0], start);
  if (flags & MOD_PACK) {
    XLINK_ERROR(bss_idx == bin->nsegments,
     ("Error no BSS segment contained in stub32c"));
    /* Stage 2b: Set _PROG as the first BSS segment */
    xlink_set_first_segment(&bin->segments[bss_idx], prog);
  }
  /* Stage 3: Lay segments in memory with proper alignment starting at 100h */
  xlink_binary_layout_segments(bin, 0, 0x100);
  s = bin->nsegments;
  if (flags & MOD_PACK) {
    int data_idx;
    xlink_list code_bytes;
    xlink_list data_bytes;
    xlink_modeler mod;
    xlink_list models;
    xlink_public *segs;
    xlink_public *words;
    /* Stage 4: Resolve all symbol references, starting from 32-bit entry */
    xlink_binary_link_root_segment(bin, main);
    /* Stage 5: Sort segments by class (CODE, DATA, BSS) */
    xlink_sort_segments(bin->segments + s, bin->nsegments - s, &data_idx, NULL);
    /* Stage 5a: Set main as the first CODE segment */
    xlink_set_first_segment(&bin->segments[s], main);
    /* Stage 6: Lay segments in memory with alignment starting at 10010h */
    xlink_binary_layout_segments(bin, s, 0x10010);
    /* Stage 7: Apply relocations to the payload program segments */
    xlink_apply_relocations(bin->segments + s, bin->nsegments - s);
    /* Stage 8: Extract the CODE and DATA segments */
    xlink_list_init(&code_bytes, sizeof(unsigned char), 0);
    offset = xlink_binary_extract_class(bin, s + 0, 0x10010, OMF_SEGMENT_CODE,
     &code_bytes);
    xlink_list_init(&data_bytes, sizeof(unsigned char), 0);
    offset = xlink_binary_extract_class(bin, s + data_idx, offset,
     OMF_SEGMENT_DATA, &data_bytes);
    printf("code bytes = %i\n", xlink_list_length(&code_bytes));
    printf("data bytes = %i\n", xlink_list_length(&data_bytes));
    /* Stage 9: Build a context modeler for CODE segment bytes */
    xlink_modeler_init(&mod, xlink_list_length(&code_bytes));
    xlink_modeler_load_binary(&mod, &code_bytes);
    /* Stage 10: Search for the best context to use for CODE segment bytes */
    xlink_list_init(&models, sizeof(xlink_model), 0);
    xlink_modeler_search(&mod, &models);
    XLINK_ERROR(xlink_list_length(&models) == 0,
     ("Error no context models found for CODE segment"));
    /* Stage 10: Compress the CODE and DATA segments independently */
    if (xlink_list_length(&models) > 0) {
      xlink_context ctx;
      xlink_bitstream bs;
      int size;
      /* Create a context from models */
      printf("Using hash table size = %i\n", bin->hash_table_memory/2);
      xlink_context_init(&ctx, &models);
      xlink_context_set_capacity(&ctx, bin->hash_table_memory/2);
      /* Create a bitstream for writing */
      xlink_bitstream_init(&bs);
      /* Encode bytes with the context and perfect hashing */
      xlink_bitstream_from_context(&bs, &ctx, &code_bytes);
      size = 8 + xlink_list_length(&models) + (bs.bits + 7)/8;
      printf("Perfect hashing: %i bits, %i bytes\n", bs.bits, (bs.bits + 7)/8);
      printf("Compressed size: %i bytes -> %2.3lf%% smaller\n", size,
       XLINK_RATIO(size, xlink_list_length(&code_bytes)));
      /* Encode bytes with the context and replacement hashing */
      ctx.matches.equals = NULL;
      xlink_bitstream_from_context(&bs, &ctx, &code_bytes);
      size = 8 + xlink_list_length(&models) + (bs.bits + 7)/8;
      printf("Replace hashing: %i bits, %i bytes\n", bs.bits, (bs.bits + 7)/8);
      printf("Compressed size: %i bytes -> %2.3lf%% smaller\n", size,
       XLINK_RATIO(size, xlink_list_length(&code_bytes)));
      xlink_context_clear(&ctx);
      /* Write payload into the prog segment data */
      {
        xlink_public *ec_bits;
        /* The packed binary data goes in the ec_bits placeholder BSS variable */
        ec_bits = xlink_binary_find_public(bin, "ec_bits");
        ec_bits->offset = 8 + xlink_list_length(&models);
        /* Need to allocate space for the ec_segs header and ec_bits data */
        prog->length = ec_bits->offset + (bs.bits + 7)/8;
        printf("prog->length = %i\n", prog->length);
        prog->data = xlink_malloc(prog->length);
        prog->info |= SEG_HAS_DATA;
        ((unsigned int *)prog->data)[0] =
         0xFFFEFFF0 - xlink_list_length(&code_bytes);
        /* TODO: check parity of byte at ec_segs - ec_bits->length
                 flip the test in the code, and toggle the parity here */
        ((unsigned int *)prog->data)[1] = xlink_compute_packed_weights(&models);
        for (i = 0; i < xlink_list_length(&models); i++) {
          xlink_model *model;
          model = xlink_list_get(&models, i);
          prog->data[8 + i] = model->mask;
        }
        xlink_bitstream_copy_bits(&bs, &prog->data[ec_bits->offset], 0);
        printf("ec_bits->offset = %i\n", ec_bits->offset);
      }
      xlink_bitstream_clear(&bs);
    }
    xlink_list_clear(&code_bytes);
    xlink_list_clear(&data_bytes);
    xlink_modeler_clear(&mod);
    xlink_list_clear(&models);
    segs = xlink_binary_find_public(bin, "hash_table_segs");
    words = xlink_binary_find_public(bin, "hash_table_words");
    segs->offset = (bin->hash_table_memory + 65535)/65536;
    words->offset = bin->hash_table_memory/2;
  }
  /* Stage 4: Apply relocations to the program segments */
  xlink_apply_relocations(bin->segments, s);
  /* Optionally write the map file. */
  if (bin->map != NULL) {
    out = fopen(bin->map, "w");
    xlink_binary_print_map(bin, out);
    fclose(out);
  }
  /* Stage 5: Write the COM file to disk a segment at a time */
  xlink_binary_write_com(bin, s);
}

const char *OPTSTRING = "o:e:i:pM:smdCh";

const struct option OPTIONS[] = {
  { "output", required_argument, NULL, 'o' },
  { "entry", required_argument,  NULL, 'e' },
  { "init", required_argument,   NULL, 'i' },
  { "pack", no_argument,         NULL, 'p' },
  { "memory", required_argument, NULL, 'M' },
  { "split", no_argument,        NULL, 's' },
  { "map", no_argument,          NULL, 'm' },
  { "dump", no_argument,         NULL, 'd' },
  { "check", no_argument,        NULL, 'c' },
  { "help", no_argument,         NULL, 'h' },
  { NULL,   0,                   NULL,  0  }
};

static void usage(const char *argv0) {
  fprintf(stderr, "Usage: %s [options] <modules>\n\n"
   "Options: \n\n"
   "  -o --output <program>           Output file name for linked program.\n"
   "  -e --entry <function>           Entry point for binary (default: main).\n"
   "  -i --init <function>            Optional 16-bit initialization routine.\n"
   "  -p --pack                       Create a compressed binary.\n"
   "  -M --memory <size>              Hash table memory size (default: 12MB).\n"
   "  -m --map                        Generate a linker map file.\n"
   "  -d --dump                       Dump module contents only.\n"
   "  -s --split                      Split segments into linkable pieces.\n"
   "  -C --check                      Compress stdin and print stats.\n"
   "  -h --help                       Display this help and exit.\n",
   argv0);
}

#define FEOF(fp) (ungetc(getc(fp), fp) == EOF)

int main(int argc, char *argv[]) {
  xlink_binary bin;
  int c;
  int opt_index;
  unsigned int flags;
  char mapfile[256];
  xlink_binary_init(&bin);
  flags = 0;
  while ((c = getopt_long(argc, argv, OPTSTRING, OPTIONS, &opt_index)) != EOF) {
    switch (c) {
      case 'o' : {
        bin.output = optarg;
        break;
      }
      case 'e' : {
        bin.entry = optarg;
        break;
      }
      case 'i' : {
        bin.init = optarg;
        break;
      }
      case 'p' : {
        flags |= MOD_PACK;
        break;
      }
      case 'M' : {
        bin.hash_table_memory = atoi(optarg);
        break;
      }
      case 'd' : {
        flags |= MOD_DUMP;
        break;
      }
      case 'm' : {
        flags |= MOD_MAP;
        break;
      }
      case 's' : {
        flags |= MOD_SPLIT;
        break;
      }
      case 'C' : {
        flags |= MOD_CHECK;
        break;
      }
      case 'h' :
      default : {
        usage(argv[0]);
        return EXIT_FAILURE;
      }
    }
  }
  XLINK_ERROR(bin.hash_table_memory & 1,
   ("Specified -M --memory size %i must be even", bin.hash_table_memory));
  if (flags & MOD_CHECK) {
    xlink_list bytes;
    xlink_modeler mod;
    xlink_list models;
    /* Read bytes from stdin */
    xlink_list_init(&bytes, sizeof(unsigned char), 0);
    while (!FEOF(stdin)) {
      xlink_list_add_byte(&bytes, getc(stdin));
    }
    /* Build a context modeler for bytes */
    xlink_modeler_init(&mod, xlink_list_length(&bytes));
    xlink_modeler_load_binary(&mod, &bytes);
    /* Search for the best context to use for bytes */
    xlink_list_init(&models, sizeof(xlink_model), 0);
    xlink_modeler_search(&mod, &models);
    if (xlink_list_length(&models) > 0) {
      xlink_context ctx;
      xlink_bitstream bs;
      int size;
      /* Segment header has 4 weight bytes + n model bytes */
      printf("Required header: 4 weight bytes + %i model byte(s)\n",
       xlink_list_length(&models));
      /* Create a context from models */
      xlink_context_init(&ctx, &models);
      xlink_context_set_capacity(&ctx, bin.hash_table_memory);
      /* Create a bitstream for writing */
      xlink_bitstream_init(&bs);
      /* Encode bytes with the context and perfect hashing */
      xlink_bitstream_from_context(&bs, &ctx, &bytes);
      size = 8 + xlink_list_length(&models) + (bs.bits + 7)/8;
      printf("Perfect hashing: %i bits, %i bytes\n", bs.bits, (bs.bits + 7)/8);
      printf("Compressed size: %i bytes -> %2.3lf%% smaller\n", size,
       XLINK_RATIO(size, xlink_list_length(&bytes)));
      /* Encode bytes with the context and replacement hashing */
      ctx.matches.equals = NULL;
      xlink_bitstream_from_context(&bs, &ctx, &bytes);
      size = 8 + xlink_list_length(&models) + (bs.bits + 7)/8;
      printf("Replace hashing: %i bits, %i bytes\n", bs.bits, (bs.bits + 7)/8);
      printf("Compressed size: %i bytes -> %2.3lf%% smaller\n", size,
       XLINK_RATIO(size, xlink_list_length(&bytes)));
      xlink_context_clear(&ctx);
      xlink_bitstream_clear(&bs);
    }
    /* Clean up */
    xlink_list_clear(&bytes);
    xlink_modeler_clear(&mod);
    xlink_list_clear(&models);
    return EXIT_SUCCESS;
  }
  if (optind == argc) {
    fprintf(stderr, "No <modules> specified!\n\n");
    usage(argv[0]);
    return EXIT_FAILURE;
  }
  if (!(flags & MOD_DUMP) && bin.output == NULL) {
    fprintf(stderr, "Output -o <program> is required!\n\n");
    usage(argv[0]);
    return EXIT_FAILURE;
  }
  if ((flags & MOD_MAP) && !(flags & MOD_DUMP)) {
    int len;
    len = strlen(bin.output);
    strcpy(mapfile, bin.output);
    if (len > 4 && strncasecmp(mapfile + len - 4, ".com", 4) == 0) {
      sprintf(mapfile + len - 4, "%s", ".map");
    }
    else {
      sprintf(mapfile + len, "%s", ".map");
    }
    bin.map = mapfile;
  }
  for (c = optind; c < argc; c++) {
    unsigned char *buf;
    xlink_file file;
    buf = xlink_file_init(&file, argv[c]);
    switch (buf[0]) {
      case OMF_THEADR :
      case OMF_LHEADR : {
        xlink_module *mod;
        mod = xlink_file_load_module(&file, flags);
        XLINK_LIST_ADD(binary, module, &bin, mod);
        break;
      }
      case OMF_LIBHDR : {
        xlink_library *lib;
        lib = xlink_file_load_library(&file, flags);
        XLINK_LIST_ADD(binary, library, &bin, lib);
        break;
      }
      default : {
        XLINK_ERROR(1,
         ("Unsupported file format, unknown record type %02X", buf[0]));
      }
    }
    free(buf);
  }
  if (!(flags & MOD_DUMP)) {
    xlink_binary_link(&bin, flags);
  }
  xlink_binary_clear(&bin);
}
