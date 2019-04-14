#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "extent.h"

typedef struct _header_t {
  size_t length;
  char data[];
} header_t;

mx_extent_t header_to_extent(header_t *header) {
  return (mx_extent_t) header->data;
}

header_t *extent_to_header(mx_extent_t extent) {
  return (header_t *) (extent - offsetof(header_t, data));
}

mx_extent_t mx_extent_create() {
  header_t *header;

  if ((header = malloc(sizeof(header_t))) == NULL)
    return NULL;

  header->length = 0;

  return header_to_extent(header);
}

mx_extent_t mx_extent_duplicate(mx_extent_t source) {
  header_t *header;

  size_t element_size = mx_extent_element_size(source);
  size_t volume = mx_extent_volume(source);
  size_t length = mx_extent_length(source);

  if ((header = malloc(sizeof(header_t) + volume * element_size)) == NULL) {
    if (length == volume)
      return NULL;
    if ((header = malloc(sizeof(header_t) + length * element_size)) == NULL)
      return NULL;
    header->volume = length;
  } else
    header->volume = volume;

  header->element_size = element_size;
  header->length = length;

  memcpy(header->data, source, length * element_size);

  return header_to_extent(header);
}

void mx_extent_delete(mx_extent_t extent) {
  free(extent_to_header(extent));
}

size_t mx_extent_length(mx_extent_t extent) {
  return extent_to_header(extent)->length;
}

void *_mx_extent_at(mx_extent_t extent, size_t element_size, size_t i) {
  return (char *) extent + i * element_size;
}

size_t mx_extent_index(mx_extent_t extent, void *elmt) {
  return ((char *) elmt - (char *) extent) / mx_extent_element_size(extent);
}

void mx_extent_get(mx_extent_t extent, size_t i, void *elmt) {
  memcpy(elmt, mx_extent_at(extent, i), mx_extent_element_size(extent));
}

void mx_extent_set(mx_extent_t extent, size_t i, void *elmt) {
  memcpy(mx_extent_at(extent, i), elmt, mx_extent_element_size(extent));
}

void mx_extent_swap(mx_extent_t extent, size_t i, size_t j) {
  char *a = mx_extent_at(extent, i);
  char *b = mx_extent_at(extent, j);
  size_t element_size = mx_extent_element_size(extent);

  for (size_t k = 0; k < element_size; k++) {
    char buffer;
    buffer = a[k];
    a[k] = b[k];
    b[k] = buffer;
  }
}

void mx_extent_move(mx_extent_t extent, size_t target, size_t source) {
  if (target == source)
    return;

  if (target < source) {
    while (source-- > target)
      mx_extent_swap(extent, source, source + 1);
  } else {
    for (; source < target; source++)
      mx_extent_swap(extent, source, source + 1);
  }
}

mx_extent_t mx_extent_resize(mx_extent_t extent, size_t volume) {
  header_t *header = extent_to_header(extent);
  size_t size;

  // calculate size and test for overflow
  if (mx_mulz_overflow(volume, header->element_size, &size))
    return NULL;
  if (mx_addz_overflow(size, sizeof(header_t), &size))
    return NULL;

  if ((header = realloc(header, size)) == NULL)
    return NULL;

  header->volume = volume;
  header->length = MX_MINIMUM(header->length, volume);

  return header_to_extent(header);
}

mx_extent_t mx_extent_shrink(mx_extent_t extent) {
  mx_extent_t shrunk;

  shrunk = mx_extent_resize(extent, mx_extent_length(extent));

  return shrunk != NULL ? shrunk : extent;
}

mx_extent_t mx_extent_ensure(mx_extent_t extent, size_t length) {
  if (length > mx_extent_volume(extent)) {
    // just volume = (length * 8 + 3) / 5 avoiding intermediate overflow
    size_t volume = length / 5 * 8 + ((length % 5) * 8 + 3) / 5;

    // if the volume doesn't overflow then attempt to allocate it
    if (volume > length) {
      mx_extent_t resize;
      if ((resize = mx_extent_resize(extent, volume)) != NULL)
        return resize;
    }

    // if either the volume overflows or the allocation failed then attempt to
    // resize to just the length
    return mx_extent_resize(extent, length);
  } else return extent;
}

mx_extent_t mx_extent_insert(mx_extent_t extent, size_t i, void *elmt) {
  return mx_extent_inject(extent, i, elmt, 1);
}

mx_extent_t
mx_extent_inject(mx_extent_t extent, size_t i, void *elmt, size_t n) {
  size_t length = mx_extent_length(extent);

  if (mx_addz_overflow(length, n, &length))
    return NULL;

  if ((extent = mx_extent_ensure(extent, length)) == NULL)
    return NULL;

  // move the existing elements n elements toward the tail
  void *target = mx_extent_at(extent, i + n);
  void *source = mx_extent_at(extent, i + 0);
  size_t size = (mx_extent_length(extent) - i) * mx_extent_element_size(extent);
  memmove(target, source, size);

  if (elmt != NULL)
    memcpy(mx_extent_at(extent, i), elmt, n * mx_extent_element_size(extent));

  // increase the length
  extent_to_header(extent)->length = length;

  return extent;
}

mx_extent_t mx_extent_remove(mx_extent_t extent, size_t i) {
  return mx_extent_excise(extent, i, 1);
}

mx_extent_t mx_extent_excise(mx_extent_t extent, size_t i, size_t n) {
  size_t length = mx_extent_length(extent) - n;

  // move the existing elements n elements toward the head
  void *target = mx_extent_at(extent, i + 0);
  void *source = mx_extent_at(extent, i + n);
  size_t size = (length - i) * mx_extent_element_size(extent);
  memmove(target, source, size);

  if (length <= (mx_extent_volume(extent) - 1) / 2) {
    mx_extent_t resize;
    // just volume = (length * 6 + 4) / 5 avoiding intermediate overflow
    size_t volume = length / 5 * 6 + ((length % 5) * 6 + 4) / 5;

    if ((resize = mx_extent_resize(extent, volume)) != NULL)
      extent = resize;
  }

  // decrease the length
  extent_to_header(extent)->length = length;

  return extent;
}

mx_extent_t mx_extent_truncate(mx_extent_t extent, size_t length) {
  size_t n = mx_extent_length(extent) - length;
  return mx_extent_excise(extent, mx_extent_length(extent) - n, n);
}

mx_extent_t mx_extent_append(mx_extent_t extent, void *elmt) {
  return mx_extent_insert(extent, mx_extent_length(extent), elmt);
}

mx_extent_t mx_extent_extend(mx_extent_t extent, void *elmt, size_t n) {
  return mx_extent_inject(extent, mx_extent_length(extent), elmt, n);
}

void *mx_extent_tail(mx_extent_t extent) {
  return mx_extent_at(extent, mx_extent_length(extent) - 1);
}

mx_extent_t mx_extent_pull(mx_extent_t extent, void *elmt) {
  if (elmt != NULL)
    mx_extent_get(extent, mx_extent_length(extent) - 1, elmt);
  return mx_extent_remove(extent, mx_extent_length(extent) - 1);
}

mx_extent_t mx_extent_shift(mx_extent_t extent, void *elmt) {
  if (elmt != NULL)
    mx_extent_get(extent, 0, elmt);
  return mx_extent_remove(extent, 0);
}

bool mx_extent_eq(mx_extent_t a, mx_extent_t b, mx_eq_f eqf) {
  if (mx_extent_length(a) != mx_extent_length(b))
    return false;

  size_t length = mx_extent_length(a);

  // compare using memcpy if no equality function is available
  if (eqf == NULL) {
    if (mx_extent_element_size(a) != mx_extent_element_size(b))
      return false;
    size_t element_size = mx_extent_element_size(a);
    for (size_t i = 0; i < length; i++) {
      if (memcmp(mx_extent_at(a, i), mx_extent_at(b, i), element_size) != 0)
        return false;
    }
  } else {
    for (size_t i = 0; i < length; i++) {
      if (eqf(mx_extent_at(a, i), mx_extent_at(b, i)) == false)
        return false;
    }
  }

  return true;
}

bool mx_extent_ne(mx_extent_t a, mx_extent_t b, mx_eq_f eqf) {
  return !mx_extent_eq(a, b, eqf);
}

void mx_extent_sort(mx_extent_t extent, mx_cmp_f cmpf) {
  qsort(extent, mx_extent_length(extent), mx_extent_element_size(extent), cmpf);
}

void *mx_extent_in(mx_extent_t extent, void *elmt, mx_eq_f eqf, void *ante) {
  void *tail = mx_extent_tail(extent);
  ante = ante == NULL ? extent : (char *) ante + mx_extent_element_size(extent);

  while (ante <= tail) {
    if (eqf(ante, elmt))
      return ante;
    ante = (char *) ante + mx_extent_element_size(extent);
  }

  return NULL;
}

size_t mx_extent_find(mx_extent_t extent, mx_eq_f eqf, void *data) {
  return mx_extent_find_next(extent, 0, eqf, data);
}

size_t
mx_extent_find_next(mx_extent_t extent, size_t i, mx_eq_f eqf, void *data) {
  for (; i < mx_extent_length(extent); i++) {
    if (eqf(mx_extent_at(extent, i), data))
      return i;
  }
  return MX_ABSENT;
}

size_t
mx_extent_find_last(mx_extent_t extent, size_t i, mx_eq_f eqf, void *data) {
  for (; i-- > 0;) {
    if (eqf(mx_extent_at(extent, i), data))
      return i;
  }
  return MX_ABSENT;
}

void *mx_extent_search(mx_extent_t extent, void *elmt, mx_cmp_f cmpf) {
  size_t element_size = mx_extent_element_size(extent);
  return bsearch(elmt, extent, mx_extent_length(extent), element_size, cmpf);
}

void mx_extent_debug(mx_extent_t extent, void (*elmt_debug)(void *)) {
  header_t *header = extent_to_header(extent);
  fprintf(stderr,
    "mx_extent_t(data = %p, element_size = %zu, utilization = %zu/%zu)",
  header->data, header->element_size, header->length, header->volume);

  if (elmt_debug != NULL) {
    fprintf(stderr, " [ ");
    for (size_t i = 0; i < header->length; i++) {
      if (i > 0)
        fprintf(stderr, ", ");
      elmt_debug(mx_extent_at(extent, i));
    }
    fprintf(stderr, " ]");
  }
}
