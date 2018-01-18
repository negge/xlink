#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

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

#define XLINK_MAX_RECORDS (1024)
#define XLINK_MAX_STRLEN (256)
#define XLINK_MAX_NAMES (1024)

typedef struct xlink_omf xlink_omf;

struct xlink_omf {
  xlink_omf_record recs[XLINK_MAX_RECORDS];
  int nrecs;
  char labels[XLINK_MAX_NAMES][XLINK_MAX_STRLEN];
  int nlabels;
  int segments[XLINK_MAX_NAMES];
  int nsegments;
  int groups[XLINK_MAX_NAMES];
  int ngroups;
  int symbols[XLINK_MAX_NAMES];
  int nsymbols;
};

void xlink_omf_add_record(xlink_omf *omf, xlink_omf_record *rec) {
  omf->recs[omf->nrecs++] = *rec;
}

int xlink_omf_add_name(xlink_omf *omf, const char *label) {
  strcpy(omf->labels[omf->nlabels++], label);
  return omf->nlabels;
}

const char *xlink_omf_get_name(xlink_omf *omf, int name_idx) {
  if (name_idx == 0) return "none";
  if (name_idx <= omf->nlabels) return omf->labels[name_idx - 1];
  return "?";
}

void xlink_omf_add_segment_idx(xlink_omf *omf, int segment_idx) {
  /*XLINK_ERROR(omf, omf->nlabels < segment_idx - 1,
   "Error, segment name index not defined");*/
  omf->segments[omf->nsegments++] = segment_idx;
}

const char *xlink_omf_get_segment_name(xlink_omf *omf, int segment_idx) {
  if (segment_idx == 0) return "none";
  if (segment_idx <= omf->nsegments) {
    return omf->labels[omf->segments[segment_idx - 1] - 1];
  }
  return "?";
}

void xlink_omf_add_group_idx(xlink_omf *omf, int group_idx) {
  /*XLINK_ERROR(omf, omf->nlabels < group_idx - 1,
   "Error, segment name index not defined");*/
  omf->groups[omf->ngroups++] = group_idx;
}

const char *xlink_omf_get_group_name(xlink_omf *omf, int group_idx) {
  if (group_idx == 0) return "none";
  if (group_idx <= omf->ngroups) {
    return omf->labels[omf->groups[group_idx - 1] - 1];
  }
  return "?";
}

void xlink_omf_add_symbol_idx(xlink_omf *omf, int symbol_idx) {
  /*XLINK_ERROR(omf, omf->nlabels < symbol_idx - 1,
   "Error, segment name index not defined");*/
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
      //XLINK_ERROR(rec, length > 0x80, "Error, invalid length.");
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

#define XLINK_ERROR(ctx, cond, err) \
  do { \
    if (cond) { \
      (ctx)->error = (err); \
      return; \
    } \
  } \
  while (0)

static void xlink_parse_omf_record(xlink_omf_record *rec,
 xlink_parse_ctx *ctx) {
  unsigned char checksum;
  XLINK_ERROR(ctx, ctx->size < 3, "Error, 3 bytes needed for a record.");
  rec->buf = ctx->pos;
  rec->idx = 0;
  rec->type = xlink_omf_record_read_byte(rec);
  rec->size = xlink_omf_record_read_word(rec);
  XLINK_ERROR(ctx, ctx->size - 3 < rec->size,
   "Error, record extends past the end of file.");
  ctx->pos += 3 + rec->size;
  ctx->size -= 3 + rec->size;
  checksum = rec->buf[3 + rec->size - 1];
  if (checksum != 0) {
    int i;
    for (i = 0; i < 3 + rec->size - 1; i++) {
      checksum += rec->buf[i];
    }
  }
  XLINK_ERROR(ctx, checksum != 0, "Error, invalid checksum.");
}

static const char *xlink_omf_record_get_desc(xlink_omf_record_type type) {
  switch (type & ~1) {
    case OMF_THEADR : return "THEADR - Translator Header Record";
    case OMF_LHEADR : return "LHEADR - Library Module Header Record";
    case OMF_EXTDEF : return "EXTDEF - External Names Definition Record";
    case OMF_PUBDEF : return "PUBDEF - Public Names Definition Record";
    case OMF_GRPDEF : return "GRPDEF - Group Definition Record";
    case OMF_LNAMES : return "LNAMES - List of Names Record";
    case OMF_SEGDEF : return "SEGDEF - Segment Definition Record";
    case OMF_LEDATA : return "LEDATA - Logical Enumerated Data Record";
    case OMF_LIDATA : return "LIDATA - Logical Iterated Data Record";
    case OMF_COMDEF : return "COMDEF - Communal Names Definition Record";
    case OMF_CEXTDEF : {
      return "CEXTDEF - COMDAT External Names Definition Record";
    }
    case OMF_COMDAT : return "COMDAT -  Initialized Communal Data Record";
    default : return "?????? - Unknown or Unsupported Record";
  }
}

void xlink_omf_dump_records(xlink_omf *omf) {
  int i;
  xlink_omf_record *rec;
  printf("Summary of records:\n");
  for (i = 0, rec = omf->recs; i < omf->nrecs; i++, rec++) {
    printf("  %02Xh %-48s%3i bytes%s\n", rec->type,
     xlink_omf_record_get_desc(rec->type), 3 + rec->size - 1,
     rec->type & 1 ? " 32-bit" : "");
  }
}

void xlink_omf_dump_names(xlink_omf *omf) {
  int i, j;
  xlink_omf_record *rec;
  for (i = 0, rec = omf->recs; i < omf->nrecs; i++, rec++) {
    xlink_omf_record_reset(rec);
    switch (rec->type) {
      case OMF_THEADR :
      case OMF_LHEADR : {
        printf("Module: %s\n", xlink_omf_record_read_string(rec));
        break;
      }
      case OMF_LNAMES : {
        printf("Local names:\n");
        for (j = 0; xlink_omf_record_has_data(rec); j++) {
          printf("%2i : '%s'\n", j, xlink_omf_record_read_string(rec));
        }
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
      case OMF_PUBDEF : {
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
      case OMF_EXTDEF : {
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
  xlink_omf_record *rec;
  printf("Segment records:\n");
  for (i = j = 0, rec = omf->recs; i < omf->nrecs; i++, rec++) {
    xlink_omf_record_reset(rec);
    switch (rec->type) {
      case OMF_SEGDEF : {
        xlink_omf_attribute attrib;
        unsigned int frame;
        unsigned int offset;
        int length;
        int name_idx;
        int class_idx;
        int overlay_idx;
        attrib.b = xlink_omf_record_read_byte(rec);
        if (attrib.align == 0) {
          frame = xlink_omf_record_read_word(rec);
          offset = xlink_omf_record_read_byte(rec);
        }
        else {
          frame = offset = 0;
        }
        length = xlink_omf_record_read_numeric(rec);
        XLINK_ERROR(rec, attrib.big && length != 0, "Invalid length field.");
        name_idx = xlink_omf_record_read_index(rec);
        class_idx = xlink_omf_record_read_index(rec);
        overlay_idx = xlink_omf_record_read_index(rec);
        /* _TEXT segment BYTE USE16 public 'CODE' */
        printf("%2i : %s segment %s %s %s '%s' %08x bytes%s\n", j++,
         xlink_omf_get_name(omf, name_idx), OMF_SEGDEF_ALIGN[attrib.align],
         OMF_SEGDEF_USE[attrib.proc], OMF_SEGDEF_COMBINE[attrib.combine],
         xlink_omf_get_name(omf, class_idx), length, attrib.big ? ", big" : "");
        break;
      }
      case OMF_GRPDEF : {
        printf("Group: %s\n",
         xlink_omf_get_name(omf, xlink_omf_record_read_index(rec)));
        for (j = 0; xlink_omf_record_has_data(rec); j++) {
          unsigned char index;
          index = xlink_omf_record_read_byte(rec);
          XLINK_ERROR(rec, index != 0xFF, "Error, invalid segment index.");
          printf("%2i : %s\n", j,
           xlink_omf_get_name(omf, xlink_omf_record_read_index(rec)));
        }
        break;
      }
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
  memset(omf, 0, sizeof(xlink_omf));
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
        xlink_omf_attribute attrib;
        attrib.b = xlink_omf_record_read_byte(&rec);
        if (attrib.align == 0) rec.idx += 3;
        xlink_omf_record_read_numeric(&rec);
        xlink_omf_add_segment_idx(omf, xlink_omf_record_read_index(&rec));
        break;
      }
      case OMF_GRPDEF : {
        xlink_omf_add_group_idx(omf, xlink_omf_record_read_index(&rec));
        break;
      }
      case OMF_EXTDEF : {
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

int main(int argc, char *argv[]) {
  xlink_file file;
  xlink_omf omf;
  xlink_file_init(&file, argv[1]);
  xlink_omf_load(&omf, &file);
}
