#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "bitset.h"

typedef struct _header_t {
  size_t volume;
  unsigned int data[];
} header_t;

/// Return the number of units in a bitset with @a volume bits
static size_t volume_to_length(size_t volume) {
  // Just size = volume / MX_BITSET_UNIT_BITS with integral ceiling division
  // avoiding intermeddiate overflow
  size_t length = volume / MX_BITSET_UNIT_BITS;
  length += (volume % MX_BITSET_UNIT_BITS > 0);
  return length;
}

static mx_bitset_t header_to_bitset(header_t *header) {
  return (mx_bitset_t) header->data;
}

static header_t *bitset_to_header(mx_bitset_t bitset) {
  return (header_t *) ((char *) bitset - offsetof(header_t, data));
}

mx_bitset_t mx_bitset_create(size_t volume) {
  header_t *header;
  size_t length = volume_to_length(volume);
  size_t size;
  
  // Calculate size and check for overflow
  if (mx_mulz_overflow(length, MX_BITSET_UNIT_SIZE, &size))
    return NULL;
  if (mx_addz_overflow(size, sizeof(header_t), &size))
    return NULL;

  if ((header = malloc(size)) == NULL)
    return NULL;

  header->volume = volume;

  return header_to_bitset(header);
}

mx_bitset_t mx_bitset_create_with(size_t volume, bool x) {
  mx_bitset_t bitset;

  if ((bitset = mx_bitset_create(volume)) == NULL)
    return NULL;
  x ? mx_bitset_unzero(bitset) : mx_bitset_zero(bitset);

  return bitset;
}

void mx_bitset_delete(mx_bitset_t bitset) {
  free(bitset_to_header(bitset));
}

size_t mx_bitset_length(mx_bitset_t bitset) {
  return volume_to_length(mx_bitset_volume(bitset));
}

size_t mx_bitset_volume(mx_bitset_t bitset) {
  return bitset_to_header(bitset)->volume;
}

bool mx_bitset_get(mx_bitset_t bitset, size_t i) {
  size_t unit_i = i / MX_BITSET_UNIT_BITS;
  size_t offset = i % MX_BITSET_UNIT_BITS;
  return (bitset[unit_i] >> offset) & 1U;
}

void mx_bitset_reset(mx_bitset_t bitset, size_t i) {
  size_t unit_i = i / MX_BITSET_UNIT_BITS;
  size_t offset = i % MX_BITSET_UNIT_BITS;
  bitset[unit_i] &= ~(1U << offset);
}

void mx_bitset_set(mx_bitset_t bitset, size_t i) {
  size_t unit_i = i / MX_BITSET_UNIT_BITS;
  size_t offset = i % MX_BITSET_UNIT_BITS;
  bitset[unit_i] |= 1U << offset;
}

void mx_bitset_assign(mx_bitset_t bitset, size_t i, bool x) {
  x ? mx_bitset_set(bitset, i) : mx_bitset_reset(bitset, i);
}

void mx_bitset_toggle(mx_bitset_t bitset, size_t i) {
  size_t unit_i = i / MX_BITSET_UNIT_BITS;
  size_t offset = i % MX_BITSET_UNIT_BITS;
  bitset[unit_i] ^= 1U << offset;
}

void mx_bitset_zero(mx_bitset_t bitset) {
  unsigned int number = 0U;
  memset(bitset, number, mx_bitset_length(bitset) * MX_BITSET_UNIT_SIZE);
}

void mx_bitset_unzero(mx_bitset_t bitset) {
  unsigned int number = ~0U;
  memset(bitset, number, mx_bitset_length(bitset) * MX_BITSET_UNIT_SIZE);
}

void mx_bitset_invert(mx_bitset_t bitset) {
  for (size_t i = 0; i < mx_bitset_length(bitset); i++)
    bitset[i] = ~bitset[i];
}

void mx_bitset_sanitize(mx_bitset_t bitset, bool x) {
  // The number of used bits in the last unit of the bitset
  size_t offset = mx_bitset_volume(bitset) % MX_BITSET_UNIT_BITS;

  // Return if the volume of the bitset is zero or if the volume is divisible by
  // MX_BITSET_UNIT_BITS
  if (offset == 0)
    return;

  // Here the volume of the bitset must be indivisible by MX_BITSET_UNIT_BITS so
  // we don't need to adjust this calculation
  size_t tail_i = mx_bitset_volume(bitset) / MX_BITSET_UNIT_BITS;

  unsigned int mask = (1U << offset) - 1;

  bitset[tail_i] = x ? bitset[tail_i] | ~mask : bitset[tail_i] & mask;
}

void mx_bitset_and(mx_bitset_t a, mx_bitset_t b) {
  size_t a_length = mx_bitset_length(a);
  size_t b_length = mx_bitset_length(b);
  size_t length = MX_MINIMUM(a_length, b_length);

  if (b_length <= a_length)
    mx_bitset_sanitize(b, 1);

  for (size_t i = 0; i < length; i++)
    a[i] &= b[i];
}

void mx_bitset_or(mx_bitset_t a, mx_bitset_t b) {
  size_t a_length = mx_bitset_length(a);
  size_t b_length = mx_bitset_length(b);
  size_t length = MX_MINIMUM(a_length, b_length);

  if (b_length <= a_length)
    mx_bitset_sanitize(b, 0);

  for (size_t i = 0; i < length; i++)
    a[i] |= b[i];
}

void mx_bitset_xor(mx_bitset_t a, mx_bitset_t b) {
  size_t a_length = mx_bitset_length(a);
  size_t b_length = mx_bitset_length(b);
  size_t length = MX_MINIMUM(a_length, b_length);

  if (b_length <= a_length)
    mx_bitset_sanitize(b, 0);

  for (size_t i = 0; i < length; i++)
    a[i] ^= b[i];
}

bool mx_bitset_all(mx_bitset_t bitset) {
  mx_bitset_sanitize(bitset, 1);

  for (size_t i = 0; i < mx_bitset_length(bitset); i++) {
    if (bitset[i] != ~0U)
      return false;
  }

  return true;
}

bool mx_bitset_any(mx_bitset_t bitset) {
  mx_bitset_sanitize(bitset, 0);

  for (size_t i = 0; i < mx_bitset_length(bitset); i++) {
    if (bitset[i] != 0U)
      return true;
  }

  return false;
}

bool mx_bitset_none(mx_bitset_t bitset) {
  mx_bitset_sanitize(bitset, 0);

  for (size_t i = 0; i < mx_bitset_length(bitset); i++) {
    if (bitset[i] != 0U)
      return false;
  }

  return true;
}

size_t mx_bitset_popcnt(mx_bitset_t bitset) {
  size_t result = 0;

  mx_bitset_sanitize(bitset, 0);

  for (size_t i = 0; i < mx_bitset_length(bitset); i++)
    result += __builtin_popcount(bitset[i]);

  return result;
}

size_t mx_bitset_next(mx_bitset_t bitset, size_t i) {
  size_t unit_i = i / MX_BITSET_UNIT_BITS;
  size_t offset = i % MX_BITSET_UNIT_BITS;
  size_t result;

  if (i >= mx_bitset_volume(bitset))
    return MX_ABSENT;

  mx_bitset_sanitize(bitset, 0);

  if ((result = __builtin_ffs(bitset[unit_i] >> offset)) != 0)
    return unit_i * MX_BITSET_UNIT_BITS + offset + result - 1;

  for (unit_i++; unit_i < mx_bitset_length(bitset); unit_i++) {
    if ((result = __builtin_ffs(bitset[unit_i])) != 0)
      return unit_i * MX_BITSET_UNIT_BITS + result - 1;
  }

  return MX_ABSENT;
}

void mx_bitset_debug(mx_bitset_t bitset) {
  fprintf(stderr, "mx_bitset_t(%p, [", bitset);
  for (size_t i = 0; i < mx_bitset_volume(bitset); i++)
    fprintf(stderr, "%u", mx_bitset_get(bitset, i));
  fprintf(stderr, "])");
}
