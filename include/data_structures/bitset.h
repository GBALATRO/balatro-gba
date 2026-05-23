/**
 * @file bitset.h
 *
 * @brief A bitset for operating on flags
 */
#ifndef BITSET_H
#define BITSET_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @def BITSET_BITS_PER_WORD
 * @brief Number of bits in a word for a bitset.
 *
 * Number of bits in a word for a bitset. Will always be 32 here.
 */
#define BITSET_BITS_PER_WORD 32

/**
 * @def BITSET_ARRAY_SIZE
 * @brief Number of words in a bitset
 *
 * Number of words in every bitset. This represents the maximum number and each
 * bitset will always use this number of words, though it's capacity can be any length
 * from `1` to `BITSET_BITS_PER_WORD * BITSET_ARRAY_SIZE`
 */
#define BITSET_ARRAY_SIZE 8

/**
 * @def BITSET_MAX_BITS
 * @brief Maximum number of bits in a bitset
 */
#define BITSET_MAX_BITS (BITSET_BITS_PER_WORD * BITSET_ARRAY_SIZE)

/**
 * @brief A bitset spread across multiple `uint32_t` words
 */
typedef struct Bitset
{
    /**
     * @brief Word array of `uint32_t` to hold the bitset data
     */
    uint32_t* w;

    /**
     * @brief Number of bits in a word, will be 32
     */
    uint32_t nbits;

    /**
     * @brief Number of words int the `w` array
     */
    uint32_t nwords;

    /**
     * @brief Number of actual flags (nbits * nwords)
     */
    uint32_t cap;
} Bitset;

/**
 * @brief An iterator into a @ref Bitset
 *
 * This iterator will parse and find the next index to a '1' bit as efficiently as possible.
 *
 * There is no implementation of the following (yet):
 *  - Reverse iteration
 *  - Bit-by-bit iteration
 *  - Iterating on offsets to '0' bits
 */
typedef struct
{
    /**
     * @brief @ref Bitset this is iterating through
     */
    const Bitset* bitset;

    /**
     * @brief Current word the iterator is on
     */
    int word;

    /**
     * @brief Current bit the iterator is on
     */
    int bit;

    /**
     * @brief Number of bits that have been iterated through in total
     */
    int itr;
} BitsetItr;

/**
 * @brief Set a flag in a bitset to a value
 *
 * @param bitset A @ref Bitset to operate on
 * @param idx the index of the flag to set
 * @param on the value to set the flag to
 */
void bitset_set_idx(Bitset* bitset, int idx, bool on);

/**
 * @brief Get the value of a flag
 *
 * @param bitset A @ref Bitset to operate on
 * @param idx the index of the flag to get
 *
 * @return the value of the flag as `true` or `false`
 */
bool bitset_get_idx(Bitset* bitset, int idx);

/**
 * @brief Set the next free index in the bitset and return the index value
 *
 * @param bitset A @ref Bitset to operate on
 *
 * @return The index of the bit that was set
 */
int bitset_set_next_free_idx(Bitset* bitset);

/**
 * @brief Clear the bitset, all to 0
 *
 * @param bitset A @ref Bitset to operate on
 */
void bitset_clear(Bitset* bitset);

/**
 * @brief Check if a bitset is empty (all 0's)
 *
 * @param bitset A @ref Bitset to operate on
 *
 * @return `true` if empty, `false` otherwise
 */
bool bitset_is_empty(Bitset* bitset);

/**
 * @brief Count how many bits are set to `1` in a bitset
 *
 * @param bitset A @ref Bitset to operate on
 *
 * @return The number of flags set to `1` in a bitset
 */
int bitset_num_set_bits(Bitset* bitset);

/**
 * @brief Find the index of the nth set bit
 *
 * Find the index of the nth flag set to `1`. This function is useful to get one value quickly,
 * but does not operate iteratively well. Use a @BitsetItr for iterative access to a bitset.
 *
 * @param bitset A @ref Bitset to operate on
 *
 * @return The index of the nth flag set to `1` in the bitset
 */
int bitset_find_idx_of_nth_set(const Bitset* bitset, int n);

/**
 * @brief Declare a @ref BitsetItr
 *
 * @param bitset A @ref Bitset to operate on
 *
 * @return A newly constructed BitsetItr
 */
BitsetItr bitset_itr_create(const Bitset* bitset);

/**
 * @brief Get the index of the next set bit in the bitset from a @ref BitsetItr
 *
 * @param itr A @ref BitsetItr to operate on
 *
 * @return a positive number if successful, UNDEFINED otherwise (out-of-bounds)
 */
int bitset_itr_next(BitsetItr* itr);

/**
 * @def BITSET_DEFINE
 * @brief Make a standard bitset
 *
 * Make a bitset with a valid static array to store it's array of words.
 *
 * Use this to define bitsets in the code, specifically as a `static` scoped
 * variable. The passed `name` will be the same name as the bitset.
 *
 * Usage example:
 *
 * ```c
 * BITSET_DEFINE(_my_bitset, 128);
 * // normal operation...
 * bitset_clear(&_my_bitset);
 * ```
 *
 * @param name the name of the bitset
 * @param capacity the capacity of the bitset
 */
#define BITSET_DEFINE(name, capacity)                  \
    static uint32_t name##_w[BITSET_ARRAY_SIZE] = {0}; \
    static Bitset name = {                             \
        .w = name##_w,                                 \
        .nbits = BITSET_BITS_PER_WORD,                 \
        .nwords = BITSET_ARRAY_SIZE,                   \
        .cap = capacity,                               \
    };

#endif // BITSET_H
