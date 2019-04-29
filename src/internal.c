#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "internal.h"

void xlink_log(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  fprintf(stderr, "Error: ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
}

void *xlink_malloc(size_t size) {
  void *ptr;
  ptr = malloc(size);
  XLINK_ERROR(ptr == NULL, ("Insufficient memory for %i byte malloc", size));
  memset(ptr, 0, size);
  return ptr;
}

void *xlink_realloc(void *ptr, size_t size) {
  ptr = realloc(ptr, size);
  XLINK_ERROR(ptr == NULL, ("Insufficient memory for %i byte realloc", size));
  return ptr;
}
