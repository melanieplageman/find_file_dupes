#ifndef MX_BITSET_H
#define MX_BITSET_H

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MX_BITSET_UNIT_SIZE sizeof(unsigned int)
#define MX_BITSET_UNIT_BITS (MX_BITSET_UNIT_SIZE * CHAR_BIT)

typedef unsigned int * mx_bitset_t;

/**
 * @brief Allocate and initialize a bitset to hold @a volume bits
 *
 * The @a volume of the bitset can't be changed post creation.
 *
 * This doesn't initialize the actual bits in the bitset. To initialize them
 * call either mx_bitset_zero() or mx_bitset_unzero() with the bitset.
 *
 * To create a bitset and initialize its bits use mx_bitset_create_with().
 *
 * @return the bitset on success; otherwise NULL
 */
mx_bitset_t mx_bitset_create(size_t volume);

/**
 * @brief Allocate and initialize a bitset to hold @a volume initialized bits
 *
 * The @a volume of the bitset can't be changed post creation.
 *
 * Each bit in the bitset will be reset (or set if @a x is true).
 *
 * @return the bitset on success; otherwise NULL
 */
mx_bitset_t mx_bitset_create_with(size_t volume, bool x);

/// Raze and deallocate the @a bitset
void mx_bitset_delete(mx_bitset_t bitset);

/// Return the number of units in the @a bitset
size_t mx_bitset_length(mx_bitset_t bitset);

/// Return the volume of the @a bitset
size_t mx_bitset_volume(mx_bitset_t bitset);

/// Return whether the bit in the @a bitset at index @a i is set
bool mx_bitset_get(mx_bitset_t bitset, size_t i);

#define mx_bitset_test mx_bitset_get

/// Reset the bit in the @a bitset at index @a i
void mx_bitset_reset(mx_bitset_t bitset, size_t i);

/// Set the bit in the @a bitset at index @a i
void mx_bitset_set(mx_bitset_t bitset, size_t i);

/// Reset (or set if @a x is true) the bit in the @a bitset at index @a i
void mx_bitset_assign(mx_bitset_t bitset, size_t i, bool x);

/// Toggle the bit in the @a bitset at index @a i
void mx_bitset_toggle(mx_bitset_t bitset, size_t i);

/// Reset all bits in the @a bitset
void mx_bitset_zero(mx_bitset_t bitset);

/// Set all bits in the @a bitset
void mx_bitset_unzero(mx_bitset_t bitset);

/// Toggle all bits in the @a bitset
void mx_bitset_invert(mx_bitset_t bitset);

#define mx_bitset_not mx_bitset_invert

/// Reset (or set if @a x is true) extraneous bits in the @a bitset
void mx_bitset_sanitize(mx_bitset_t bitset, bool x);

/**
 * @brief Calculate the bitwise AND of the bits in @a with the bits in @a b
 *
 * The result will be stored in @a a. If @a a and @a b don't have the same
 * volume then the shorter of the two will be used.
 */
void mx_bitset_and(mx_bitset_t a, mx_bitset_t b);

void mx_bitset_or(mx_bitset_t a, mx_bitset_t b);

void mx_bitset_xor(mx_bitset_t a, mx_bitset_t b);

/// Return whether all bits are set in the @a bitset
bool mx_bitset_all(mx_bitset_t bitset);

/// Return whether any bits are set in the @a bitset
bool mx_bitset_any(mx_bitset_t bitset);

/// Return whether no bits are set in the @a bitset
bool mx_bitset_none(mx_bitset_t bitset);

/// Return the number of bits set in the @a bitset
size_t mx_bitset_popcnt(mx_bitset_t bitset);

/// Return the index of the next bit set in the @a bitset (inclusive of @a i)
size_t mx_bitset_next(mx_bitset_t bitset, size_t i);

void mx_bitset_debug(mx_bitset_t bitset);

#endif /* MX_BITSET_H */
