#ifndef _XLINK_ec_h
#define _XLINK_ec_h

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

#endif
