#ifndef _XLINK_io_h
#define _XLINK_io_h

typedef struct xlink_file xlink_file;

struct xlink_file {
  const char *name;
  unsigned int size;
  unsigned int pos;
  const unsigned char *buf;
};

void xlink_file_init(xlink_file *file, const char *name);
void xlink_file_clear(xlink_file *file);

#endif
