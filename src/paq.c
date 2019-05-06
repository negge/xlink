#include <stdlib.h>
#include <string.h>
#include "internal.h"
#include "paq.h"

void xlink_model_init(xlink_model *model, unsigned char mask) {
  int i;
  model->mask = mask;
  /* For now just compute the weight as the number of bits set in mask */
  model->weight = 0;
  for (i = 0; i < 8; i++) {
    model->weight += !!(mask & (1 << i));
  }
}

unsigned int xlink_model_compute_packed_weights(xlink_list *models) {
  unsigned int packed;
  int i, j;
  int last;
  packed = 0xffffffffu;
  i = 0;
  last = 0;
  for (j = 0; j < xlink_list_length(models); j++, i++) {
    xlink_model *model;
    model = xlink_list_get(models, j);
    i += model->weight - last;
    XLINK_ERROR(i > 31, ("Invalid list of models, i = %i", i));
    packed &= ~(1 << (31 - i));
    last = model->weight;
  }
  return packed;
}

/* Compute the weight state for each model in order */
void xlink_model_set_state(xlink_list *models, unsigned int state) {
  int weight;
  int i;
  weight = 0;
  for (i = 0; state != 0; i++) {
    int test;
    xlink_model *model;
    weight--;
    do {
      weight++;
      test = state >= 0x80000000u;
      state += state;
    }
    while (test);
    if (state == 0) break;
    model = xlink_list_get(models, i);
    XLINK_ERROR(model->weight != weight,
     ("Mismatch when decoding model %02x weight: got %i expected %i",
     model->mask, weight, model->weight));
    model->state = state;
  }
}

int model_comp(const void *a, const void *b) {
  const xlink_model *mod_a;
  const xlink_model *mod_b;
  int mask_a;
  int mask_b;
  int i;
  mod_a = (xlink_model *)a;
  mod_b = (xlink_model *)b;
  /* Compute the bit-reversal of the mask */
  mask_a = mask_b = 0;
  for (i = 0; i < 8; i++) {
    mask_a <<= 1;
    mask_b <<= 1;
    mask_a |= !!(mod_a->mask & (1 << i));
    mask_b |= !!(mod_b->mask & (1 << i));
  }
  /* Sort on weights ascending, then bit-reversed mask */
  if (mod_a->weight == mod_b->weight) {
    return mask_a - mask_b;
  }
  return mod_a->weight - mod_b->weight;
}

int match_comp(const void *a, const void *b) {
  const xlink_match *mat_a;
  const xlink_match *mat_b;
  mat_a = (xlink_match *)a;
  mat_b = (xlink_match *)b;
  int i;
  if (mat_a->mask == mat_b->mask) {
    if (mat_a->partial == mat_b->partial) {
      for (i = 0; i < 8; i++) {
        if (mat_a->mask & (1 << (7 - i))) {
          if (mat_a->buf[i] == mat_b->buf[i]) {
            continue;
          }
          return mat_a->buf[i] - mat_b->buf[i];
        }
      }
      return 0;
    }
    return mat_a->partial - mat_b->partial;
  }
  return mat_a->mask - mat_b->mask;
}

int match_equals(const void *a, const void *b) {
  return match_comp(a, b) == 0;
}

unsigned int match_hash_code(const void *m) {
  const xlink_match *mat;
  unsigned int hash;
  int i;
  mat = (xlink_match *)m;
  hash = 0x0;
  /* Combine the mask */
  hash += mat->mask;
  hash *= 0x6f;
  hash ^= mat->mask;
  /* Combine the partial */
  hash += mat->partial;
  hash *= 0x6f;
  hash ^= mat->partial;
  /* Combine the history */
  for (i = 0; i < 8; i++) {
    if (mat->mask & (1 << (7 - i))) {
      hash += mat->buf[i];
      hash *= 0x6f;
      hash ^= mat->buf[i];
    }
  }
  /* Extra hashing for good measure */
  hash ^= (hash >> 20) ^ (hash >> 12);
  return hash ^ (hash >> 7) ^ (hash >> 4);
}

unsigned int match_hash_code_simple(const void *m) {
  const xlink_match *mat;
  unsigned int hash;
  unsigned char byte;
  int i;
  mat = (xlink_match *)m;
  /* Use current weight state to salt the hash */
  hash = mat->salt;
  /* Combine the mask */
  hash = (hash & 0xffffff00) | mat->mask;
  /* Combine the partial */
  byte = (hash & 0x000000ff);
  hash = (hash & 0xffffff00) | (byte ^ mat->partial);
  hash = ((int)hash)*0x6f;
  byte = (hash & 0x000000ff);
  hash = (hash & 0xffffff00) | ((byte + mat->partial) & 0x000000ff);
  hash--;
  /* Combine the history */
  for (i = 0; i < 8; i++) {
    if (mat->mask & (1 << (7 - i))) {
      byte = (hash & 0x000000ff);
      hash = (hash & 0xffffff00) | (byte ^ mat->buf[i]);
      hash = ((int)hash)*0x6f;
      byte = (hash & 0x000000ff);
      hash = (hash & 0xffffff00) | ((byte + mat->buf[i]) & 0x000000ff);
      hash--;
    }
  }
  return hash;
}

static unsigned int xlink_rotate_left(unsigned int val, int bits) {
  return (val << bits) | (val >> (32 - bits));
}

unsigned int match_hash_code_fast(const void *m) {
  const xlink_match *mat;
  unsigned int hash;
  unsigned char byte;
  int i;
  mat = (xlink_match *)m;
  /* Use current weight state to salt the hash */
  hash = mat->salt;
  /* Combine the mask */
  hash = (hash & 0xffffff00) | mat->mask;
  /* Combine the partial */
  byte = (hash & 0x000000ff);
  hash = (hash & 0xffffff00) | (byte ^ mat->partial);
  hash = xlink_rotate_left(hash, 9);
  byte = (hash & 0x000000ff);
  hash = (hash & 0xffffff00) | ((byte + mat->partial) & 0x000000ff);
  hash--;
  /* Combine the history */
  for (i = 0; i < 8; i++) {
    if (mat->mask & (1 << (7 - i))) {
      byte = (hash & 0x000000ff);
      hash = (hash & 0xffffff00) | (byte ^ mat->buf[i]);
      hash = xlink_rotate_left(hash, 9);
      byte = (hash & 0x000000ff);
      hash = (hash & 0xffffff00) | ((byte + mat->buf[i]) & 0x000000ff);
      hash--;
    }
  }
  return hash;
}

void xlink_context_init(xlink_context *ctx, xlink_list *models, int capacity,
 int fast, int clamp) {
  ctx->models = models;
  xlink_set_init(&ctx->matches, match_hash_code_simple, match_equals,
   sizeof(xlink_match), 0, 0.75);
  if (capacity > 0) {
    xlink_context_set_fixed_capacity(ctx, capacity);
  }
  if (fast) {
    ctx->matches.hash_code = match_hash_code_fast;
  }
  xlink_context_reset(ctx);
  ctx->clamp = clamp;
}

void xlink_context_clear(xlink_context *ctx) {
  xlink_set_clear(&ctx->matches);
}

void xlink_context_reset(xlink_context *ctx) {
  memset(ctx->buf, 0, sizeof(ctx->buf));
  xlink_set_reset(&ctx->matches);
}

void xlink_context_set_fixed_capacity(xlink_context *ctx, int capacity) {
  ctx->matches.equals = NULL;
  ctx->matches.load = 1.f;
  xlink_set_reset(&ctx->matches);
  xlink_set_resize(&ctx->matches, capacity);
}

void xlink_context_get_counts(xlink_context *ctx, unsigned char partial,
 unsigned int counts[2]) {
  xlink_match key;
  int i;
  memcpy(key.buf, ctx->buf, sizeof(ctx->buf));
  key.partial = partial;
  counts[0] = counts[1] = 2;
  for (i = 0; i < xlink_list_length(ctx->models); i++) {
    xlink_model *model;
    xlink_match *match;
    model = xlink_list_get(ctx->models, i);
    key.mask = model->mask;
    key.salt = model->state;
    match = xlink_set_get(&ctx->matches, &key);
    if (match != NULL) {
      counts[0] += ((unsigned int)match->counts[0]) << model->weight;
      counts[1] += ((unsigned int)match->counts[1]) << model->weight;
    }
  }
}

void xlink_context_update_bit(xlink_context *ctx, unsigned char partial,
 int bit) {
  xlink_match key;
  int i;
  key.partial = partial;
  memcpy(key.buf, ctx->buf, sizeof(ctx->buf));
  for (i = 0; i < xlink_list_length(ctx->models); i++) {
    xlink_model *model;
    xlink_match *match;
    model = xlink_list_get(ctx->models, i);
    key.mask = model->mask;
    key.salt = model->state;
    match = xlink_set_get(&ctx->matches, &key);
    if (match == NULL) {
      memset(key.counts, 0, sizeof(key.counts));
      match = xlink_set_put(&ctx->matches, &key);
    }
    if (ctx->clamp) {
      match->counts[bit] = XLINK_MIN(255, match->counts[bit] + 1);
    }
    else {
      match->counts[bit] = match->counts[bit] + 1;
    }
    if (match->counts[1 - bit] > 1) {
      match->counts[1 - bit] >>= 1;
    }
  }
}
