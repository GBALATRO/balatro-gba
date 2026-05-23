#include "mgba_logger.h"

#ifdef MGBA_LOGGING

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <tonc.h>

#define MGBA_REG_DEBUG_ENABLE ((vu16*)0x4FFF780)
#define MGBA_REG_DEBUG_FLAGS  ((vu16*)0x4FFF700)
#define MGBA_REG_DEBUG_STRING ((char*)0x4FFF600)

static const u32 MGBA_ENABLE_MAGIC = 0xC0DE;
static const u32 MGBA_ENABLE_OK = 0x1DEA;
static const u32 MGBA_LOG_SEND = 0x100;
static const u32 MGBA_LOG_BUFFER_SIZE = 0x100;

static bool mgba_logger_available = false;

bool mgba_logger_init(void)
{
    *MGBA_REG_DEBUG_ENABLE = MGBA_ENABLE_MAGIC;
    mgba_logger_available = (*MGBA_REG_DEBUG_ENABLE == MGBA_ENABLE_OK);
    return mgba_logger_available;
}

void mgba_printf(MgbaLogLevel level, const char* fmt, ...)
{
    if (!mgba_logger_available || fmt == NULL)
        return;

    va_list args;
    va_start(args, fmt);
    vsnprintf(MGBA_REG_DEBUG_STRING, MGBA_LOG_BUFFER_SIZE, fmt, args);
    va_end(args);

    *MGBA_REG_DEBUG_FLAGS = ((uint16_t)level & 0x7) | MGBA_LOG_SEND;
}
#else

// Noop stubs
bool mgba_logger_init(void)
{
    return false;
}

void mgba_printf(MgbaLogLevel level, const char* fmt, ...)
{
}

#endif
