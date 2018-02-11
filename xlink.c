#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <getopt.h>

typedef enum {
  OMF_THEADR  = 0x80,  // Translator Header Record
  OMF_LHEADR  = 0x82,  // Library Module Header Record
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

typedef struct xlink_file xlink_file;

struct xlink_file {
  const char *name;
  int size;
  unsigned char *buf;
};

void xlink_file_clear(xlink_file *file);
void xlink_file_init(xlink_file *file, const char *name);

typedef struct xlink_parse_ctx xlink_parse_ctx;

struct xlink_parse_ctx {
  const unsigned char *pos;
  int size;
  const char *error;
};

void xlink_parse_ctx_init(xlink_parse_ctx *ctx, const unsigned char *buf,
 int size);

typedef struct xlink_omf_record xlink_omf_record;

struct xlink_omf_record {
  const unsigned char *buf;
  int idx;
  unsigned char type;
  unsigned short size;
  const char *error;
};

typedef char xlink_omf_string[256];

typedef struct xlink_omf_name xlink_omf_name;

struct xlink_omf_name {
  xlink_omf_string name;
};

typedef struct xlink_omf_module xlink_omf_module;
typedef struct xlink_omf_public xlink_omf_public;
typedef struct xlink_omf_reloc xlink_omf_reloc;

#define SEG_HAS_DATA 0x1

typedef struct xlink_omf_segment xlink_omf_segment;

struct xlink_omf_segment {
  int index;
  xlink_omf_module *module;
  xlink_omf_attribute attrib;
  unsigned int frame;
  unsigned int offset;
  int length;
  xlink_omf_name *name;
  xlink_omf_name *class;
  unsigned int info;
  unsigned char *data;
  unsigned char *mask;
  xlink_omf_public **publics;
  int npublics;
  xlink_omf_reloc **relocs;
  int nrelocs;
  int start;
};

void xlink_omf_segment_clear(xlink_omf_segment *segment);

#define CEIL2(len, bits) (((len) + ((1 << (bits)) - 1)) >> (bits))

#define CLEARBIT(mask, idx)  (mask)[(idx)/8] &= ~(1 << ((idx)%8))
#define SETBIT(mask, idx)    (mask)[(idx)/8] |=  (1 << ((idx)%8))
#define GETBIT(mask, idx)   ((mask)[(idx)/8] &   (1 << ((idx)%8)))

#define ALIGN2(len, bits) (((len) + ((1 << (bits)) - 1)) & ~((1 << (bits)) - 1))

typedef struct xlink_omf_group xlink_omf_group;

struct xlink_omf_group {
  int index;
  xlink_omf_module *module;
  xlink_omf_name *name;
  xlink_omf_segment **segments;
  int nsegments;
};

struct xlink_omf_public {
  int index;
  xlink_omf_module *module;
  xlink_omf_group *group;
  xlink_omf_segment *segment;
  int base_frame;
  xlink_omf_string name;
  int offset;
  int type_idx;
  int is_local;
};

typedef struct xlink_omf_extern xlink_omf_extern;

struct xlink_omf_extern {
  int index;
  xlink_omf_module *module;
  xlink_omf_string name;
  int type_idx;
  int is_local;
  xlink_omf_public *public;
};

typedef struct xlink_omf_addend xlink_omf_addend;

struct xlink_omf_addend {
  unsigned short segment;
  int offset;
};

struct xlink_omf_reloc {
  int index;
  xlink_omf_module *module;
  xlink_omf_segment *segment;
  unsigned int offset;
  int mode;
  xlink_omf_location location;
  xlink_omf_frame frame;
  xlink_omf_target target;
  int frame_idx;
  int target_idx;
  unsigned int disp;
  xlink_omf_addend addend;
};

typedef struct xlink_omf xlink_omf;

struct xlink_omf {
  xlink_omf_record *records;
  int nrecords;
};

struct xlink_omf_module {
  const char *filename;
  int index;
  xlink_omf_string source;
  xlink_omf_name **names;
  int nnames;
  xlink_omf_segment **segments;
  int nsegments;
  xlink_omf_group **groups;
  int ngroups;
  xlink_omf_public **publics;
  int npublics;
  xlink_omf_extern **externs;
  int nexterns;
  xlink_omf_reloc **relocs;
  int nrelocs;
};

typedef struct xlink_binary xlink_binary;

struct xlink_binary {
  char *output;
  char *entry;
  xlink_omf_module **modules;
  int nmodules;
  xlink_omf_public *main;
  xlink_omf_segment **segments;
  int nsegments;
  xlink_omf_extern **externs;
  int nexterns;
};

xlink_omf_module *xlink_binary_get_module(xlink_binary *bin, int module_idx);

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
  XLINK_ERROR(ptr == NULL, ("Insufficient memory for malloc"));
  memset(ptr, 0, size);
  return ptr;
}

void *xlink_realloc(void *ptr, size_t size) {
  ptr = realloc(ptr, size);
  XLINK_ERROR(ptr == NULL, ("Insufficient memory for realloc"));
  return ptr;
}

void xlink_omf_init(xlink_omf *omf) {
  memset(omf, 0, sizeof(xlink_omf));
}

void xlink_omf_clear(xlink_omf *omf) {
  free(omf->records);
}

const char *xlink_segment_get_name(xlink_omf_segment *seg) {
  static char name[256];
  sprintf(name, "%s:%i", seg->name->name, seg->index);
  return name;
}

const char *xlink_segment_get_class_name(xlink_omf_segment *seg) {
  static char name[256];
  sprintf(name, "'%s'", seg->class->name);
  return name;
}

int xlink_segment_get_alignment(xlink_omf_segment *seg) {
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

int xlink_segment_add_public(xlink_omf_segment *seg, xlink_omf_public *public) {
  seg->npublics++;
  seg->publics =
   xlink_realloc(seg->publics, seg->npublics*sizeof(xlink_omf_public *));
  seg->publics[seg->npublics - 1] = public;
  return seg->npublics;
}

int xlink_segment_add_reloc(xlink_omf_segment *seg, xlink_omf_reloc *reloc) {
  seg->nrelocs++;
  seg->relocs =
   xlink_realloc(seg->relocs, seg->nrelocs*sizeof(xlink_omf_reloc *));
  seg->relocs[seg->nrelocs - 1] = reloc;
  return seg->nrelocs;
}

void xlink_group_clear(xlink_omf_group *grp) {
  free(grp->segments);
  memset(grp, 0, sizeof(xlink_omf_group));
}

int xlink_group_add_segment(xlink_omf_group *grp, xlink_omf_segment *segment) {
  grp->nsegments++;
  grp->segments =
   xlink_realloc(grp->segments, grp->nsegments*sizeof(xlink_omf_segment *));
  grp->segments[grp->nsegments - 1] = segment;
  return grp->nsegments;
}

const char *xlink_group_get_name(xlink_omf_group *grp) {
  static char name[256];
  sprintf(name, "%s:%i", grp->name->name, grp->index);
  return name;
}

void xlink_module_init(xlink_omf_module *mod, const char *filename) {
  memset(mod, 0, sizeof(xlink_omf_module));
  mod->filename = filename;
}

void xlink_module_clear(xlink_omf_module *mod) {
  int i;
  for (i = 0; i < mod->nnames; i++) {
    free(mod->names[i]);
  }
  free(mod->names);
  for (i = 0; i < mod->nsegments; i++) {
    xlink_omf_segment_clear(mod->segments[i]);
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
  for (i = 0; i < mod->nrelocs; i++) {
    free(mod->relocs[i]);
  }
  free(mod->relocs);
  memset(mod, 0, sizeof(xlink_omf_module));
}

void xlink_binary_init(xlink_binary *bin) {
  memset(bin, 0, sizeof(xlink_binary));
  bin->entry = "main_";
}

void xlink_binary_clear(xlink_binary *bin) {
  int i;
  for (i = 0; i < bin->nmodules; i++) {
    xlink_module_clear(bin->modules[i]);
    free(bin->modules[i]);
  }
  free(bin->modules);
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

int xlink_binary_add_module(xlink_binary *bin, xlink_omf_module *module) {
  module->index = bin->nmodules;
  bin->nmodules++;
  bin->modules =
   xlink_realloc(bin->modules, bin->nmodules*sizeof(xlink_omf_module *));
  bin->modules[bin->nmodules - 1] = module;
  return bin->nmodules;
}

xlink_omf_module *xlink_binary_get_module(xlink_binary *bin, int module_idx) {
  XLINK_ERROR(module_idx < 1 || module_idx > bin->nmodules,
   ("Could not get module %i, nmodules = %i", module_idx, bin->nmodules));
  return bin->modules[module_idx - 1];
}

const char *xlink_binary_get_module_name(xlink_binary *bin, int module_idx) {
  static char name[256];
  sprintf(name, "%s:%i", xlink_binary_get_module(bin, module_idx)->filename,
   module_idx);
  return name;
}

xlink_omf_public *xlink_binary_find_public(xlink_binary *bin,
 const char *symb) {
  xlink_omf_public *ret;
  int i, j;
  ret = NULL;
  for (i = 0; i < bin->nmodules; i++) {
    xlink_omf_module *mod;
    mod = bin->modules[i];
    for (j = 0; j < mod->npublics; j++) {
      xlink_omf_public *pub;
      pub = mod->publics[j];
      if (!pub->is_local && strcmp(symb, pub->name) == 0) {
        XLINK_ERROR(ret != NULL,
         ("Duplicate public definition found for symbol %s in %s and %s",
         symb, ret->module->filename, mod->filename));
        ret = pub;
      }
    }
  }
  XLINK_ERROR(ret == NULL, ("Could not find public definition %s", symb));
  return ret;
}

int xlink_binary_has_segment(xlink_binary *bin, xlink_omf_segment *segment) {
  int i;
  for (i = 0; i < bin->nsegments; i++) {
    if (bin->segments[i] == segment) {
      return 1;
    }
  }
  return 0;
}

int xlink_binary_has_extern(xlink_binary *bin, xlink_omf_extern *ext) {
  int i;
  for (i = 0; i < bin->nexterns; i++) {
    if (bin->externs[i] == ext) {
      return 1;
    }
  }
  return 0;
}

void xlink_binary_add_segment(xlink_binary *bin, xlink_omf_segment *segment) {
  int i;
  if (xlink_binary_has_segment(bin, segment)) {
    return;
  }
  bin->nsegments++;
  bin->segments =
   xlink_realloc(bin->segments, bin->nsegments*sizeof(xlink_omf_segment *));
  bin->segments[bin->nsegments - 1] = segment;
  for (i = 0; i < segment->nrelocs; i++) {
    xlink_omf_reloc *rel;
    xlink_omf_extern *ext;
    rel = segment->relocs[i];
    XLINK_ERROR(rel->frame != OMF_FRAME_TARG || rel->target != OMF_TARGET_EXT,
     ("Unsupported frame F%i and target F%i", rel->frame, rel->target));
    ext = segment->module->externs[rel->target_idx - 1];
    if (xlink_binary_has_extern(bin, ext)) {
      continue;
    }
    bin->nexterns++;
    bin->externs =
     xlink_realloc(bin->externs, bin->nexterns*sizeof(xlink_omf_extern *));
    bin->externs[bin->nexterns - 1] = ext;
  }
}

void xlink_binary_print_map(xlink_binary *bin, FILE *out) {
  int i;
  fprintf(out, "Segment layout:\n");
  for (i = 0; i < bin->nsegments; i++) {
    xlink_omf_segment *seg;
    seg = bin->segments[i];
    fprintf(out, " %4x %8s segment %s %s %s %6s %08x bytes%s\n",
     seg->start, xlink_segment_get_name(seg),
     OMF_SEGDEF_ALIGN[seg->attrib.align], OMF_SEGDEF_USE[seg->attrib.proc],
     OMF_SEGDEF_COMBINE[seg->attrib.combine], xlink_segment_get_class_name(seg),
     seg->length, seg->attrib.big ? ", big" : "");
  }
}

int xlink_module_add_name(xlink_omf_module *mod, xlink_omf_name *name) {
  mod->nnames++;
  mod->names = xlink_realloc(mod->names, mod->nnames*sizeof(xlink_omf_name *));
  mod->names[mod->nnames - 1] = name;
  return mod->nnames;
}

xlink_omf_name *xlink_module_get_name(xlink_omf_module *mod, int name_idx) {
  XLINK_ERROR(name_idx < 1 || name_idx > mod->nnames,
   ("Could not get name %i, nnames = %i", name_idx, mod->nnames));
  return mod->names[name_idx - 1];
}

int xlink_module_add_segment(xlink_omf_module *mod,
 xlink_omf_segment *segment) {
  segment->index = mod->nsegments;
  segment->module = mod;
  mod->nsegments++;
  mod->segments =
   xlink_realloc(mod->segments, mod->nsegments*sizeof(xlink_omf_segment *));
  mod->segments[mod->nsegments - 1] = segment;
  return mod->nsegments;
}

xlink_omf_segment *xlink_module_get_segment(xlink_omf_module *mod,
 int segment_idx) {
  XLINK_ERROR(segment_idx < 1 || segment_idx > mod->nsegments,
   ("Could not get segment %i, nsegments = %i", segment_idx, mod->nsegments));
  return mod->segments[segment_idx - 1];
}

const char *xlink_module_get_segment_name(xlink_omf_module *mod, int segment_idx) {
  xlink_omf_segment *seg;
  static char name[256];
  seg = xlink_module_get_segment(mod, segment_idx);
  sprintf(name, "%s:%i", seg->name->name, segment_idx);
  return name;
}

int xlink_module_add_group(xlink_omf_module *mod, xlink_omf_group *group) {
  group->index = mod->ngroups;
  group->module = mod;
  mod->ngroups++;
  mod->groups =
   xlink_realloc(mod->groups, mod->ngroups*sizeof(xlink_omf_group *));
  mod->groups[mod->ngroups - 1] = group;
  return mod->ngroups;
}

xlink_omf_group *xlink_module_get_group(xlink_omf_module *mod, int group_idx) {
  XLINK_ERROR(group_idx < 1 || group_idx > mod->ngroups,
   ("Could not get group %i, ngroup = %i", group_idx, mod->ngroups));
  return mod->groups[group_idx - 1];
}

const char *xlink_module_get_group_name(xlink_omf_module *mod, int group_idx) {
  xlink_omf_group *grp;
  static char name[256];
  grp = xlink_module_get_group(mod, group_idx);
  sprintf(name, "%s:%i", grp->name->name, group_idx);
  return name;
}

xlink_omf_public *xlink_module_get_public(xlink_omf_module *mod,
 int public_idx) {
  XLINK_ERROR(public_idx < 1 || public_idx > mod->npublics,
   ("Could not get public %i, npublics = %i\n", public_idx, mod->npublics));
  return mod->publics[public_idx - 1];
}

int xlink_module_add_public(xlink_omf_module *mod, xlink_omf_public *public) {
  public->index = mod->npublics;
  public->module = mod;
  mod->npublics++;
  mod->publics =
   xlink_realloc(mod->publics, mod->npublics*sizeof(xlink_omf_public *));
  mod->publics[mod->npublics - 1] = public;
  return mod->npublics;
}

xlink_omf_public *xlink_module_find_public(xlink_omf_module *mod,
 const char *symb) {
  xlink_omf_public *ret;
  int i;
  ret = NULL;
  for (i = 0; i < mod->npublics; i++) {
    xlink_omf_public *pub;
    pub = mod->publics[i];
    if (pub->is_local && strcmp(symb, pub->name) == 0) {
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

int xlink_module_add_extern(xlink_omf_module *mod, xlink_omf_extern *ext) {
  ext->index = mod->nexterns;
  ext->module = mod;
  mod->nexterns++;
  mod->externs =
   xlink_realloc(mod->externs, mod->nexterns*sizeof(xlink_omf_extern *));
  mod->externs[mod->nexterns - 1] = ext;
  return mod->nexterns;
}

xlink_omf_extern *xlink_module_get_extern(xlink_omf_module *mod,
 int extern_idx) {
  XLINK_ERROR(extern_idx < 1 || extern_idx > mod->nexterns,
   ("Could not get extern %i, nexterns = %i", extern_idx, mod->nexterns));
  return mod->externs[extern_idx - 1];
}

const char *xlink_module_get_extern_name(xlink_omf_module *mod,
 int extern_idx) {
  static char name[256];
  sprintf(name, "%s:%i", xlink_module_get_extern(mod, extern_idx)->name,
   extern_idx);
  return name;
}

xlink_omf_reloc *xlink_module_get_reloc(xlink_omf_module *mod, int reloc_idx) {
  XLINK_ERROR(reloc_idx < 1 || reloc_idx > mod->nrelocs,
   ("Could not get reloc %i, nrelocs = %i", reloc_idx, mod->relocs));
  return mod->relocs[reloc_idx - 1];
}

int xlink_module_add_reloc(xlink_omf_module *mod, xlink_omf_reloc *reloc) {
  reloc->index = mod->nrelocs;
  reloc->module = mod;
  mod->nrelocs++;
  mod->relocs =
   xlink_realloc(mod->relocs, mod->nrelocs*sizeof(xlink_omf_reloc *));
  mod->relocs[mod->nrelocs - 1] = reloc;
  return mod->nrelocs;
}

const char *xlink_module_get_reloc_frame(xlink_omf_module *mod,
 xlink_omf_frame frame, int index) {
  static char buf[256];
  char str[256];
  switch (frame) {
    case OMF_FRAME_SEG : {
      sprintf(str, "segment %s", xlink_module_get_segment_name(mod, index));
      break;
    }
    case OMF_FRAME_GRP : {
      sprintf(str, "group %s", xlink_module_get_group_name(mod, index));
      break;
    }
    case OMF_FRAME_EXT : {
      sprintf(str, "extern %s", xlink_module_get_extern_name(mod, index));
      break;
    }
    case OMF_FRAME_ABS : {
      sprintf(str, "absolute %i", index);
      break;
    }
    case OMF_FRAME_LOC : {
      sprintf(str, "prevloc");
      break;
    }
    case OMF_FRAME_TARG : {
      sprintf(str, "target");
      break;
    }
    default : {
      sprintf(str, "index = %i", index);
    }
  }
  sprintf(buf, "F%i %s", frame, str);
  return buf;
}

const char *xlink_module_get_reloc_target(xlink_omf_module *mod,
 xlink_omf_target target, int index) {
  static char buf[256];
  char str[256];
  switch (target) {
    case OMF_TARGET_SEG :
    case OMF_TARGET_SEG_DISP : {
      sprintf(str, "segment %s", xlink_module_get_segment_name(mod, index));
      break;
    }
    case OMF_TARGET_GRP :
    case OMF_TARGET_GRP_DISP : {
      sprintf(str, "group %s", xlink_module_get_group_name(mod, index));
      break;
    }
    case OMF_TARGET_EXT :
    case OMF_TARGET_EXT_DISP : {
      sprintf(str, "extern %s", xlink_module_get_extern_name(mod, index));
      break;
    }
    case OMF_TARGET_ABS :
    case OMF_TARGET_ABS_DISP : {
      sprintf(str, "absolute %i", index);
      break;
    }
  }
  sprintf(buf, "T%i %s", target, str);
  return buf;
}

const char *xlink_module_get_reloc_addend(xlink_omf_location location,
 const unsigned char *data) {
  static char buf[256];
  char str[256];
  switch (location) {
    case OMF_LOCATION_8BIT :
    case OMF_LOCATION_8BIT_HIGH : {
      sprintf(str, "0x%x", *(uint8_t *)data);
      break;
    }
    case OMF_LOCATION_16BIT :
    case OMF_LOCATION_SEGMENT :
    case OMF_LOCATION_16BIT_LOADER : {
      sprintf(str, "0x%x", *(uint16_t *)data);
      break;
    }
    case OMF_LOCATION_FAR : {
      sprintf(str, "0x%x:0x%x", *(uint16_t *)(data + 2), *(uint16_t *)data);
      break;
    }
    case OMF_LOCATION_32BIT :
    case OMF_LOCATION_32BIT_LOADER : {
      sprintf(str, "0x%x", *(uint32_t *)data);
      break;
    }
    case OMF_LOCATION_48BIT :
    case OMF_LOCATION_48BIT_PHARLAP : {
      sprintf(str, "0x%x:0x%x", *(uint16_t *)(data + 4), *(uint32_t *)data);
      break;
    }
    default : {
      XLINK_ERROR(1, ("Unsupported location %i", location));
    }
  }
  sprintf(buf, "addend %s", str);
  return buf;
}

void xlink_segment_apply_relocations(xlink_omf_segment *seg) {
  int i;
  for (i = 0; i < seg->nrelocs; i++) {
    xlink_omf_reloc *rel;
    xlink_omf_public *pub;
    int offset;
    int target;
    unsigned char *data;
    rel = seg->relocs[i];
    XLINK_ERROR(rel->frame != OMF_FRAME_TARG || rel->target != OMF_TARGET_EXT,
     ("Unsupported frame F%i and target F%i", rel->frame, rel->target));
    pub = seg->module->externs[rel->target_idx - 1]->public;
    offset = seg->start + rel->offset;
    target = pub->segment->start + pub->offset;
    data = seg->data + rel->offset;
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
      default : {
        XLINK_ERROR(1, ("Unsupported location %i", rel->location));
      }
    }
  }
}

void xlink_omf_segment_clear(xlink_omf_segment *segment) {
  free(segment->data);
  free(segment->mask);
  free(segment->publics);
  free(segment->relocs);
  memset(segment, 0, sizeof(xlink_omf_segment));
}

void xlink_file_clear(xlink_file *file) {
  free(file->buf);
  memset(file, 0, sizeof(xlink_file));
}

void xlink_file_init(xlink_file *file, const char *name) {
  FILE *fp;
  int size;
  fp = fopen(name, "rb");
  XLINK_ERROR(fp == NULL, ("Could not open OMF file %s", name));
  memset(file, 0, sizeof(xlink_file));
  file->name = name;
  fseek(fp, 0, SEEK_END);
  file->size = ftell(fp);
  file->buf = xlink_malloc(file->size);
  XLINK_ERROR(file->buf == NULL, ("Could not allocate %i bytes", file->size));
  fseek(fp, 0, SEEK_SET);
  size = fread(file->buf, 1, file->size, fp);
  XLINK_ERROR(size != file->size,
   ("Problem reading OMF file, got %i of %i bytes", size, file->size));
  fclose(fp);
}

void xlink_parse_ctx_init(xlink_parse_ctx *ctx, const unsigned char *buf,
 int size) {
  memset(ctx, 0, sizeof(xlink_parse_ctx));
  ctx->pos = buf;
  ctx->size = size;
}

static void xlink_omf_record_reset(xlink_omf_record *rec) {
  rec->idx = 3;
}

static int xlink_omf_record_has_data(xlink_omf_record *rec) {
  return rec->idx < 3 + rec->size - 1;
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

static void xlink_parse_omf_record(xlink_omf_record *rec,
 xlink_parse_ctx *ctx) {
  unsigned char checksum;
  XLINK_ERROR(ctx->size < 3,
   ("Need 3 bytes for a record but only have %i", ctx->size));
  rec->buf = ctx->pos;
  rec->idx = 0;
  rec->type = xlink_omf_record_read_byte(rec);
  rec->size = xlink_omf_record_read_word(rec);
  XLINK_ERROR(ctx->size - 3 < rec->size,
   ("Record extends %i bytes past the end of file", rec->size - ctx->size + 3));
  ctx->pos += 3 + rec->size;
  ctx->size -= 3 + rec->size;
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

void xlink_omf_dump_lxdata(xlink_omf *omf, xlink_omf_module *mod) {
  int i;
  xlink_omf_record *rec;
  printf("LEDATA, LIDATA, COMDAT and FIXUPP records:\n");
  for (i = 0, rec = omf->records; i < omf->nrecords; i++, rec++) {
    int segment_idx;
    int offset;
    xlink_omf_record_reset(rec);
    switch (rec->type) {
      case OMF_LEDATA : {
        segment_idx = xlink_omf_record_read_index(rec);
        offset = xlink_omf_record_read_numeric(rec);
        printf("  LEDATA: segment %s, offset 0x%X, size %i\n",
         xlink_module_get_segment_name(mod, segment_idx), offset,
         rec->size - 4);
        break;
      }
      case OMF_LIDATA : {
        segment_idx = xlink_omf_record_read_index(rec);
        offset = xlink_omf_record_read_numeric(rec);
        printf("  LIDATA: segment %s, offset 0x%X, size ",
         xlink_module_get_segment_name(mod, segment_idx), offset);
        printf(" = %i\n", xlink_omf_record_read_lidata_block(rec));
        break;
      }
    }
  }
}

void xlink_module_dump_names(xlink_omf_module *mod) {
  int i;
  if (mod->nnames > 0) {
    printf("Local names:\n");
    for (i = 0; i < mod->nnames; i++) {
      printf("%2i : '%s'\n", i, xlink_module_get_name(mod, i + 1)->name);
    }
  }
}

void xlink_module_dump_symbols(xlink_omf_module *mod) {
  int i;
  if (mod->npublics > 0) {
    printf("Public names:\n");
    for (i = 0; i < mod->npublics; i++) {
      xlink_omf_public *pub;
      pub = mod->publics[i];
      printf("%2i : '%s', segment %s, group %s, offset 0x%x, type %i%s\n", i,
       pub->name, pub->segment ? xlink_segment_get_name(pub->segment) : ":0",
       pub->group ? xlink_group_get_name(pub->group) : ":0",
       pub->offset, pub->type_idx, pub->is_local ? " LOCAL" : "");
    }
  }
  if (mod->nexterns > 0) {
    printf("External names:\n");
    for (i = 0; i < mod->nexterns; i++) {
      xlink_omf_extern *ext;
      ext = xlink_module_get_extern(mod, i + 1);
      printf("%2i : '%s', type %i%s\n", i, ext->name, ext->type_idx,
       ext->is_local ? " LOCAL" : "");
    }
  }
}

void xlink_module_dump_segments(xlink_omf_module *mod) {
  int i, j;
  printf("Segment records:\n");
  for (i = 0; i < mod->nsegments; i++) {
    xlink_omf_segment *seg;
    seg = mod->segments[i];
    printf("%2i : %6s segment %s %s %s %6s %08x bytes%s\n", i, seg->name->name,
     OMF_SEGDEF_ALIGN[seg->attrib.align], OMF_SEGDEF_USE[seg->attrib.proc],
     OMF_SEGDEF_COMBINE[seg->attrib.combine], xlink_segment_get_class_name(seg),
     seg->length, seg->attrib.big ? ", big" : "");
  }
  for (i = 0; i < mod->ngroups; i++) {
    xlink_omf_group *grp;
    grp = mod->groups[i];
    printf("Group: %s\n", xlink_group_get_name(grp));
    for (j = 0; j < grp->nsegments; j++) {
      printf("%2i : %s\n", j, xlink_segment_get_name(grp->segments[j]));
    }
  }
}

void xlink_module_dump_relocations(xlink_omf_module *mod) {
  int i, j;
  for (i = 0; i < mod->nsegments; i++) {
    xlink_omf_segment *seg;
    seg = mod->segments[i];
    if (seg->info & SEG_HAS_DATA) {
      for (j = 0; j < seg->length; j++) {
        XLINK_ERROR(GETBIT(seg->mask, j) == 0,
         ("Missing data for segment %s, offset = %i",
         xlink_segment_get_name(seg), j));
      }
    }
  }
  for (i = 0; i < mod->nrelocs; i++) {
    xlink_omf_reloc *rel;
    xlink_omf_segment *seg;
    rel = mod->relocs[i];
    seg = rel->segment;
    printf("  FIXUPP: %s %s, segment %s, offset 0x%x, %s, %s, %s\n",
     OMF_FIXUP_MODE[rel->mode], OMF_FIXUP_LOCATION[rel->location],
     xlink_segment_get_name(seg), rel->offset,
     xlink_module_get_reloc_frame(mod, rel->frame, rel->frame_idx),
     xlink_module_get_reloc_target(mod, rel->target, rel->target_idx),
     xlink_module_get_reloc_addend(rel->location, seg->data + rel->offset));
  }
}

xlink_omf_module *xlink_file_load_module(xlink_file *file) {
  xlink_omf_module *mod;
  xlink_parse_ctx ctx;
  xlink_omf omf;
  xlink_omf_segment *seg;
  int offset;
  mod = xlink_malloc(sizeof(xlink_omf_module));
  xlink_module_init(mod, file->name);
  xlink_parse_ctx_init(&ctx, file->buf, file->size);
  xlink_omf_init(&omf);
  seg = NULL;
  while (ctx.size > 0) {
    xlink_omf_record rec;
    int i;
    xlink_parse_omf_record(&rec, &ctx);
    xlink_omf_add_record(&omf, &rec);
    switch (rec.type) {
      case OMF_THEADR :
      case OMF_LHEADR : {
        strcpy(mod->source, xlink_omf_record_read_string(&rec));
        break;
      }
      case OMF_LNAMES :
      case OMF_LLNAMES : {
        while (xlink_omf_record_has_data(&rec)) {
          xlink_omf_name *name;
          name = xlink_malloc(sizeof(xlink_omf_name));
          strcpy(name->name, xlink_omf_record_read_string(&rec));
          xlink_module_add_name(mod, name);
        }
        break;
      }
      case OMF_SEGDEF : {
        xlink_omf_segment *seg;
        seg = xlink_malloc(sizeof(xlink_omf_segment));
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
        memset(seg->mask, 0, CEIL2(seg->length, 3));
        xlink_module_add_segment(mod, seg);
        break;
      }
      case OMF_GRPDEF : {
        xlink_omf_group *grp;
        grp = xlink_malloc(sizeof(xlink_omf_group));
        grp->name =
         xlink_module_get_name(mod, xlink_omf_record_read_index(&rec));
        grp->nsegments = 0;
        while (xlink_omf_record_has_data(&rec)) {
          unsigned char index;
          index = xlink_omf_record_read_byte(&rec);
          XLINK_ERROR(index != 0xFF, ("Invalid segment index %i", index));
          xlink_group_add_segment(grp,
           xlink_module_get_segment(mod, xlink_omf_record_read_index(&rec)));
        }
        xlink_module_add_group(mod, grp);
        break;
      }
      case OMF_PUBDEF :
      case OMF_LPUBDEF : {
        int group_idx;
        int segment_idx;
        xlink_omf_public base;
        group_idx = xlink_omf_record_read_index(&rec);
        segment_idx = xlink_omf_record_read_index(&rec);
        base.group = group_idx ? xlink_module_get_group(mod, group_idx) : NULL;
        base.segment =
         segment_idx ? xlink_module_get_segment(mod, segment_idx) : NULL;
        base.base_frame =
         base.segment == NULL ? xlink_omf_record_read_word(&rec) : 0;
        base.is_local = (rec.type == OMF_LPUBDEF);
        while (xlink_omf_record_has_data(&rec)) {
          xlink_omf_public *pub;
          pub = xlink_malloc(sizeof(xlink_omf_public));
          *pub = base;
          strcpy(pub->name, xlink_omf_record_read_string(&rec));
          pub->offset = xlink_omf_record_read_numeric(&rec);
          pub->type_idx = xlink_omf_record_read_index(&rec);
          xlink_module_add_public(mod, pub);
          if (pub->segment) {
            xlink_segment_add_public(pub->segment, pub);
          }
        }
        break;
      }
      case OMF_EXTDEF :
      case OMF_LEXTDEF : {
        xlink_omf_extern base;
        base.is_local = (rec.type == OMF_LEXTDEF);
        while (xlink_omf_record_has_data(&rec)) {
          xlink_omf_extern *ext;
          ext = xlink_malloc(sizeof(xlink_omf_extern));
          *ext = base;
          strcpy(ext->name, xlink_omf_record_read_string(&rec));
          ext->type_idx = xlink_omf_record_read_index(&rec);
          xlink_module_add_extern(mod, ext);
        }
        break;
      }
      case OMF_LEDATA : {
        seg = xlink_module_get_segment(mod, xlink_omf_record_read_index(&rec));
        offset = xlink_omf_record_read_numeric(&rec);
        seg->info |= SEG_HAS_DATA;
        for (i = offset; xlink_omf_record_has_data(&rec); i++) {
          XLINK_ERROR(i >= seg->length,
           ("LEDATA wrote past end of segment, offset = %i but length = %i",
           i, seg->length));
          XLINK_ERROR(GETBIT(seg->mask, i),
           ("LEDATA overwrote existing data in segment %s, offset = %i",
           xlink_segment_get_name(seg), i));
          seg->data[i] = xlink_omf_record_read_byte(&rec);
          SETBIT(seg->mask, i);
        }
        break;
      }
      case OMF_FIXUPP : {
        while (xlink_omf_record_has_data(&rec)) {
          unsigned char byte;
          byte = xlink_omf_record_read_byte(&rec);
          if (byte & 0x80) {
            xlink_omf_reloc *rel;
            xlink_omf_fixup_locat locat;
            xlink_omf_fixup_fixdata fixdata;
            const unsigned char *data;
            XLINK_ERROR(seg == NULL, ("Got FIXUP before LxDATA record"));
            rel = xlink_malloc(sizeof(xlink_omf_reloc));
            rel->segment = seg;
            locat.b0 = byte;
            locat.b1 = xlink_omf_record_read_byte(&rec);
            XLINK_ERROR(locat.high != 1, ("Expecting FIXUP subrecord"));
            rel->offset = locat.offset + offset;
            rel->mode = locat.mode;
            rel->location = locat.location;
            XLINK_ERROR(
             seg->length < rel->offset + OMF_FIXUP_SIZE[rel->location],
             ("FIXUP offset %i past segment end %i", rel->offset, seg->length));
            for (i = 0; i < OMF_FIXUP_SIZE[rel->location]; i++) {
              XLINK_ERROR(GETBIT(seg->mask, rel->offset + i) == 0,
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
            xlink_module_add_reloc(mod, rel);
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
  xlink_omf_dump_records(&omf);
  printf("Module: %s\n", mod->filename);
  xlink_module_dump_names(mod);
  xlink_module_dump_symbols(mod);
  xlink_module_dump_segments(mod);
  xlink_omf_dump_lxdata(&omf, mod);
  xlink_module_dump_relocations(mod);
  xlink_omf_clear(&omf);
  return mod;
}

xlink_segment_class xlink_segment_get_class(const xlink_omf_segment *seg) {
  if (strcmp(seg->class->name, "CODE") == 0) {
    return OMF_SEGMENT_CODE;
  }
  if (seg->info & SEG_HAS_DATA) {
    return OMF_SEGMENT_DATA;
  }
  return OMF_SEGMENT_BSS;
}

int seg_comp(const void *a, const void *b) {
  const xlink_omf_segment *seg_a;
  const xlink_omf_segment *seg_b;
  seg_a = *(xlink_omf_segment **)a;
  seg_b = *(xlink_omf_segment **)b;
  if (xlink_segment_get_class(seg_a) == xlink_segment_get_class(seg_b)) {
    if (seg_a->module == seg_b->module) {
      return seg_a->index - seg_b->index;
    }
    return seg_a->module->index - seg_b->module->index;
  }
  return xlink_segment_get_class(seg_a) - xlink_segment_get_class(seg_b);
}

void xlink_binary_link(xlink_binary *bin) {
  int i;
  int offset;
  FILE *out;
  /* Stage 1: Resolve all symbol references, starting from bin->entry */
  bin->main = xlink_binary_find_public(bin, bin->entry);
  xlink_binary_add_segment(bin, bin->main->segment);
  for (i = 0; i < bin->nexterns; i++) {
    xlink_omf_extern *ext;
    ext = bin->externs[i];
    if (ext->is_local) {
      ext->public = xlink_module_find_public(ext->module, ext->name);
    }
    else {
      ext->public = xlink_binary_find_public(bin, ext->name);
    }
    xlink_binary_add_segment(bin, ext->public->segment);
  }
  /* Stage 2: Sort segments by class (CODE, DATA, BSS) with bin->entry first */
  qsort(bin->segments, bin->nsegments, sizeof(xlink_omf_segment *), seg_comp);
  if (bin->segments[0] != bin->main->segment) {
    for (i = 1; i < bin->nsegments; i++) {
      if (bin->segments[i] == bin->main->segment) {
        bin->segments[i] = bin->segments[0];
        bin->segments[0] = bin->main->segment;
        break;
      }
    }
  }
  /* Stage 3: Lay segments in memory with proper alignment starting at 100h */
  offset = 0x100;
  for (i = 0; i < bin->nsegments; i++) {
    xlink_omf_segment *seg;
    seg = bin->segments[i];
    offset = ALIGN2(offset, xlink_segment_get_alignment(seg));
    seg->start = offset;
    offset += seg->length;
  }
  XLINK_ERROR(offset > 65536,
   ("Address space exceeds 65536 bytes, %i", offset));
  xlink_binary_print_map(bin, stdout);
  /* Stage 4: Apply relocations to each segment based on memory location */
  for (i = 0; i < bin->nsegments; i++) {
    xlink_segment_apply_relocations(bin->segments[i]);
  }
  /* Stage 5: Write the COM file to disk a segment at a time */
  out = fopen(bin->output, "wb");
  XLINK_ERROR(out == NULL, ("Unable to open output file '%s'", bin->output));
  for (offset = 0x100, i = 0; i < bin->nsegments; i++) {
    xlink_omf_segment *seg;
    seg = bin->segments[i];
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

const char *OPTSTRING = "o:e:h";

const struct option OPTIONS[] = {
  { "output", required_argument, NULL, 'o' },
  { "entry", required_argument,  NULL, 'e' },
  { "help", no_argument,         NULL, 'h' },
  { NULL,   0,                   NULL,  0  }
};

static void usage(const char *argv0) {
  fprintf(stderr, "Usage: %s [options] <modules> -o <program>\n\n"
   "Options: \n\n"
   "  -o --output <program>           Output file name for linked program.\n"
   "  -e --entry <function>           Entry point for binary (default: main).\n"
   "  -h --help                       Display this help and exit.\n",
   argv0);
}

int main(int argc, char *argv[]) {
  xlink_binary bin;
  int c;
  int opt_index;
  xlink_binary_init(&bin);
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
      case 'h' :
      default : {
        usage(argv[0]);
        return EXIT_FAILURE;
      }
    }
  }
  if (optind == argc) {
    printf("No <modules> specified!\n\n");
    usage(argv[0]);
    return EXIT_FAILURE;
  }
  if (bin.output == NULL) {
    printf("Output -o <program> is required!\n\n");
    usage(argv[0]);
    return EXIT_FAILURE;
  }
  for (c = optind; c < argc; c++) {
    xlink_file file;
    xlink_file_init(&file, argv[c]);
    xlink_binary_add_module(&bin, xlink_file_load_module(&file));
    xlink_file_clear(&file);
    /* TODO: Add support for loading multiple OMF files at once. */
    break;
  }
  xlink_binary_link(&bin);
  xlink_binary_clear(&bin);
}
