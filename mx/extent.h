#ifndef MX_EXTENT_H
#define MX_EXTENT_H

#include <stdbool.h>
#include <stddef.h>

#include "common.h"

typedef void * mx_extent_t;

/// Allocate and initialize a extent for elements of size @a element_size
mx_extent_t mx_extent_create(size_t length);

/**
 * @brief Allocate and initialize a extent by duplicating @a source
 *
 * Attempt to create a extent with the same element_size, volume, and length as
 * @source. If this fails and the length of @source is less than its volume
 * then attempt to create a extent with the same element_size and length as
 * @source.
 *
 * If either of these are successful then memcpy() each element in @a source
 * into the new extent.
 *
 * @return the extent on success; otherwise NULL
 */
#define mx_extent_duplicate(__e) \
  _mx_extent_duplicate((__e), sizeof(*(__e)))
mx_extent_t _mx_extent_duplicate(mx_extent_t source, size_t element_size);

/// Raze and deallocate the @a extent
void mx_extent_delete(mx_extent_t extent);

/// Return the length of the @a extent
size_t mx_extent_length(mx_extent_t extent);

/// Return a pointer to the element in the @a extent at index @a i
#define mx_extent_at(__e, __i) \
  _mx_extent_at((__e), sizeof(*(__e)), (__i))
void *_mx_extent_at(mx_extent_t extent, size_t element_size, size_t i);

/**
 * @brief Return the index of the element at @a elmt in the @a extent
 *
 * This does not compare @a elmt against the elements in the @a extent. @a elmt
 * must already be a pointer to an element in the @a extent.
 *
 * This is _almost_ the inverse of mx_extent_at(). Because of integer division:
 *   mx_extent_at(@a extent, mx_extent_index(@a elmt)) != @a elmt
 * If @a elmt is a pointer to the middle of an element in the @a extent.
 */
#define mx_extent_index(__e, __x) \
  _mx_extent_index((__e), sizeof(*(__e)), (__x))
size_t _mx_extent_index(mx_extent_t extent, size_t element_size, void *elmt);

/// Copy the element at index @a i in the @a extent into @a elmt
#define mx_extent_get(__e, __i, __x) \
  _mx_extent_get((__e), sizeof(*(__e)), (__i), (__x))
void _mx_extent_get(mx_extent_t extent, size_t element_size, size_t i, void *elmt);

/// Copy the data at @a elmt into the @a extent at index @a i
#define mx_extent_set(__e, __i, __x) \
  _mx_extent_set((__e), sizeof(*(__e)), (__i), (__x))
void mx_extent_set(mx_extent_t extent, size_t element_size, size_t i, void *elmt);

/// Swap the data at index @a i with the data at index @a j in the @a extent
void mx_extent_swap(mx_extent_t extent, size_t i, size_t j);

/// Move the data at index @a source to index @a target in the @a extent
void mx_extent_move(mx_extent_t extent, size_t target, size_t source);

/// Return a pointer to the last element in the @a extent
void *mx_extent_tail(mx_extent_t extent);

/// Test if the elements in @a a and @a b are equal according to @a eqf
bool mx_extent_eq(mx_extent_t a, mx_extent_t b, mx_eq_f eqf);

/// Test if the elements in @a a and @a b differ according to @a eqf
bool mx_extent_ne(mx_extent_t a, mx_extent_t b, mx_eq_f eqf);

/// Sort the @a extent according to @a cpmf
void mx_extent_sort(mx_extent_t extent, mx_cmp_f cmpf);

void *mx_extent_in(mx_extent_t extent, void *elmt, mx_eq_f eqf, void *ante);

/// Find the first element in the @a extent for which @a eqf returns true
size_t mx_extent_find(mx_extent_t extent, mx_eq_f eqf, void *data);

size_t
mx_extent_find_next(mx_extent_t extent, size_t i, mx_eq_f eqf, void *data);

size_t
mx_extent_find_last(mx_extent_t extent, size_t i, mx_eq_f eqf, void *data);

void *mx_extent_search(mx_extent_t extent, void *elmt, mx_cmp_f cmpf);

/**
 * @brief Print debugging information about the @a extent
 *
 * If @a elmt_debug is not NULL then it will be used to print debugging
 * information about each element in the @a extent.
 */
void mx_extent_debug(mx_extent_t extent, void (*elmt_debug)(void *));

#endif /* MX_EXTENT_H */
