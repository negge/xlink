#ifndef _XLINK_ec_h
#define _XLINK_ec_h

#include <stdint.h>
#include "paq.h"
#include "util.h"

typedef struct xlink_bitstream xlink_bitstream;

struct xlink_bitstream {
  xlink_list bytes;
  int bits;
};

void xlink_bitstream_init(xlink_bitstream *bs);
void xlink_bitstream_clear(xlink_bitstream *bs);
void xlink_bitstream_write_byte(xlink_bitstream *bs, unsigned char byte);
void xlink_bitstream_copy_bits(xlink_bitstream *bs, unsigned char *dst,
 int pos);

typedef uint32_t xlink_word;
typedef uint64_t xlink_dword;

typedef struct xlink_encoder xlink_encoder;

struct xlink_encoder {
  xlink_context *ctx;
  xlink_list bytes;
  int bits;
  xlink_word low;
  xlink_word range;
  int zero;
  int ones;
};

void xlink_encoder_init(xlink_encoder *enc, xlink_context *ctx);
void xlink_encoder_clear(xlink_encoder *enc);
void xlink_encoder_write_bytes(xlink_encoder *enc, xlink_list *bytes);
void xlink_encoder_finalize(xlink_encoder *enc, xlink_bitstream *bs);

typedef struct xlink_decoder xlink_decoder;

struct xlink_decoder {
  xlink_context *ctx;
  const xlink_list *bytes;
  int bits;
  int pos;
  xlink_word low;
  xlink_word range;
  xlink_word value;
};

void xlink_decoder_init(xlink_decoder *dec, xlink_context *ctx,
 xlink_bitstream *bs);
void xlink_decoder_clear(xlink_decoder *dec);
unsigned char xlink_decoder_read_byte(xlink_decoder *dec);

#endif
