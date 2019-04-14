#ifndef MX_STRING_H
#define MX_STRING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "common.h"

typedef char * mx_string_t;

/**
 * @brief Allocate and initialize a string to hold @a length characters from
 *        @a source
 *
 * If @a source is not NULL and @a length is not 0 then create a string with
 * length = @a length and copy this number of characters from @a source into it.
 *
 * If @a source is not NULL and @a length is 0 then calculate length as
 * strlen(@a source). Then create a string with this length and copy this number
 * of characters from @a source into it.
 *
 * If @a source is NULL then create a string with length = @a length and fill it
 * with this number of NUL characters.
 *
 * To create a zero length string do mx_string_create(NULL, 0);
 *
 * @return the string on success; otherwise NULL
 */
mx_string_t mx_string_create(char *source, size_t length);

/**
 * @brief Allocate and initialize a string by duplicating @a source
 *
 * Attempt to create a string with the same volume and length as @a source. If
 * this fails and the length of @a source is less than its volume then attempt
 * to create a string with the same length as @a source.
 *
 * If either of these are successful then memcpy() each character in @a source
 * into the new string.
 *
 * @return the string on success; otherwise NULL
 */
mx_string_t mx_string_duplicate(mx_string_t source);

/// Raze and deallocate the @a string
void mx_string_delete(mx_string_t string);

/// Return the volume of the @a string
size_t mx_string_volume(mx_string_t string);

/// Return the length of the @a string
size_t mx_string_length(mx_string_t string);

/**
 * @brief Resize the @a string to its @a volume
 *
 * This will fail if realloc() returns NULL. The C standard does not guarantee
 * that realloc() to a smaller size will be successful. Therefore this can fail
 * even if the requested @a volume is less than the current volume of the
 * @a string. If the realloc() fails then the @a string will be unmodified.
 *
 * If @a volume is less than the length of the @a string then the @a string will
 * be truncated and have its length reduced to @a volume.
 *
 * @return the resized string on success; otherwise NULL
 */
mx_string_t mx_string_resize(mx_string_t string, size_t volume);

/**
 * @brief Resize the @a string to its length
 *
 * If mx_string_resize() fails then the @a string will be returned unmodified.
 *
 * @return the shrunk string on success; otherwise the unmodified string
 */
mx_string_t mx_string_shrink(mx_string_t string)
  __attribute__((warn_unused_result));

/**
 * @brief Ensure that the volume of the @a string is at least @a length
 *
 * If the volume of the @a string is less than @a length then
 * mx_string_resize() will be called. Preallocation will first be attempted in
 * order to accomodate further increases in length according to the formula:
 *   volume = (length * 8 + 3) / 5
 * If this preallocation fails then a resize to @a length will be attempted. If
 * this also fails then the @a string will be unmodified.
 *
 * After a successful mx_string_ensure() subsequent mx_string_insert()s (and
 * mx_string_append()s) into the string are guaranteed to be successful as long
 * as the resultant length does not exceed @a length.
 *
 * Note that the @a string does not remember this @a length. As a result calling
 * any functions that can decrease the volume of the @a string such as:
 *   mx_string_resize()
 *   mx_string_shrink()
 *   mx_string_remove()
 * Will invalidate this guarantee.
 *
 * @return the resultant string on success; otherwise NULL
 */
mx_string_t mx_string_ensure(mx_string_t string, size_t length)
  __attribute__((warn_unused_result));

/**
 * @brief Insert @a c into the @a string at index @a i
 *
 * This will call mx_string_ensure(). If that fails then the @a string will be
 * unmodified.
 *
 * @return the resultant string on success; otherwise NULL
 */
mx_string_t mx_string_insert(mx_string_t string, size_t i, char c)
  __attribute__((warn_unused_result));

/**
 * @brief Insert @a n characters at @a c into the @a string at index @a i
 *
 * If @a n is zero then this will behave like @a n is strlen(@a c).
 *
 * This will call mx_string_ensure(). If that fails then the @a string will be
 * unmodified.
 *
 * This is more efficient than calling mx_string_insert() @a n times as the
 * characters after @a i will be shifted only once.
 *
 * @return the resultant string on success; otherwise NULL
 */
mx_string_t mx_string_inject(mx_string_t string, size_t i, char *c, size_t n)
  __attribute__((warn_unused_result));

/**
 * @brief Remove the character at index @a i from the @a string
 *
 * No matter what the character is first removed from the @a string. Then if the
 * length of the @a string is reduced such that:
 *   length <= (volume - 1) / 2
 * Then a deallocation will be attempted to reduce the volume to:
 *   volume = (length * 6 + 4) / 5
 * If this deallocation fails then the string will be returned as is.
 *
 * @return the resultant string
 */
mx_string_t mx_string_remove(mx_string_t string, size_t i)
  __attribute__((warn_unused_result));

/**
 * @brief Remove @a n characters at index @a i from the @a string
 *
 * No matter what the characters are first removed from the @a string. Then if
 * the length of the @a vector is reduced such that:
 *   length <= (volume - 1) / 2
 * Then a deallocation will be attempted to reduce the volume to:
 *   volume = (length * 6 + 4) / 5
 * If this deallocation fails then the string will be returned as is.
 *
 * @return the resultant string
 */
mx_string_t mx_string_excise(mx_string_t string, size_t i, size_t n)
  __attribute__((warn_unused_result));

/**
 * @brief Insert @a c as the last character in the @a string
 *
 * This will call mx_string_ensure(). If that fails then the @a string will be
 * unmodified.
 *
 * @return the resultant string on success; otherwise NULL
 */
mx_string_t mx_string_append(mx_string_t string, char c)
  __attribute__((warn_unused_result));

/**
 * @brief Append @a n characters from @a c to the @a string
 *
 * If @a n is zero then this will behave like @a n is strlen(@a c).
 *
 * This will call mx_string_ensure(). If that fails then the @a string will be
 * unmodified.
 *
 * @return the resultant string on success; otherwise NULL
 */
mx_string_t mx_string_extend(mx_string_t string, char *c, size_t n)
  __attribute__((warn_unused_result));

__attribute__((__format__(__printf__, 2, 3)))
mx_string_t mx_string_catf(mx_string_t string, char *format, ...);

/// Return a pointer to the last character in the @a string
char *mx_string_tail(mx_string_t string);

/// Test if @a a and @a b are equal
bool mx_string_eq(const void *a, const void *b);

/// Test if @a a and @a b differ
bool mx_string_ne(const void *a, const void *b);

bool mx_string_lt(mx_string_t a, mx_string_t b);
bool mx_string_gt(mx_string_t a, mx_string_t b);
bool mx_string_le(mx_string_t a, mx_string_t b);
bool mx_string_ge(mx_string_t a, mx_string_t b);

char *mx_string_in(mx_string_t string, char c, char *ante);

/// Return a hash of the string @a x
uint64_t mx_string_hash(const void *x);

/// Print debugging information about the @a string
void mx_string_debug(mx_string_t string);

#endif /* MX_STRING_H */
