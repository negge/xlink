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

#define SEG_HAS_DATA 0x1

typedef struct xlink_omf_segment xlink_omf_segment;

struct xlink_omf_segment {
  xlink_omf_attribute attrib;
  unsigned int frame;
  unsigned int offset;
  int length;
  int name_idx;
  int class_idx;
  int overlay_idx;

  unsigned int info;
  unsigned char *data;
  unsigned char *mask;
};

void xlink_omf_segment_clear(xlink_omf_segment *segment);

#define CEIL2(len, bits) (((len) + ((1 << (bits)) - 1)) >> (bits))

#define CLEARBIT(mask, idx)  (mask)[(idx)/8] &= ~(1 << ((idx)%8))
#define SETBIT(mask, idx)    (mask)[(idx)/8] |=  (1 << ((idx)%8))
#define GETBIT(mask, idx)   ((mask)[(idx)/8] &   (1 << ((idx)%8)))

typedef struct xlink_omf_group xlink_omf_group;

struct xlink_omf_group {
  int name_idx;
  int segments[256];
  int nsegments;
};

typedef struct xlink_omf_public xlink_omf_public;

struct xlink_omf_public {
  int group_idx;
  int segment_idx;
  int base_frame;
  xlink_omf_string name;
  int offset;
  int type_idx;
};

typedef struct xlink_omf_extern xlink_omf_extern;

struct xlink_omf_extern {
  xlink_omf_string name;
  int type_idx;
};

typedef struct xlink_omf_reloc xlink_omf_reloc;

struct xlink_omf_reloc {
  int segment_idx;
  unsigned int offset;
  int mode;
  xlink_omf_location location;
  xlink_omf_frame frame;
  xlink_omf_target target;
  int frame_idx;
  int target_idx;
  unsigned int disp;
};

typedef struct xlink_omf xlink_omf;

struct xlink_omf {
  xlink_omf_record *records;
  int nrecords;

  int segment_idx;
  int offset;
};

typedef struct xlink_binary xlink_binary;

struct xlink_binary {
  xlink_omf_name *modules;
  int nmodules;
  xlink_omf_name *names;
  int nnames;
  xlink_omf_segment *segments;
  int nsegments;
  xlink_omf_group *groups;
  int ngroups;
  xlink_omf_public *publics;
  int npublics;
  xlink_omf_extern *externs;
  int nexterns;
  xlink_omf_reloc *relocs;
  int nrelocs;
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
  XLINK_ERROR(ptr == NULL, ("Insufficient memory for malloc"));
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

void xlink_binary_init(xlink_binary *bin) {
  memset(bin, 0, sizeof(xlink_binary));
}

void xlink_binary_clear(xlink_binary *bin) {
  int i;
  free(bin->names);
  for (i = 0; i < bin->nsegments; i++) {
    xlink_omf_segment_clear(&bin->segments[i]);
  }
  free(bin->segments);
  free(bin->groups);
  free(bin->publics);
  free(bin->externs);
  free(bin->relocs);
  memset(bin, 0, sizeof(xlink_binary));
}

int xlink_omf_add_record(xlink_omf *omf, xlink_omf_record *rec) {
  omf->nrecords++;
  omf->records =
   xlink_realloc(omf->records, omf->nrecords*sizeof(xlink_omf_record));
  omf->records[omf->nrecords - 1] = *rec;
  return omf->nrecords;
}

int xlink_binary_add_module(xlink_binary *bin, xlink_omf_name *module) {
  bin->nmodules++;
  bin->modules =
   xlink_realloc(bin->modules, bin->nmodules*sizeof(xlink_omf_name));
  bin->modules[bin->nmodules - 1] = *module;
  return bin->nmodules;
}

xlink_omf_name *xlink_binary_get_module(xlink_binary *bin, int module_idx) {
  XLINK_ERROR(module_idx < 1 || module_idx > bin->nmodules,
   ("Could not get module %i, nmodules = %i", module_idx, bin->nmodules));
  return &bin->modules[module_idx - 1];
}

const char *xlink_binary_get_module_name(xlink_binary *bin, int module_idx) {
  static char name[256];
  sprintf(name, "%s:%i", xlink_binary_get_module(bin, module_idx)->name,
   module_idx);
  return name;
}

int xlink_binary_add_name(xlink_binary *bin, xlink_omf_name *name) {
  bin->nnames++;
  bin->names = xlink_realloc(bin->names, bin->nnames*sizeof(xlink_omf_name));
  bin->names[bin->nnames - 1] = *name;
  return bin->nnames;
}

const char *xlink_binary_get_name(xlink_binary *bin, int name_idx) {
  if (name_idx == 0) return "none";
  if (name_idx <= bin->nnames) return bin->names[name_idx - 1].name;
  return "?";
}

int xlink_binary_add_segment(xlink_binary *bin, xlink_omf_segment *segment) {
  bin->nsegments++;
  bin->segments =
   xlink_realloc(bin->segments, bin->nsegments*sizeof(xlink_omf_segment));
  bin->segments[bin->nsegments - 1] = *segment;
  return bin->nsegments;
}

xlink_omf_segment *xlink_binary_get_segment(xlink_binary *bin,
 int segment_idx) {
  XLINK_ERROR(segment_idx < 1 || segment_idx > bin->nsegments,
   ("Could not get segment %i, nsegments = %i", segment_idx, bin->nsegments));
  return &bin->segments[segment_idx - 1];
}

const char *xlink_binary_get_segment_name(xlink_binary *bin, int segment_idx) {
  xlink_omf_segment *seg;
  static char name[256];
  seg = xlink_binary_get_segment(bin, segment_idx);
  sprintf(name, "%s:%i", xlink_binary_get_name(bin, seg->name_idx),
   segment_idx);
  return name;
}

int xlink_binary_add_group(xlink_binary *bin, xlink_omf_group *group) {
  bin->ngroups++;
  bin->groups =
   xlink_realloc(bin->groups, bin->ngroups*sizeof(xlink_omf_group));
  bin->groups[bin->ngroups - 1] = *group;
  return bin->ngroups;
}

const char *xlink_binary_get_group_name(xlink_binary *bin, int group_idx) {
  if (group_idx == 0) return "none";
  if (group_idx <= bin->ngroups) {
    return xlink_binary_get_name(bin, bin->groups[group_idx - 1].name_idx);
  }
  return "?";
}

int xlink_binary_add_public(xlink_binary *bin, xlink_omf_public *public) {
  bin->npublics++;
  bin->publics =
   xlink_realloc(bin->publics, bin->npublics*sizeof(xlink_omf_public));
  bin->publics[bin->npublics - 1] = *public;
  return bin->npublics;
}

int xlink_binary_add_extern(xlink_binary *bin, xlink_omf_extern *ext) {
  bin->nexterns++;
  bin->externs =
   xlink_realloc(bin->externs, bin->nexterns*sizeof(xlink_omf_extern));
  bin->externs[bin->nexterns - 1] = *ext;
  return bin->nexterns;
}

const char *xlink_binary_get_extern_name(xlink_binary *bin, int extern_idx) {
  if (extern_idx == 0) return "none";
  if (extern_idx <= bin->nexterns) {
    return bin->externs[extern_idx - 1].name;
  }
  return "?";
}

int xlink_binary_add_reloc(xlink_binary *bin, xlink_omf_reloc *reloc) {
  bin->nrelocs++;
  bin->relocs =
   xlink_realloc(bin->relocs, bin->nrelocs*sizeof(xlink_omf_reloc));
  bin->relocs[bin->nrelocs - 1] = *reloc;
  return bin->nrelocs;
}

xlink_omf_reloc *xlink_bin_get_reloc(xlink_binary *bin, int reloc_idx) {
  XLINK_ERROR(reloc_idx < 1 || reloc_idx > bin->nrelocs,
   ("Could not get reloc %i, nrelocs = %i", reloc_idx, bin->nrelocs));
  return &bin->relocs[reloc_idx - 1];
}

const char *xlink_binary_get_reloc_frame(xlink_binary *bin,
 xlink_omf_frame frame, int index) {
  static char buf[256];
  char str[256];
  switch (frame) {
    case OMF_FRAME_SEG : {
      sprintf(str, "segment %s", xlink_binary_get_segment_name(bin, index));
      break;
    }
    case OMF_FRAME_GRP : {
      sprintf(str, "group %s", xlink_binary_get_group_name(bin, index));
      break;
    }
    case OMF_FRAME_EXT : {
      sprintf(str, "extern %s", xlink_binary_get_extern_name(bin, index));
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

const char *xlink_binary_get_reloc_target(xlink_binary *bin,
 xlink_omf_target target, int index) {
  static char buf[256];
  char str[256];
  switch (target) {
    case OMF_TARGET_SEG :
    case OMF_TARGET_SEG_DISP : {
      sprintf(str, "segment %s", xlink_binary_get_segment_name(bin, index));
      break;
    }
    case OMF_TARGET_GRP :
    case OMF_TARGET_GRP_DISP : {
      sprintf(str, "group %s", xlink_binary_get_group_name(bin, index));
      break;
    }
    case OMF_TARGET_EXT :
    case OMF_TARGET_EXT_DISP : {
      sprintf(str, "extern %s", xlink_binary_get_extern_name(bin, index));
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

const char *xlink_binary_get_reloc_addend(xlink_binary *bin,
 xlink_omf_location location, const unsigned char *data) {
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

void xlink_omf_segment_clear(xlink_omf_segment *segment) {
  free(segment->data);
  free(segment->mask);
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

void xlink_omf_dump_names(xlink_omf *omf, xlink_binary *bin) {
  int i, j;
  xlink_omf_record *rec;
  printf("Module: %s\n", xlink_binary_get_module_name(bin, bin->nmodules));
  if (bin->nnames > 0) {
    printf("Local names:\n");
    for (i = 0; i < bin->nnames; i++) {
      printf("%2i : '%s'\n", i, xlink_binary_get_name(bin, i + 1));
    }
  }
  for (i = 0, rec = omf->records; i < omf->nrecords; i++, rec++) {
    xlink_omf_record_reset(rec);
    switch (rec->type) {
      case OMF_COMDEF : {
        printf("Communal names:\n");
        for (j = 0; xlink_omf_record_has_data(rec); j++) {
          const char *str;
          int type_idx;
          int data_type;
          int data_num;
          int data_len;
          str = xlink_omf_record_read_string(rec);
          type_idx = xlink_omf_record_read_index(rec);
          data_type = xlink_omf_record_read_byte(rec);
          switch (data_type) {
            case OMF_DATA_TYPE_FAR : {
              data_num = xlink_omf_record_read_length(rec);
              data_len = xlink_omf_record_read_length(rec);
              printf(" FAR: %i*%i bytes", data_num, data_len);
              break;
            }
            case OMF_DATA_TYPE_NEAR : {
              data_len = xlink_omf_record_read_length(rec);
              printf(" NEAR: %i bytes", data_len);
              break;
            }
          }
        }
      }
    }
  }
}

void xlink_omf_dump_symbols(xlink_omf *omf, xlink_binary *bin) {
  int i, j;
  xlink_omf_record *rec;
  if (bin->npublics > 0) {
    printf("Public names:\n");
    for (i = 0; i < bin->npublics; i++) {
      xlink_omf_public *pub;
      pub = &bin->publics[i];
      printf("%2i : '%s', segment %s, group %s, offset 0x%x, type %i\n", i,
       pub->name, xlink_binary_get_segment_name(bin, pub->segment_idx),
       xlink_binary_get_group_name(bin, pub->group_idx), pub->offset,
       pub->type_idx);
    }
  }
  if (bin->nexterns > 0) {
    printf("External names:\n");
    for (i = 0; i < bin->nexterns; i++) {
      xlink_omf_extern *ext;
      ext = &bin->externs[i];
      printf("%2i : '%s', type %i\n", i, ext->name, ext->type_idx);
    }
  }
  for (i = 0, rec = omf->records; i < omf->nrecords; i++, rec++) {
    xlink_omf_record_reset(rec);
    switch (rec->type) {
      case OMF_CEXTDEF : {
        printf("Communal names:\n");
        for (j = 0; xlink_omf_record_has_data(rec); j++) {
          int name_idx;
          int type_idx;
          name_idx = xlink_omf_record_read_index(rec);
          type_idx = xlink_omf_record_read_index(rec);
          printf("  %2i : %s, Type %i", j, xlink_binary_get_name(bin, name_idx),
           type_idx);
        }
        break;
      }
    }
  }
}

void xlink_omf_dump_segments(xlink_omf *omf, xlink_binary *bin) {
  int i, j;
  printf("Segment records:\n");
  for (i = 0; i < bin->nsegments; i++) {
    xlink_omf_segment *seg;
    seg = &bin->segments[i];
    printf("%2i : %s segment %s %s %s '%s' %08x bytes%s\n", i,
     xlink_binary_get_name(bin, seg->name_idx),
     OMF_SEGDEF_ALIGN[seg->attrib.align], OMF_SEGDEF_USE[seg->attrib.proc],
     OMF_SEGDEF_COMBINE[seg->attrib.combine],
     xlink_binary_get_name(bin, seg->class_idx), seg->length,
     seg->attrib.big ? ", big" : "");
  }
  for (i = 0; i < bin->ngroups; i++) {
    xlink_omf_group *grp;
    grp = &bin->groups[i];
    printf("Group: %s\n", xlink_binary_get_name(bin, grp->name_idx));
    for (j = 0; j < grp->nsegments; j++) {
      printf("%2i : %s\n", j, xlink_binary_get_name(bin, grp->segments[j]));
    }
  }
}

void xlink_omf_dump_relocations(xlink_omf *omf, xlink_binary *bin) {
  int i, j;
  xlink_omf_record *rec;
  for (i = 0; i < bin->nsegments; i++) {
    xlink_omf_segment *seg;
    seg = &bin->segments[i];
    if (seg->info & SEG_HAS_DATA) {
      for (j = 0; j < seg->length; j++) {
        XLINK_ERROR(GETBIT(seg->mask, j) == 0,
         ("Missing data for segment %s, offset = %i",
         xlink_binary_get_segment_name(bin, i + 1), j));
      }
    }
  }
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
         xlink_binary_get_segment_name(bin, segment_idx), offset,
         rec->size - 4);
        break;
      }
      case OMF_LIDATA : {
        segment_idx = xlink_omf_record_read_index(rec);
        offset = xlink_omf_record_read_numeric(rec);
        printf("  LIDATA: segment %s, offset 0x%X, size ",
         xlink_binary_get_segment_name(bin, segment_idx), offset);
        printf(" = %i\n", xlink_omf_record_read_lidata_block(rec));
        break;
      }
      case OMF_COMDAT : {
        break;
      }
    }
  }
  for (i = 0; i < bin->nrelocs; i++) {
    xlink_omf_reloc *rel;
    xlink_omf_segment *seg;
    rel = &bin->relocs[i];
    seg = xlink_binary_get_segment(bin, rel->segment_idx);
    printf("  FIXUPP: %s %s, segment %s, offset 0x%x, %s, %s, %s\n",
     OMF_FIXUP_MODE[rel->mode], OMF_FIXUP_LOCATION[rel->location],
     xlink_binary_get_segment_name(bin, rel->segment_idx), rel->offset,
     xlink_binary_get_reloc_frame(bin, rel->frame, rel->frame_idx),
     xlink_binary_get_reloc_target(bin, rel->target, rel->target_idx),
     xlink_binary_get_reloc_addend(bin, rel->location, seg->data + rel->offset));
  }
}

void xlink_omf_load(xlink_binary *bin, xlink_file *file) {
  xlink_parse_ctx ctx;
  xlink_omf omf;
  xlink_parse_ctx_init(&ctx, file->buf, file->size);
  xlink_omf_init(&omf);
  while (ctx.size > 0) {
    xlink_omf_record rec;
    int i;
    xlink_parse_omf_record(&rec, &ctx);
    xlink_omf_add_record(&omf, &rec);
    switch (rec.type) {
      case OMF_THEADR :
      case OMF_LHEADR : {
        xlink_omf_name module;
        strcpy(module.name, xlink_omf_record_read_string(&rec));
        xlink_binary_add_module(bin, &module);
        break;
      }
      case OMF_LNAMES :
      case OMF_LLNAMES : {
        while (xlink_omf_record_has_data(&rec)) {
          xlink_omf_name name;
          strcpy(name.name, xlink_omf_record_read_string(&rec));
          xlink_binary_add_name(bin, &name);
        }
        break;
      }
      case OMF_SEGDEF : {
        xlink_omf_segment seg;
        seg.attrib.b = xlink_omf_record_read_byte(&rec);
        if (seg.attrib.align == 0) {
          seg.frame = xlink_omf_record_read_word(&rec);
          seg.offset = xlink_omf_record_read_byte(&rec);
        }
        else {
          seg.frame = seg.offset = 0;
        }
        seg.length = xlink_omf_record_read_numeric(&rec);
        XLINK_ERROR(seg.attrib.big && seg.length != 0,
         ("Invalid length for 'big' segment, expected 0 got %i", seg.length));
        seg.name_idx = xlink_omf_record_read_index(&rec);
        XLINK_ERROR(seg.name_idx > bin->nnames,
         ("Segment name index %i not defined", seg.name_idx));
        seg.class_idx = xlink_omf_record_read_index(&rec);
        XLINK_ERROR(seg.class_idx > bin->nnames,
         ("Segment class index %i not defined", seg.class_idx));
        seg.overlay_idx = xlink_omf_record_read_index(&rec);
        XLINK_ERROR(seg.overlay_idx > bin->nnames,
         ("Segment overlay index %i not defined", seg.overlay_idx));
        seg.info = 0;
        seg.data = xlink_malloc(seg.length);
        seg.mask = xlink_malloc(CEIL2(seg.length, 3));
        memset(seg.mask, 0, CEIL2(seg.length, 3));
        xlink_binary_add_segment(bin, &seg);
        break;
      }
      case OMF_GRPDEF : {
        xlink_omf_group grp;
        grp.name_idx = xlink_omf_record_read_index(&rec);
        XLINK_ERROR(grp.name_idx > bin->nnames,
         ("Group name index %i not defined", grp.name_idx));
        grp.nsegments = 0;
        while (xlink_omf_record_has_data(&rec)) {
          unsigned char index;
          index = xlink_omf_record_read_byte(&rec);
          XLINK_ERROR(index != 0xFF, ("Invalid segment index %i", index));
          grp.segments[grp.nsegments++] = xlink_omf_record_read_index(&rec);
          XLINK_ERROR(grp.segments[grp.nsegments - 1] > bin->nsegments,
           ("Segment index %i not defined", grp.segments[grp.nsegments - 1]));
        }
        xlink_binary_add_group(bin, &grp);
        break;
      }
      case OMF_PUBDEF :
      case OMF_LPUBDEF : {
        xlink_omf_public pub;
        pub.group_idx = xlink_omf_record_read_index(&rec);
        pub.segment_idx = xlink_omf_record_read_index(&rec);
        pub.base_frame = 0;
        if (pub.group_idx == 0 && pub.segment_idx == 0) {
          pub.base_frame = xlink_omf_record_read_word(&rec);
        }
        while (xlink_omf_record_has_data(&rec)) {
          strcpy(pub.name, xlink_omf_record_read_string(&rec));
          pub.offset = xlink_omf_record_read_numeric(&rec);
          pub.type_idx = xlink_omf_record_read_index(&rec);
          xlink_binary_add_public(bin, &pub);
        }
        break;
      }
      case OMF_EXTDEF :
      case OMF_LEXTDEF : {
        while (xlink_omf_record_has_data(&rec)) {
          xlink_omf_extern ext;
          strcpy(ext.name, xlink_omf_record_read_string(&rec));
          ext.type_idx = xlink_omf_record_read_index(&rec);
          xlink_binary_add_extern(bin, &ext);
        }
        break;
      }
      case OMF_LEDATA : {
        xlink_omf_segment *seg;
        omf.segment_idx = xlink_omf_record_read_index(&rec);
        XLINK_ERROR(omf.segment_idx < 1 || omf.segment_idx > bin->nsegments,
         ("Segment index %i not defined", omf.segment_idx));
        omf.offset = xlink_omf_record_read_numeric(&rec);
        seg = xlink_binary_get_segment(bin, omf.segment_idx);
        seg->info |= SEG_HAS_DATA;
        for (i = omf.offset; xlink_omf_record_has_data(&rec); i++) {
          XLINK_ERROR(i >= seg->length,
           ("LEDATA wrote past end of segment, offset = %i but length = %i",
           i, seg->length));
          XLINK_ERROR(GETBIT(seg->mask, i),
           ("LEDATA overwrote existing data in segment %s, offset = %i",
           xlink_binary_get_segment_name(bin, omf.segment_idx), i));
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
            xlink_omf_segment *seg;
            xlink_omf_reloc rel;
            xlink_omf_fixup_locat locat;
            xlink_omf_fixup_fixdata fixdata;
            XLINK_ERROR(!omf.segment_idx, ("Got FIXUP before LxDATA record"));
            rel.segment_idx = omf.segment_idx;
            seg = xlink_binary_get_segment(bin, rel.segment_idx);
            locat.b0 = byte;
            locat.b1 = xlink_omf_record_read_byte(&rec);
            XLINK_ERROR(locat.high != 1, ("Expecting FIXUP subrecord"));
            rel.offset = locat.offset + omf.offset;
            rel.mode = locat.mode;
            rel.location = locat.location;
            XLINK_ERROR(seg->length < rel.offset + OMF_FIXUP_SIZE[rel.location],
             ("FIXUP offset %i past segment end %i", rel.offset, seg->length));
            for (i = 0; i < OMF_FIXUP_SIZE[rel.location]; i++) {
              XLINK_ERROR(GETBIT(seg->mask, rel.offset + i) == 0,
               ("FIXUP modifies uninitialized data, location %i",
               rel.offset + i));
            }
            fixdata.b = xlink_omf_record_read_byte(&rec);
            if (!fixdata.th_frame) {
              rel.frame = fixdata.frame;
              rel.frame_idx = 0;
              if (fixdata.frame < OMF_FRAME_LOC) {
                rel.frame_idx = xlink_omf_record_read_index(&rec);
                switch (rel.frame) {
                  case OMF_FRAME_SEG : {
                    XLINK_ERROR(rel.frame_idx > bin->nsegments,
                     ("Segment index %i not defined", rel.frame_idx));
                    break;
                  }
                  case OMF_FRAME_GRP : {
                    XLINK_ERROR(rel.frame_idx > bin->ngroups,
                     ("Group index %i not defined", rel.frame_idx));
                    break;
                  }
                  case OMF_FRAME_EXT : {
                    XLINK_ERROR(rel.frame_idx > bin->nexterns,
                     ("Extern index %i not defined", rel.frame_idx));
                    break;
                  }
                }
              }
            }
            else {
              // TODO: Get frame info from thread in xlink_omf struct
            }
            if (!fixdata.th_target) {
              rel.target = (fixdata.no_disp << 2) + fixdata.target;
              rel.target_idx = xlink_omf_record_read_index(&rec);
              switch (rel.target) {
                case OMF_TARGET_SEG :
                case OMF_TARGET_SEG_DISP : {
                  XLINK_ERROR(rel.target_idx > bin->nsegments,
                   ("Segment index %i not defined", rel.target_idx));
                  break;
                }
                case OMF_TARGET_GRP :
                case OMF_TARGET_GRP_DISP : {
                  XLINK_ERROR(rel.target_idx > bin->ngroups,
                   ("Group index %i not defined", rel.target_idx));
                  break;
                }
                case OMF_TARGET_EXT :
                case OMF_TARGET_EXT_DISP : {
                  XLINK_ERROR(rel.target_idx > bin->nexterns,
                   ("Extern index %i not defined", rel.target_idx));
                  break;
                }
              }
            }
            else {
              // TODO: Get target info from thread in xlink_omf struct
            }
            if (!fixdata.no_disp) {
              rel.disp = xlink_omf_record_read_numeric(&rec);
            }
            xlink_binary_add_reloc(bin, &rel);
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
  xlink_omf_dump_names(&omf, bin);
  xlink_omf_dump_symbols(&omf, bin);
  xlink_omf_dump_segments(&omf, bin);
  xlink_omf_dump_relocations(&omf, bin);
  xlink_omf_clear(&omf);
}

const char *OPTSTRING = "h";

const struct option OPTIONS[] = {
  { "help", no_argument,       NULL, 'h' },
  { NULL,   0,                 NULL,  0  }
};

static void usage(const char *argv0) {
  fprintf(stderr, "Usage: %s [options]\n\n"
   "Options: \n\n"
   "  -h --help                       Display this help and exit.\n",
   argv0);
}

int main(int argc, char *argv[]) {
  int c;
  int opt_index;
  xlink_binary bin;
  while ((c = getopt_long(argc, argv, OPTSTRING, OPTIONS, &opt_index)) != EOF) {
    switch (c) {
      case 'h' :
      default : {
        usage(argv[0]);
        return EXIT_FAILURE;
      }
    }
  }
  xlink_binary_init(&bin);
  for (c = optind; c < argc; c++) {
    xlink_file file;
    xlink_file_init(&file, argv[c]);
    xlink_omf_load(&bin, &file);
    xlink_file_clear(&file);
    /* TODO: Add support for loading multiple OMF files at once. */
    break;
  }
  xlink_binary_clear(&bin);
}
