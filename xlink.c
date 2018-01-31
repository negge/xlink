#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <getopt.h>

typedef enum {
  OMF_THEADR  = 0x80,  // Translator Header Record
  OMF_LHEADR  = 0x82,  // Library Module Header Record
  OMF_EXTDEF  = 0x8C,  // External Names Definition Record
  OMF_PUBDEF  = 0x90,  // (0x91) Public Names Definition Record
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
} xlink_omf_record_type;

typedef enum {
  OMF_DATA_TYPE_FAR  = 0x61,
  OMF_DATA_TYPE_NEAR = 0x62
} xlink_omf_data_type;

typedef union xlink_omf_attribute xlink_omf_attribute;

union xlink_omf_attribute {
  uint8_t b;
  struct {
    /* Packed byte with segment attributes */
    uint8_t proc:1, big:1, combine:3, align:3;
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

typedef struct xlink_file xlink_file;

struct xlink_file {
  const char *name;
  int size;
  unsigned char *buf;
};

void xlink_file_clear(xlink_file *file);
int xlink_file_init(xlink_file *file, const char *name);

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

#define XLINK_MAX_NAMES (1024)

typedef char xlink_omf_string[256];

typedef struct xlink_omf_segment xlink_omf_segment;

struct xlink_omf_segment {
  xlink_omf_attribute attrib;
  unsigned int frame;
  unsigned int offset;
  int length;
  int name_idx;
  int class_idx;
  int overlay_idx;
};

typedef struct xlink_omf_group xlink_omf_group;

struct xlink_omf_group {
  int name_idx;
  int segments[256];
  int nsegments;
};

typedef struct xlink_omf xlink_omf;

struct xlink_omf {
  xlink_omf_record *recs;
  int nrecs;
  xlink_omf_string *labels;
  int nlabels;
  xlink_omf_segment *segments;
  int nsegments;
  xlink_omf_group *groups;
  int ngroups;
  int symbols[XLINK_MAX_NAMES];
  int nsymbols;
};

#define XLINK_ERROR(cond, err) \
  do { \
    if (cond) { \
      fprintf(stderr, "Error: %s\n", err); \
      exit(-1); \
    } \
  } \
  while (0)

void xlink_omf_init(xlink_omf *omf) {
  memset(omf, 0, sizeof(xlink_omf));
}

void xlink_omf_clear(xlink_omf *omf) {
  free(omf->recs);
  free(omf->labels);
  free(omf->segments);
  free(omf->groups);
  memset(omf, 0, sizeof(xlink_omf));
}

int xlink_omf_add_record(xlink_omf *omf, xlink_omf_record *rec) {
  omf->nrecs++;
  omf->recs = realloc(omf->recs, omf->nrecs*sizeof(xlink_omf_record));
  omf->recs[omf->nrecs - 1] = *rec;
  return omf->nrecs;
}

int xlink_omf_add_name(xlink_omf *omf, const char *label) {
  omf->nlabels++;
  omf->labels = realloc(omf->labels, omf->nlabels*sizeof(xlink_omf_string));
  strcpy(omf->labels[omf->nlabels - 1], label);
  return omf->nlabels;
}

const char *xlink_omf_get_name(xlink_omf *omf, int name_idx) {
  if (name_idx == 0) return "none";
  if (name_idx <= omf->nlabels) return omf->labels[name_idx - 1];
  return "?";
}

int xlink_omf_add_segment(xlink_omf *omf, xlink_omf_segment *segment) {
  omf->nsegments++;
  omf->segments =
   realloc(omf->segments, omf->nsegments*sizeof(xlink_omf_segment));
  omf->segments[omf->nsegments - 1] = *segment;
  return omf->nsegments;
}

const char *xlink_omf_get_segment_name(xlink_omf *omf, int segment_idx) {
  if (segment_idx == 0) return "none";
  if (segment_idx <= omf->nsegments) {
    return xlink_omf_get_name(omf, omf->segments[segment_idx - 1].name_idx);
  }
  return "?";
}

int xlink_omf_add_group(xlink_omf *omf, xlink_omf_group *group) {
  omf->ngroups++;
  omf->groups = realloc(omf->groups, omf->ngroups*sizeof(xlink_omf_group));
  omf->groups[omf->ngroups - 1] = *group;
  return omf->ngroups;
}

const char *xlink_omf_get_group_name(xlink_omf *omf, int group_idx) {
  if (group_idx == 0) return "none";
  if (group_idx <= omf->ngroups) {
    return xlink_omf_get_name(omf, omf->groups[group_idx - 1].name_idx);
  }
  return "?";
}

void xlink_omf_add_symbol_idx(xlink_omf *omf, int symbol_idx) {
  XLINK_ERROR(omf->nlabels < symbol_idx - 1, "Segment name index not defined");
  omf->symbols[omf->nsymbols++] = symbol_idx;
}

const char *xlink_omf_get_symbol_name(xlink_omf *omf, int symbol_idx) {
  if (symbol_idx == 0) return "none";
  if (symbol_idx <= omf->nsymbols) {
    return omf->labels[omf->symbols[symbol_idx - 1] - 1];
  }
  return "?";
}

void xlink_file_clear(xlink_file *file) {
  free(file->buf);
  memset(file, 0, sizeof(xlink_file));
}

int xlink_file_init(xlink_file *file, const char *name) {
  FILE *fp;
  int size;
  fp = fopen(name, "rb");
  if (fp == NULL) {
    fprintf(stderr, "Error, could not open OMF file %s\n", name);
    return EXIT_FAILURE;
  }
  memset(file, 0, sizeof(xlink_file));
  file->name = name;
  fseek(fp, 0, SEEK_END);
  file->size = ftell(fp);
  file->buf = malloc(file->size);
  if (file->buf == NULL) {
    fprintf(stderr, "Error, could not allocate %i bytes\n", file->size);
    return EXIT_FAILURE;
  }
  fseek(fp, 0, SEEK_SET);
  size = fread(file->buf, 1, file->size, fp);
  if (size != file->size) {
    fprintf(stderr, "Error reading OMF file, got %i of %i bytes\n", size,
     file->size);
    return EXIT_FAILURE;
  }
  fclose(fp);
  return EXIT_SUCCESS;
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
      XLINK_ERROR(length > 0x80, "Invalid length");
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
  XLINK_ERROR(ctx->size < 3, "3 bytes needed for a record");
  rec->buf = ctx->pos;
  rec->idx = 0;
  rec->type = xlink_omf_record_read_byte(rec);
  rec->size = xlink_omf_record_read_word(rec);
  XLINK_ERROR(ctx->size - 3 < rec->size, "Record extends past the end of file");
  ctx->pos += 3 + rec->size;
  ctx->size -= 3 + rec->size;
  checksum = rec->buf[3 + rec->size - 1];
  if (checksum != 0) {
    int i;
    for (i = 0; i < 3 + rec->size - 1; i++) {
      checksum += rec->buf[i];
    }
  }
  XLINK_ERROR(checksum != 0, "Invalid checksum.");
}

static const char *xlink_omf_record_get_name(xlink_omf_record_type type) {
  switch (type & ~1) {
    case OMF_THEADR : return "THEADR";
    case OMF_LHEADR : return "LHEADR";
    case OMF_EXTDEF : return "EXTDEF";
    case OMF_PUBDEF : return "PUBDEF";
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
    default : return "??????";
  }
}

static const char *xlink_omf_record_get_desc(xlink_omf_record_type type) {
  switch (type & ~1) {
    case OMF_THEADR : return "Translator Header";
    case OMF_LHEADR : return "Library Module Header";
    case OMF_EXTDEF : return "External Names Definition";
    case OMF_PUBDEF : return "Public Names Definition";
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
    default : return "Unknown or Unsupported";
  }
}

void xlink_omf_dump_records(xlink_omf *omf) {
  int i;
  xlink_omf_record *rec;
  printf("Summary of records:\n");
  for (i = 0, rec = omf->recs; i < omf->nrecs; i++, rec++) {
    printf("  %02Xh %-7s - %-32s%6i bytes%s\n", rec->type,
     xlink_omf_record_get_name(rec->type), xlink_omf_record_get_desc(rec->type),
     3 + rec->size - 1, rec->type & 1 ? " 32-bit" : "");
  }
}

void xlink_omf_dump_names(xlink_omf *omf) {
  int i, j;
  xlink_omf_record *rec;
  if (omf->nlabels > 0) {
    printf("Local names:\n");
    for (i = 0; i < omf->nlabels; i++) {
      printf("%2i : '%s'\n", i, xlink_omf_get_name(omf, i + 1));
    }
  }
  for (i = 0, rec = omf->recs; i < omf->nrecs; i++, rec++) {
    xlink_omf_record_reset(rec);
    switch (rec->type) {
      case OMF_THEADR :
      case OMF_LHEADR : {
        printf("Module: %s\n", xlink_omf_record_read_string(rec));
        break;
      }
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

void xlink_omf_dump_symbols(xlink_omf *omf) {
  int i, j;
  xlink_omf_record *rec;
  for (i = 0, rec = omf->recs; i < omf->nrecs; i++, rec++) {
    xlink_omf_record_reset(rec);
    switch (rec->type) {
      case OMF_PUBDEF :
      case OMF_LPUBDEF : {
        int group_idx;
        int segment_idx;
        int base_frame;
        printf("Public names:\n");
        group_idx = xlink_omf_record_read_index(rec);
        segment_idx = xlink_omf_record_read_index(rec);
        base_frame = 0;
        if (group_idx == 0 && segment_idx == 0) {
          base_frame = xlink_omf_record_read_word(rec);
        }
        for (j = 0; xlink_omf_record_has_data(rec); j++) {
          const char *str;
          int offset;
          int type_idx;
          str = xlink_omf_record_read_string(rec);
          offset = xlink_omf_record_read_numeric(rec);
          type_idx = xlink_omf_record_read_index(rec);
          printf("%2i : '%s', segment %s, group %s, offset 0x%x, type %i\n",
           j, str, xlink_omf_get_segment_name(omf, segment_idx),
           xlink_omf_get_group_name(omf, group_idx), offset, type_idx);
        }
        break;
      }
      case OMF_EXTDEF :
      case OMF_LEXTDEF : {
        printf("External names:\n");
        for (j = 0; xlink_omf_record_has_data(rec); j++) {
          const char *str;
          int type_idx;
          str = xlink_omf_record_read_string(rec);
          type_idx = xlink_omf_record_read_index(rec);
          printf("%2i : '%s', type %i\n", j, str, type_idx);
        }
        break;
      }
      case OMF_CEXTDEF : {
        printf("Communal names:\n");
        for (j = 0; xlink_omf_record_has_data(rec); j++) {
          int name_idx;
          int type_idx;
          name_idx = xlink_omf_record_read_index(rec);
          type_idx = xlink_omf_record_read_index(rec);
          printf("  %2i : %s, Type %i", j, xlink_omf_get_name(omf, name_idx),
           type_idx);
        }
        break;
      }
    }
  }
}

void xlink_omf_dump_segments(xlink_omf *omf) {
  int i, j;
  printf("Segment records:\n");
  for (i = 0; i < omf->nsegments; i++) {
    xlink_omf_segment *seg;
    seg = &omf->segments[i];
    printf("%2i : %s segment %s %s %s '%s' %08x bytes%s\n", i,
     xlink_omf_get_name(omf, seg->name_idx),
     OMF_SEGDEF_ALIGN[seg->attrib.align], OMF_SEGDEF_USE[seg->attrib.proc],
     OMF_SEGDEF_COMBINE[seg->attrib.combine],
     xlink_omf_get_name(omf, seg->class_idx), seg->length,
     seg->attrib.big ? ", big" : "");
  }
  for (i = 0; i < omf->ngroups; i++) {
    xlink_omf_group *grp;
    grp = &omf->groups[i];
    printf("Group: %s\n", xlink_omf_get_name(omf, grp->name_idx));
    for (j = 0; j < grp->nsegments; j++) {
      printf("%2i : %s\n", j, xlink_omf_get_name(omf, grp->segments[j]));
    }
  }
}

void xlink_omf_dump_relocations(xlink_omf *omf) {
  int i;
  xlink_omf_record *rec;
  printf("LEDATA, LIDATA, COMDAT and FIXUPP records:\n");
  for (i = 0, rec = omf->recs; i < omf->nrecs; i++, rec++) {
    xlink_omf_record_reset(rec);
    switch (rec->type) {
      case OMF_LEDATA : {
        int segment_idx;
        int offset;
        segment_idx = xlink_omf_record_read_index(rec);
        offset = xlink_omf_record_read_numeric(rec);
        printf("  LEDATA: segment %s, offset 0x%X, size %i\n",
         xlink_omf_get_segment_name(omf, segment_idx), offset, rec->size - 4);
        break;
      }
      case OMF_LIDATA : {
        int segment_idx;
        int offset;
        segment_idx = xlink_omf_record_read_index(rec);
        offset = xlink_omf_record_read_numeric(rec);
        printf("  LIDATA: segment %s, offset 0x%X, size ",
         xlink_omf_get_segment_name(omf, segment_idx), offset);
        printf(" = %i\n", xlink_omf_record_read_lidata_block(rec));
        break;
      }
      case OMF_COMDAT : {
        break;
      }
    }
  }
}

void xlink_omf_load(xlink_omf *omf, xlink_file *file) {
  xlink_parse_ctx ctx;
  xlink_omf_init(omf);
  xlink_parse_ctx_init(&ctx, file->buf, file->size);
  while (ctx.size > 0) {
    xlink_omf_record rec;
    xlink_parse_omf_record(&rec, &ctx);
    xlink_omf_add_record(omf, &rec);
    switch (rec.type) {
      case OMF_LNAMES : {
        while (xlink_omf_record_has_data(&rec)) {
          xlink_omf_add_name(omf, xlink_omf_record_read_string(&rec));
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
        XLINK_ERROR(seg.attrib.big && seg.length != 0, "Invalid length field");
        seg.name_idx = xlink_omf_record_read_index(&rec);
        XLINK_ERROR(seg.name_idx > omf->nlabels,
         "Segment name index not defined");
        seg.class_idx = xlink_omf_record_read_index(&rec);
        XLINK_ERROR(seg.class_idx > omf->nlabels,
         "Segment class index not defined");
        seg.overlay_idx = xlink_omf_record_read_index(&rec);
        XLINK_ERROR(seg.overlay_idx > omf->nlabels,
         "Segment overlay index not defined");
        xlink_omf_add_segment(omf, &seg);
        break;
      }
      case OMF_GRPDEF : {
        xlink_omf_group grp;
        grp.name_idx = xlink_omf_record_read_index(&rec);
        XLINK_ERROR(grp.name_idx > omf->nlabels,
         "Group name index not defined");
        grp.nsegments = 0;
        while (xlink_omf_record_has_data(&rec)) {
          unsigned char index;
          index = xlink_omf_record_read_byte(&rec);
          XLINK_ERROR(index != 0xFF, "Invalid segment index");
          grp.segments[grp.nsegments++] = xlink_omf_record_read_index(&rec);
        }
        xlink_omf_add_group(omf, &grp);
        break;
      }
      case OMF_EXTDEF :
      case OMF_LEXTDEF : {
        while (xlink_omf_record_has_data(&rec)) {
          xlink_omf_add_symbol_idx(omf,
           xlink_omf_add_name(omf, xlink_omf_record_read_string(&rec)));
          xlink_omf_record_read_index(&rec);
        }
        break;
      }
    }
  }
  xlink_omf_dump_records(omf);
  xlink_omf_dump_names(omf);
  xlink_omf_dump_symbols(omf);
  xlink_omf_dump_segments(omf);
  xlink_omf_dump_relocations(omf);
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
  xlink_omf *modules;
  int nmodules;
  while ((c = getopt_long(argc, argv, OPTSTRING, OPTIONS, &opt_index)) != EOF) {
    switch (c) {
      case 'h' :
      default : {
        usage(argv[0]);
        return EXIT_FAILURE;
      }
    }
  }
  for (nmodules = 0, modules = NULL, c = optind; c < argc; c++) {
    xlink_file file;
    xlink_file_init(&file, argv[c]);
    nmodules++;
    modules = realloc(modules, nmodules*sizeof(xlink_omf));
    xlink_omf_load(&modules[nmodules - 1], &file);
    xlink_file_clear(&file);
  }
  for (c = 0; c < nmodules; c++) {
    xlink_omf_clear(&modules[c]);
  }
  free(modules);
}
