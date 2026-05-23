#ifndef POOL_H
#define POOL_H

#include "bitset.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef POOLS_TEST_ENV
#define POOLS_DEF_FILE "def_test_mempool.h"
#else
#define POOLS_DEF_FILE "def_balatro_mempool.h"
#endif

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

#define POOL_GET(type)       pool_get_##type()
#define POOL_FREE(type, obj) pool_free_##type(obj)
#define POOL_IDX(type, obj)  pool_idx_##type(obj) // the index of the object
#define POOL_AT(type, idx)   pool_at_##type(idx)  // the object at

#define POOL_ENTRY(name, capacity) POOL_DECLARE_TYPE(name);
#include POOLS_DEF_FILE
#undef POOL_ENTRY

#endif // POOL_H
