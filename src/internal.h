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

#endif
