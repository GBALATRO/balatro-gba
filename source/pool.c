#include "pool.h"

#define POOL_ENTRY(name, capacity, mem_section) POOL_DEFINE_TYPE(name, capacity, mem_section);
#include POOLS_DEF_FILE
#undef POOL_ENTRY
