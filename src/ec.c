#include <stdlib.h>
#include <string.h>
#include "ec.h"
#include "internal.h"
#include "util.h"

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

void xlink_bitstream_copy_bits(xlink_bitstream *bs, unsigned char *dst,
 int pos) {
  int i;
  /* Intentionally skip the first bit */
  for (i = 1; i < bs->bits; i++, pos++) {
    XLINK_SET_BIT(dst, pos, XLINK_GET_BIT(bs->bytes.data, i));
  }
}

#define EC_BITS (31)
#define EC_BASE (1U << EC_BITS)
#define EC_HALF (EC_BASE >> 1)
#define EC_MASK (EC_BASE + (EC_BASE - 1))

#define xlink_encoder_store(enc, bit) \
  do { \
    xlink_list_expand_capacity(&enc->bytes, (enc->bits & ~7) + 1); \
    XLINK_SET_BIT(enc->bytes.data, enc->bits, bit); \
    enc->bits++; \
  } \
  while (0)

static void xlink_encoder_emit(xlink_encoder *enc, int bit) {
  XLINK_ERROR(bit == 1 && enc->zero == 0,
   ("Got a carry, but have not seen a zero to propogate it into yet"));
  if (bit == 0) {
    if (enc->zero) {
      xlink_encoder_store(enc, 0);
    }
    while (enc->ones > 0) {
      xlink_encoder_store(enc, 1);
      enc->ones--;
    }
    enc->zero = 1;
  }
  else {
    xlink_encoder_store(enc, 1);
    enc->zero = enc->ones > 0;
    while (enc->ones > 1) {
      xlink_encoder_store(enc, 0);
      enc->ones--;
    }
    enc->ones = 0;
  }
  enc->bytes.length = (enc->bits + 7)/8;
}

static void xlink_encoder_write_bit(xlink_encoder *enc, xlink_word c0,
 xlink_word c1, int bit) {
  xlink_word s;
  XLINK_ERROR(c0 == 0 || c1 == 0 || (c0 > EC_MASK - c1),
   ("Error invalid counts, c0 = %i and c1 = %i", c0, c1));
  s = ((xlink_dword)enc->range)*c1/(c0 + c1);
  enc->range = bit ? s : enc->range - s;
  if (!bit) {
    /* Handle carry */
    if (s > EC_MASK - enc->low) {
      xlink_encoder_emit(enc, 1);
    }
    enc->low += s;
  }
  /* Normalize */
  while (enc->range < EC_BASE) {
    if (enc->low & EC_BASE) {
      enc->ones++;
    }
    else {
      xlink_encoder_emit(enc, 0);
    }
    enc->low <<= 1;
    enc->range <<= 1;
  }
}

void xlink_encoder_init(xlink_encoder *enc, xlink_context *ctx) {
  enc->ctx = ctx;
  xlink_list_init(&enc->bytes, sizeof(unsigned char), 0);
  enc->bits = 0;
  enc->low = 0;
  enc->range = EC_BASE;
  enc->zero = 0;
  enc->ones = 0;
  /* Start by writing a 1 bit, this is necessary for decoder implementation */
  xlink_encoder_write_bit(enc, 1, 1, 1);
  /* Update the context with 1 bit */
  xlink_context_update_bit(enc->ctx, 0, 1);
}

void xlink_encoder_clear(xlink_encoder *enc) {
  xlink_list_clear(&enc->bytes);
}

#define xlink_list_get_byte(list, i) ((unsigned char *)xlink_list_get(list, i))

void xlink_encoder_write_bytes(xlink_encoder *enc, xlink_list *bytes) {
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
      xlink_encoder_write_bit(enc, counts[0], counts[1], bit);
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

void xlink_encoder_finalize(xlink_encoder *enc, xlink_bitstream *bs) {
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
    xlink_encoder_emit(enc, 1);
  }
  for (i = 0; i < s; i++) {
    if ((enc->low + m) & (EC_BASE >> i)) {
      enc->ones++;
    }
    else {
      xlink_encoder_emit(enc, 0);
    }
  }
  /* Flush any pending bits */
  xlink_encoder_emit(enc, 0);
  /* Copy finalized bits to bitstream */
  bs->bits = enc->bits;
  bs->bytes.length = 0;
  xlink_list_append(&bs->bytes, &enc->bytes);
}

static int xlink_decoder_read_bit(xlink_decoder *dec, xlink_word c0,
 xlink_word c1) {
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

void xlink_decoder_init(xlink_decoder *dec, xlink_context *ctx,
 xlink_bitstream *bs) {
  dec->ctx = ctx;
  dec->bytes = &bs->bytes;
  dec->bits = bs->bits;
  dec->pos = 1;
  dec->low = 0;
  dec->range = 1;
  dec->value = 0;
  /* Discard first bit */
  XLINK_ERROR(xlink_decoder_read_bit(dec, 1, 1) == 0,
   ("Expected first decoded bit to be 1"));
  /* Update the context with 1 bit */
  xlink_context_update_bit(dec->ctx, 0, 1);
}

void xlink_decoder_clear(xlink_decoder *dec) {
}

unsigned char xlink_decoder_read_byte(xlink_decoder *dec) {
  unsigned char byte;
  int i;
  byte = 1;
  for (i = 8; i-- > 0; ) {
    unsigned int counts[2];
    int bit;
    xlink_context_get_counts(dec->ctx, byte, counts);
    bit = xlink_decoder_read_bit(dec, counts[0], counts[1]);
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
