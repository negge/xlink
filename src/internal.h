#ifndef _XLINK_internal_h
#define _XLINK_internal_h

#include <stdlib.h>

#define XLINK_MIN(a, b) ((a) <= (b) ? (a) : (b))
#define XLINK_MAX(a, b) ((a) >= (b) ? (a) : (b))

void xlink_log(const char *fmt, ...);

#define XLINK_LOG(err) xlink_log err

#define XLINK_ERROR(cond, err) \
  do { \
    if (cond) { \
      XLINK_LOG(err); \
      exit(-1); \
    } \
  } \
  while (0)

void *xlink_malloc(size_t size);
void *xlink_realloc(void *ptr, size_t size);

#define XLINK_SET_BIT(buf, i, b) ((buf)[(i) >> 3] = \
 (((buf)[(i) >> 3] & ~(1 << ((i) & 7))) | (!!(b) << ((i) & 7))))
#define XLINK_GET_BIT(buf, i)    ((buf)[(i) >> 3] >> ((i) & 7) & 1)

#define XLINK_FEOF(fp) (ungetc(getc(fp), fp) == EOF)

#endif
