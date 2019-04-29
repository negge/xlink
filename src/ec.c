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
