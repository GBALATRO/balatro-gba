/**
 * @file pool.h
 *
 * @brief Memory pool implementation
 *
 * Memory pools predefined arrays of objects with accessors meant to replace basic `malloc` and
 * `free` calls. This is so the memory management is kept controlled, traceable, defragmented, and
 * simple (in terms of how it is stored and accessed at runtime). Pools are stored in contiguous
 * memory. @ref POOL_IDX is used to get the index of objects inside the pool. Additionally, @ref
 * POOL_AT is used to get pointers at the index of pool memory.
 *
 */
#ifndef POOL_H
#define POOL_H

#include "bitset.h"

#include <stdbool.h>
#include <stdint.h>

/**
 * @def POOLS_DEF_FILE
 * @brief Header containing the memory pool definitions.
 *
 * Expands to def_test_mempool.h (a mempool defined per test), when POOLS_TEST_ENV is defined,
 * otherwise @ref def_balatro_mempool.h .
 */
#ifdef POOLS_TEST_ENV
#define POOLS_DEF_FILE "def_test_mempool.h"
#else
#define POOLS_DEF_FILE "def_balatro_mempool.h"
#endif

/**
 * @brief Declares a pool and its allocation/index/free functions.
 * @param type Object type stored in the pool.
 */
#define POOL_DECLARE_TYPE(type)       \
    typedef struct                    \
    {                                 \
        Bitset* bitset;               \
        type* objects;                \
    } type##Pool;                     \
    type* pool_get_##type();          \
    void pool_free_##type(type* obj); \
    int pool_idx_##type(type* obj);   \
    type* pool_at_##type(int idx);

/**
 * @brief Generates a pool and its allocation/index/free functions.
 * @param type Object type stored in the pool.
 * @param capacity Number of objects to allocate for the pool
 */
#define POOL_DEFINE_TYPE(type, capacity)                                \
    BITSET_DEFINE(type##_bitset, capacity)                              \
    static type type##_storage[capacity];                               \
    static type##Pool type##_pool = {                                   \
        .bitset = &type##_bitset,                                       \
        .objects = type##_storage,                                      \
    };                                                                  \
    type* pool_get_##type()                                             \
    {                                                                   \
        int free_offset = bitset_set_next_free_idx(type##_pool.bitset); \
        if (free_offset == -1)                                          \
            return NULL;                                                \
        return &type##_pool.objects[free_offset];                       \
    }                                                                   \
    void pool_free_##type(type* entry)                                  \
    {                                                                   \
        if (entry == NULL)                                              \
            return;                                                     \
        int offset = entry - &type##_pool.objects[0];                   \
        bitset_set_idx(type##_pool.bitset, offset, false);              \
    }                                                                   \
    int pool_idx_##type(type* entry)                                    \
    {                                                                   \
        return entry - &type##_pool.objects[0];                         \
    }                                                                   \
    type* pool_at_##type(int idx)                                       \
    {                                                                   \
        if (idx < 0 || idx >= (type##_pool.bitset)->cap)                \
            return NULL;                                                \
        return &type##_pool.objects[idx];                               \
    }

/**
 * @def POOL_GET
 *
 * @brief Get a pointer of `type` from it's mempool
 *
 * @param type The struct `type` associated with defined `*_mempool.h`
 * @ret A pointer of `type`
 *
 * @sa POOL_FREE
 */
#define POOL_GET(type) pool_get_##type()

/**
 * @def POOL_FREE
 *
 * @brief Free a pointer of `type` from it's mempool
 *
 * @param type The struct `type` associated with defined `*_mempool.h`
 * @param obj A pointer `obj` of `type` with associated pool
 *
 * @warning `type` must be the same type called with @ref POOL_GET. Otherwise it is undefined
 * behavior
 * 
 * @sa POOL_GET
 */
#define POOL_FREE(type, obj) pool_free_##type(obj)

/**
 * @def POOL_IDX
 *
 * @brief Get the index of the array from `type`'s mempool
 *
 * @param type The struct `type` associated with defined `*_mempool.h`
 * @param obj A pointer `obj` of `type` with associated pool
 * @ret The index of `obj` in the mempool
 * 
 * @sa POOL_AT
 */
#define POOL_IDX(type, obj) pool_idx_##type(obj)

/**
 * @def POOL_AT
 *
 * @brief Get the pointer in the mempool of `type` from an index
 *
 * @param type The struct `type` associated with defined `*_mempool.h`
 * @param idx The index of the desired object in the pool, must be a value of 0 to pool_size - 1
 * @ret A pointer of `type`
 * 
 * @sa POOL_IDX
 */
#define POOL_AT(type, idx) pool_at_##type(idx)

// X-macro, these are defined in the mempool included at the top of this file
#define POOL_ENTRY(name, capacity) POOL_DECLARE_TYPE(name);
#include POOLS_DEF_FILE
#undef POOL_ENTRY

#endif // POOL_H
