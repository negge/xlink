#include <stdio.h>
#include "internal.h"
#include "io.h"

void xlink_file_init(xlink_file *file, const char *name) {
  FILE *fp;
  int size;
  fp = fopen(name, "rb");
  XLINK_ERROR(fp == NULL, ("Could not open file %s", name));
  file->name = name;
  fseek(fp, 0, SEEK_END);
  file->size = ftell(fp);
  file->buf = xlink_malloc(file->size);
  fseek(fp, 0, SEEK_SET);
  size = fread((unsigned char *)file->buf, 1, file->size, fp);
  XLINK_ERROR(size != file->size,
   ("Problem reading file %s, got %i of %i bytes", name, size, file->size));
  fclose(fp);
  file->pos = 0;
}

void xlink_file_clear(xlink_file *file) {
  free((unsigned char *)file->buf);
}
