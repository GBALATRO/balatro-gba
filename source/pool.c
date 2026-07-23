#include "pool.h"

#define POOL_ENTRY(name, capacity, ram) POOL_DEFINE_TYPE(name, capacity, ram);
#include POOLS_DEF_FILE
#undef POOL_ENTRY
