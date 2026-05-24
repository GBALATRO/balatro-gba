#include "pool.h"

#define POOL_ENTRY(name, capacity) POOL_DEFINE_TYPE(name, capacity);
#include POOLS_DEF_FILE
#undef POOL_ENTRY
