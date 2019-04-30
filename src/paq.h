#ifndef _XLINK_paq_h
#define _XLINK_paq_h

#include "util.h"

typedef struct xlink_model xlink_model;

struct xlink_model {
  unsigned char mask;
  int weight;
  unsigned int state;
};

void xlink_model_init(xlink_model *model, unsigned char mask);
int model_comp(const void *a, const void *b);

typedef struct xlink_match xlink_match;

struct xlink_match {
  /* Used in the match_hash_code_simple() function */
  unsigned int salt;
  unsigned char partial;
  unsigned char mask;
  unsigned char buf[8];
  unsigned char counts[2];
};

int match_comp(const void *a, const void *b);
int match_equals(const void *a, const void *b);
unsigned int match_hash_code(const void *m);
unsigned int match_hash_code_fast(const void *m);
unsigned int match_hash_code_simple(const void *m);

typedef struct xlink_context xlink_context;

struct xlink_context {
  unsigned char buf[8];
  xlink_list *models;
  xlink_set matches;
  int clamp;
};

void xlink_context_init(xlink_context *ctx, xlink_list *models, int capacity,
 int fast, int clamp);
void xlink_context_clear(xlink_context *ctx);
void xlink_context_reset(xlink_context *ctx);
void xlink_context_set_fixed_capacity(xlink_context *ctx, int capacity);
void xlink_context_get_counts(xlink_context *ctx, unsigned char partial,
 unsigned int counts[2]);
void xlink_context_update_bit(xlink_context *ctx, unsigned char partial,
 int bit);

#endif
