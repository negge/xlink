#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <getopt.h>
#include <math.h>
#include <float.h>
#include "ec.h"
#include "internal.h"
#include "io.h"
#include "omf.h"
#include "paq.h"
#include "util.h"

static const char *XLINK_SEGMENT_CLASS_NAME[] = {
  "CODE",
  "DATA",
  "BSS",
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
  rec->buf = &file->buf[file->pos];
  rec->idx = 0;
  rec->type = xlink_omf_record_read_byte(rec);
  rec->size = xlink_omf_record_read_word(rec);
  XLINK_ERROR(file->size - 3 < rec->size,
   ("Record extends %i bytes past the end of file %s",
   rec->size - file->size + 3, file->name));
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
#define MOD_ONE   (0x20)
#define MOD_LOW   (0x40)
#define MOD_CLAMP (0x80)
#define MOD_EXIT  (0x100)
#define MOD_BASE  (0x200)

xlink_module *xlink_file_load_omf_module(xlink_file *file, unsigned int flags) {
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

xlink_library *xlink_file_load_omf_library(xlink_file *file,
 unsigned int flags) {
  xlink_library *lib;
  int done;
  lib = xlink_malloc(sizeof(xlink_library));
  done = 0;
  while (file->size > 0 && !done) {
    switch (file->buf[file->pos]) {
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
        mod = xlink_file_load_omf_module(file, flags);
        XLINK_LIST_ADD(library, module, lib, mod);
        /* Page size is always a power of two */
        mask = lib->page_size - 1;
        /* Compute the number of bytes to skip to reach the next page */
        skip = ((file->pos + mask) & ~mask) - file->pos;
        file->pos += skip;
        file->size -= skip;
        break;
      }
      default : {
        XLINK_ERROR(1, ("Unknown record type %02X", file->buf[file->pos]));
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
  for (i = 0; segments[i] != first; i++);
  memmove(&segments[1], &segments[0], sizeof(*segments)*i);
  segments[0] = first;
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

#define xlink_list_get_model(list, i) ((xlink_model *)xlink_list_get(list, i))

#define XLINK_RATIO(packed, bytes) (100*(1 - (((double)(packed))/(bytes))))

void xlink_model_search(xlink_list *models, xlink_list *bytes) {
  xlink_modeler mod;
  /* Build a context modeler for bytes */
  xlink_modeler_init(&mod, xlink_list_length(bytes));
  xlink_modeler_load_binary(&mod, bytes);
  /* Search for the best context to use for bytes */
  xlink_list_empty(models);
  xlink_modeler_search(&mod, models);
  XLINK_ERROR(xlink_list_length(models) == 0,
   ("Error no context models found for bytes"));
  xlink_modeler_clear(&mod);
}

typedef struct xlink_ec_segment xlink_ec_segment;

struct xlink_ec_segment {
  xlink_list bytes;
  xlink_list models;
  unsigned int state;
  unsigned int header_size;
};

void xlink_ec_segment_init(xlink_ec_segment *ec) {
  memset(ec, 0, sizeof(xlink_ec_segment));
  xlink_list_init(&ec->bytes, sizeof(unsigned char), 0);
  xlink_list_init(&ec->models, sizeof(xlink_model), 0);
}

void xlink_ec_segment_clear(xlink_ec_segment *ec) {
  xlink_list_clear(&ec->bytes);
  xlink_list_clear(&ec->models);
}

void xlink_decoder_test_bytes(xlink_decoder *dec, xlink_list *bytes) {
  int i;
  for (i = 0; i < xlink_list_length(bytes); i++) {
    unsigned char orig;
    unsigned char byte;
    orig = *xlink_list_get_byte(bytes, i);
    byte = xlink_decoder_read_byte(dec);
    XLINK_ERROR(byte != orig,
     ("Decoder mismatch %02x != %02x at pos = %i", byte, orig, i));
  }
}

void xlink_bitstream_from_context(xlink_bitstream *bs, xlink_context *ctx,
 xlink_list *bytes) {
  xlink_encoder enc;
  xlink_decoder dec;
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
  xlink_decoder_test_bytes(&dec, bytes);
  xlink_decoder_clear(&dec);
}

void xlink_bitstream_from_segments(xlink_bitstream *bs, xlink_ec_segment *code,
 xlink_ec_segment *data, int capacity, int fast, int clamp) {
  xlink_encoder enc;
  xlink_decoder dec;
  xlink_context ctx;
  int i;
  xlink_bitstream_init(bs);
  /* Create a context from code models */
  xlink_context_init(&ctx, &code->models, capacity, fast, clamp);
  /* Create an encoder from the context */
  xlink_encoder_init(&enc, &ctx);
  /* Encode the code bytes */
  xlink_encoder_write_bytes(&enc, &code->bytes);
  /* If the data segment has any bytes */
  if (xlink_list_length(&data->bytes) > 0) {
    /* Reset the context with the data models */
    ctx.models = &data->models;
    xlink_set_reset(&ctx.matches);
    /* Encode the code bytes */
    xlink_encoder_write_bytes(&enc, &data->bytes);
  }
  /* Finalize the bitstream */
  xlink_encoder_finalize(&enc, bs);
  xlink_encoder_clear(&enc);
  xlink_context_clear(&ctx);
  /* Create a context from code models */
  xlink_context_init(&ctx, &code->models, capacity, fast, clamp);
  /* Initialize the decoder with the context and bitstream */
  xlink_decoder_init(&dec, &ctx, bs);
  /* Test that decoded code bytes match original input */
  xlink_decoder_test_bytes(&dec, &code->bytes);
  /* If the data segment has any bytes */
  if (xlink_list_length(&data->bytes) > 0) {
    /* Reset the context with the data models */
    ctx.models = &data->models;
    xlink_set_reset(&ctx.matches);
    /* Test that decoded data bytes match original input */
    xlink_decoder_test_bytes(&dec, &data->bytes);
  }
  xlink_decoder_clear(&dec);
  xlink_context_clear(&ctx);
}

void xlink_binary_load_modules(xlink_binary *bin) {
  int i;
  for (i = 0; i < sizeof(XLINK_STUB_MODULES)/sizeof(xlink_file); i++) {
    xlink_file file;
    file = XLINK_STUB_MODULES[i];
    if (strstr(file.name, "stub32") == NULL) {
      xlink_module *mod;
      mod = xlink_file_load_omf_module(&file, 0);
      XLINK_LIST_ADD(binary, module, bin, mod);
    }
  }
}

void xlink_binary_load_stub(xlink_binary *bin, const char *stub) {
  char buf[1024];
  xlink_module *mod;
  int i;
  sprintf(buf, "stubs/%s.o", stub);
  mod = NULL;
  for (i = 0; i < sizeof(XLINK_STUB_MODULES)/sizeof(xlink_file); i++) {
    xlink_file file;
    file = XLINK_STUB_MODULES[i];
    if (strcmp(file.name, buf) == 0) {
      mod = xlink_file_load_omf_module(&file, 0);
      XLINK_LIST_ADD(binary, module, bin, mod);
      break;
    }
  }
  XLINK_ERROR(mod == NULL, ("Stub %s not found", stub));
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

void xlink_binary_write_map(xlink_binary *bin) {
  FILE *out;
  out = fopen(bin->map, "w");
  xlink_binary_print_map(bin, out);
  fclose(out);
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

xlink_reloc *xlink_segment_find_reloc(xlink_segment *seg, const char *name) {
  xlink_reloc *ret;
  int i;
  ret = NULL;
  for (i = 1; i <= seg->nrelocs; i++) {
    xlink_reloc *rel;
    rel = xlink_segment_get_reloc(seg, i);
    if (rel->target == OMF_TARGET_EXT) {
      xlink_extern *ext;
      ext = xlink_module_get_extern(seg->module, rel->target_idx);
      if (strcmp(ext->name, name) == 0) {
        XLINK_ERROR(ret != NULL,
         ("Duplicate relocation definition found for name %s in %s", name,
         seg->module->filename));
        ret = rel;
        break;
      }
    }
  }
  XLINK_ERROR(ret == NULL, ("No relocation found for extern name %s", name));
  return ret;
}

unsigned char xlink_binary_get_relative_byte(xlink_binary *bin,
 xlink_segment *prog, int relative) {
  int prog_idx;
  int i;
  unsigned char byte;
  XLINK_ERROR(relative >= 0,
   ("Relative byte index should be negative, %i", relative));
  for (prog_idx = bin->nsegments; prog_idx-- > 0; ) {
    if (bin->segments[prog_idx] == prog) {
      break;
    }
  }
  XLINK_ERROR(prog_idx < 0, ("PROG segment index not found"));
  for (i = prog_idx; relative < 0 && i-- > 0; ) {
    relative += bin->segments[i + 1]->start - bin->segments[i]->start;
  }
  XLINK_ERROR(i < 0, ("Relative byte %i bytes before program start", relative));
  byte = 0;
  if (relative <= bin->segments[i]->length) {
    byte = bin->segments[i]->data[relative];
  }
  return byte;
}

int xlink_parity(unsigned char byte) {
  int bits;
  bits = 0;
  while (byte > 0) {
    bits += byte & 1;
    byte >>= 1;
  }
  return bits & 1;
}

int xlink_header_length(const xlink_list *models) {
  XLINK_ERROR(xlink_list_length(models) == 0,
   ("Invalid header, must have at least one context model"));
  return 8 + xlink_list_length(models);
}

void xlink_binary_set_public_offset(xlink_binary *bin, const char *symb,
 int offset) {
  xlink_public *pub;
  pub = xlink_binary_find_public(bin, symb);
  pub->offset = offset;
}

void xlink_binary_link(xlink_binary *bin, unsigned int flags) {
  int bss_idx;
  xlink_segment *start;
  xlink_segment *main;
  xlink_segment *prog;
  int s;
  /* Stage -1: Load all modules */
  xlink_binary_load_modules(bin);
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
      if (flags & MOD_CLAMP) {
        if (flags & MOD_LOW) {
          if (flags & MOD_BASE) {
            /* Load the 32-bit unpacking stub */
            stub = bin->init ? "stub32pcfbi" : "stub32pcfb";
          }
          else {
            /* Load the 32-bit unpacking stub */
            stub = bin->init ? "stub32pcfi" : "stub32pcf";
          }
        }
        else {
          if (flags & MOD_BASE) {
            /* Load the 32-bit unpacking stub */
            stub = bin->init ? "stub32pcbi" : "stub32pcb";
          }
          else {
            /* Load the 32-bit unpacking stub */
            stub = bin->init ? "stub32pci" : "stub32pc";
          }
        }
      }
      else {
        if (flags & MOD_LOW) {
          if (flags & MOD_BASE) {
            /* Load the 32-bit unpacking stub */
            stub = bin->init ? "stub32pfbi" : "stub32pfb";
          }
          else {
            /* Load the 32-bit unpacking stub */
            stub = bin->init ? "stub32pfi" : "stub32pf";
          }
        }
        else {
          if (flags & MOD_BASE) {
            /* Load the 32-bit unpacking stub */
            stub = bin->init ? "stub32pbi" : "stub32pb";
          }
          else {
            /* Load the 32-bit unpacking stub */
            stub = bin->init ? "stub32pi" : "stub32p";
          }
        }
      }
    }
    else {
      if (flags & MOD_EXIT) {
        if (flags & MOD_BASE) {
          stub = bin->init ? "stub32ebi" : "stub32eb";
        }
        else {
          stub = bin->init ? "stub32ei" : "stub32e";
        }
      }
      else {
        if (flags & MOD_BASE) {
          stub = bin->init ? "stub32bi" : "stub32b";
        }
        else {
          /* If there is a 16-bit initialization function, use STUB32I */
          stub = bin->init ? "stub32i" : "stub32";
        }
      }
    }
    printf("Using %s\n", stub);
    /* Load the stub segment */
    xlink_binary_load_stub(bin, stub);
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
  /* Stage 1a: Link main segment when MOD_EXIT, as start missing main_ extern */
  if (flags & MOD_EXIT) {
    xlink_binary_link_root_segment(bin, main);
  }
  /* Stage 2: Sort segments by class (CODE, DATA, BSS) */
  xlink_sort_segments(bin->segments, bin->nsegments, NULL, &bss_idx);
  /* Stage 2a: Set _MAIN as the first CODE segment */
  xlink_set_first_segment(&bin->segments[0], start);
  /* Stage 2b: Set main as the second CODE segment when MOD_EXIT */
  if (flags & MOD_EXIT) {
    xlink_set_first_segment(&bin->segments[1], main);
  }
  if (flags & MOD_PACK) {
    XLINK_ERROR(bss_idx == bin->nsegments,
     ("Error no BSS segment contained in stub32p"));
    /* Stage 2b: Set _PROG as the first BSS segment */
    xlink_set_first_segment(&bin->segments[bss_idx], prog);
  }
  /* Stage 3: Lay segments in memory with proper alignment starting at 100h */
  xlink_binary_layout_segments(bin, 0, 0x100);
  s = bin->nsegments;
  /* Stage 4: Apply relocations to the program segments */
  xlink_apply_relocations(bin->segments, s);
  if (flags & MOD_PACK) {
    int data_idx;
    int offset;
    xlink_ec_segment code;
    xlink_ec_segment data;
    unsigned char byte;
    xlink_bitstream bs;
    int size;
    if (flags & MOD_BASE) {
      xlink_segment *base;
      base =
       xlink_binary_find_segment_by_public(bin, "_XLINK_base", OMF_SEGMENT_BSS);
      XLINK_ERROR(xlink_binary_has_segment(bin, base),
       ("Error, XLINK_base in 16-bit BSS, but set by 32-bit packing stub"));
    }
    /* Stage 4: Resolve all symbol references, starting from 32-bit entry */
    xlink_binary_link_root_segment(bin, main);
    /* Stage 5: Sort segments by class (CODE, DATA, BSS) */
    xlink_sort_segments(bin->segments + s, bin->nsegments - s, &data_idx, NULL);
    /* Stage 5a: Set main as the first CODE segment */
    xlink_set_first_segment(&bin->segments[s], main);
    if (flags & MOD_BASE) {
      xlink_segment *base;
      base =
       xlink_binary_find_segment_by_public(bin, "_XLINK_base", OMF_SEGMENT_BSS);
      xlink_binary_link_root_segment(bin, base);
      xlink_set_first_segment(&bin->segments[s], base);
      base->start = 0x10000;
      s++;
    }
    /* Stage 6: Lay segments in memory with alignment starting at 10010h */
    xlink_binary_layout_segments(bin, s, 0x10010);
    /* Stage 7: Apply relocations to the payload program segments */
    xlink_apply_relocations(bin->segments + s, bin->nsegments - s);
    /* Stage 8: Extract the CODE and DATA segments */
    xlink_ec_segment_init(&code);
    xlink_ec_segment_init(&data);
    offset = xlink_binary_extract_class(bin, s + 0, 0x10010, OMF_SEGMENT_CODE,
     &code.bytes);
    offset = xlink_binary_extract_class(bin, s + data_idx, offset,
     OMF_SEGMENT_DATA, &data.bytes);
    if (flags & MOD_ONE) {
      /* If only one EC segment, append the data.bytes to code.bytes */
      xlink_list_append(&code.bytes, &data.bytes);
      xlink_list_empty(&data.bytes);
    }
    printf("code bytes = %i\n", xlink_list_length(&code.bytes));
    printf("data bytes = %i\n", xlink_list_length(&data.bytes));
    /* Stage 9: Search for the best context to use for CODE segment bytes */
    xlink_model_search(&code.models, &code.bytes);
    code.header_size = xlink_header_length(&code.models);
    code.state = xlink_model_compute_packed_weights(&code.models);
    if (xlink_list_length(&data.bytes) > 0) {
      /* State 9a: Search for the best context to use for DATA segment bytes */
      xlink_model_search(&data.models, &data.bytes);
      data.header_size = xlink_header_length(&data.models);
      data.state = xlink_model_compute_packed_weights(&data.models);
    }
    byte = xlink_binary_get_relative_byte(bin, prog, -code.header_size);
    /* Set the parity of the CODE and DATA state based on byte */
    if (xlink_list_length(&data.bytes) == 0) {
      if (xlink_parity(byte) == xlink_parity(code.state & 0xff)) {
        code.state ^= 1;
      }
    }
    else {
      if (xlink_parity(byte) != xlink_parity(code.state & 0xff)) {
        code.state ^= 1;
      }
      if (xlink_parity(byte) == xlink_parity(data.state & 0xff)) {
        data.state ^= 1;
      }
    }
    /* Update the model states based on the segment states */
    xlink_model_set_state(&code.models, code.state);
    xlink_model_set_state(&data.models, data.state);
    /* Stage 10: Compress the CODE and DATA segments with perfect hashing */
    xlink_bitstream_from_segments(&bs, &code, &data, 0, flags & MOD_LOW,
     flags & MOD_CLAMP);
    size = code.header_size + data.header_size + (bs.bits + 7)/8;
    printf("Perfect hashing: %i bits, %i bytes\n", bs.bits, (bs.bits + 7)/8);
    printf("Compressed size: %i bytes -> %2.3lf%% smaller\n", size,
     XLINK_RATIO(size, code.bytes.length + data.bytes.length));
    xlink_bitstream_clear(&bs);
    /* Stage 10: Compress the CODE and DATA segments with replacement hashing */
    xlink_bitstream_from_segments(&bs, &code, &data, bin->hash_table_memory/2,
     flags & MOD_LOW, flags & MOD_CLAMP);
    size = code.header_size + data.header_size + (bs.bits + 7)/8;
    printf("Replacement hashing: %i bits, %i bytes\n", bs.bits, (bs.bits + 7)/8);
    printf("Compressed size: %i bytes -> %2.3lf%% smaller\n", size,
     XLINK_RATIO(size, code.bytes.length + data.bytes.length));
    /* Write payload into the prog segment data */
    {
      int i;
      xlink_binary_set_public_offset(bin, "ec_bits",
       code.header_size + data.header_size);
      /* Need to allocate space for the ec_segs header and ec_bits data */
      XLINK_ERROR(size > prog->length,
       ("Compressed binary data %i larger than reserved PROG space %i", size,
       prog->length));
      prog->length = code.header_size + data.header_size + (bs.bits + 7)/8;
      printf("prog->length = %i\n", prog->length);
      prog->data = xlink_malloc(prog->length);
      prog->info |= SEG_HAS_DATA;
      ((unsigned int *)prog->data)[0] =
       0xFFFEFFF0 - code.bytes.length;
      ((unsigned int *)prog->data)[1] = code.state;
      for (i = 0; i < xlink_list_length(&code.models); i++) {
        xlink_model *model;
        model = xlink_list_get(&code.models, i);
        prog->data[8 + i] = model->mask;
      }
      if (xlink_list_length(&data.bytes) > 0) {
        ((unsigned int *)&prog->data[code.header_size])[0] =
         0xFFFEFFF0 - code.bytes.length - data.bytes.length;
        ((unsigned int *)&prog->data[code.header_size])[1] = data.state;
        for (i = 0; i < xlink_list_length(&data.models); i++) {
          xlink_model *model;
          model = xlink_list_get(&data.models, i);
          prog->data[code.header_size + 8 + i] = model->mask;
        }
      }
      xlink_bitstream_copy_bits(&bs, prog->data,
       (code.header_size + data.header_size)*8);
      printf("ec_bits->offset = %i\n", code.header_size + data.header_size);
    }
    xlink_bitstream_clear(&bs);
    /* Fix up the compressing stub */
    {
      xlink_reloc *ec_segs;
      xlink_public *header;
      xlink_binary_set_public_offset(bin, "hash_table_segs",
       (bin->hash_table_memory + 65535)/65536);
      xlink_binary_set_public_offset(bin, "hash_table_words",
       bin->hash_table_memory/2);
      ec_segs = xlink_segment_find_reloc(start, "ec_segs");
      ec_segs->addend.offset = -code.header_size;
      /* Apply relocations again to put the fixups into effect */
      xlink_apply_relocations(bin->segments, s);
      if (!bin->init) {
        xlink_public *heap;
        heap = xlink_binary_find_public(bin, "XLINK_heap_offset");
        start->data[heap->offset] += code.header_size;
      }
      header = xlink_binary_find_public(bin, "XLINK_header_size");
      start->data[header->offset] = code.header_size;
      XLINK_ERROR(
       byte != xlink_binary_get_relative_byte(bin, prog, -code.header_size),
       ("Parity byte %i modified after relocation, %02X != %02X",
       code.header_size, byte,
       xlink_binary_get_relative_byte(bin, prog, -code.header_size)));
      if (!xlink_parity(byte)) {
        /* Flip the direction of branch after segment done */
        start->data[header->offset + 1] ^= 1;
      }
    }
    xlink_ec_segment_clear(&code);
    xlink_ec_segment_clear(&data);
  }
  /* Optionally write the map file. */
  if (bin->map != NULL) {
    xlink_binary_write_map(bin);
  }
  /* Stage 5: Write the COM file to disk a segment at a time */
  xlink_binary_write_com(bin, s);
}

const char *OPTSTRING = "o:e:i:pC1LEBM:smdch";

const struct option OPTIONS[] = {
  { "output", required_argument, NULL, 'o' },
  { "entry", required_argument,  NULL, 'e' },
  { "init", required_argument,   NULL, 'i' },
  { "pack", no_argument,         NULL, 'p' },
  { "one", no_argument,          NULL, '1' },
  { "low", no_argument,          NULL, 'L' },
  { "clamp", no_argument,        NULL, 'C' },
  { "exit", no_argument,         NULL, 'E' },
  { "base", no_argument,         NULL, 'B' },
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
   "  -1 --one                        Use only one EC segment with -p --pack.\n"
   "  -L --low                        Use low complexity hashing function.\n"
   "  -C --clamp                      Clamp raw count at 255 (adds 5 bytes).\n"
   "  -E --exit                       Program will explicitly call exit().\n"
   "  -B --base                       Compute and export XLINK_base symbol.\n"
   "  -M --memory <size>              Hash table memory size (default: 12MB).\n"
   "  -m --map                        Generate a linker map file.\n"
   "  -d --dump                       Dump module contents only.\n"
   "  -s --split                      Split segments into linkable pieces.\n"
   "  -c --check                      Compress stdin and print stats.\n"
   "  -h --help                       Display this help and exit.\n",
   argv0);
}

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
      case '1' : {
        flags |= MOD_ONE;
        break;
      }
      case 'L' : {
        flags |= MOD_LOW;
        break;
      }
      case 'C' : {
        flags |= MOD_CLAMP;
        break;
      }
      case 'E' : {
        flags |= MOD_EXIT;
        break;
      }
      case 'B' : {
        flags |= MOD_BASE;
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
      case 'c' : {
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
  XLINK_ERROR(flags & MOD_PACK && flags & MOD_EXIT,
   ("Specified -E --exit with -p --pack but only valid for unpacked binaries"));
  XLINK_ERROR(bin.hash_table_memory & 1,
   ("Specified -M --memory size %i must be even", bin.hash_table_memory));
  XLINK_ERROR(flags & MOD_ONE && !(flags & MOD_PACK),
   ("Specified -1 --one without -p --pack but only valid for packed binaries"));
  XLINK_ERROR(flags & MOD_LOW && !(flags & MOD_PACK || flags & MOD_CHECK),
   ("Specified -L --low without -p --pack or -c --check command line option"));
  XLINK_ERROR(flags & MOD_CLAMP && !(flags & MOD_PACK || flags & MOD_CHECK),
   ("Specified -C --clamp without -p --pack or -c --check command line option"));
  if (flags & MOD_CHECK) {
    xlink_list bytes;
    xlink_list models;
    /* Read bytes from stdin */
    xlink_list_init(&bytes, sizeof(unsigned char), 0);
    while (!XLINK_FEOF(stdin)) {
      xlink_list_add_byte(&bytes, getc(stdin));
    }
    xlink_list_init(&models, sizeof(xlink_model), 0);
    /* Search for the best context to use for bytes */
    xlink_model_search(&models, &bytes);
    if (xlink_list_length(&models) > 0) {
      xlink_context ctx;
      xlink_bitstream bs;
      int size;
      /* Segment header has 4 weight bytes + n model bytes */
      printf("Required header: 4 weight bytes + %i model byte(s)\n",
       xlink_list_length(&models));
      xlink_model_set_state(&models,
       xlink_model_compute_packed_weights(&models));
      /* Create a context from models */
      xlink_context_init(&ctx, &models, 0, flags & MOD_LOW, flags & MOD_CLAMP);
      /* Create a bitstream for writing */
      xlink_bitstream_init(&bs);
      /* Encode bytes with the context and perfect hashing */
      xlink_bitstream_from_context(&bs, &ctx, &bytes);
      size = 8 + xlink_list_length(&models) + (bs.bits + 7)/8;
      printf("Perfect hashing: %i bits, %i bytes\n", bs.bits, (bs.bits + 7)/8);
      printf("Compressed size: %i bytes -> %2.3lf%% smaller\n", size,
       XLINK_RATIO(size, xlink_list_length(&bytes)));
      /* Encode bytes with the context and replacement hashing */
      printf("Using hash table size = %i\n", bin.hash_table_memory/2);
      xlink_context_set_fixed_capacity(&ctx, bin.hash_table_memory/2);
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
    xlink_file file;
    xlink_file_init(&file, argv[c]);
    switch (file.buf[0]) {
      case OMF_THEADR :
      case OMF_LHEADR : {
        xlink_module *mod;
        mod = xlink_file_load_omf_module(&file, flags);
        XLINK_LIST_ADD(binary, module, &bin, mod);
        break;
      }
      case OMF_LIBHDR : {
        xlink_library *lib;
        lib = xlink_file_load_omf_library(&file, flags);
        XLINK_LIST_ADD(binary, library, &bin, lib);
        break;
      }
      default : {
        XLINK_ERROR(1,
         ("Unsupported file format, unknown record type %02X", file.buf[0]));
      }
    }
    xlink_file_clear(&file);
  }
  if (!(flags & MOD_DUMP)) {
    xlink_binary_link(&bin, flags);
  }
  xlink_binary_clear(&bin);
}
