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
static const u16 MGBA_LOG_LEVEL_MASK = 0x7;

static bool mgba_logger_available = false;

bool mgba_logger_init(void)
{
    *MGBA_REG_DEBUG_ENABLE = MGBA_ENABLE_MAGIC;
    mgba_logger_available = (*MGBA_REG_DEBUG_ENABLE == MGBA_ENABLE_OK);
    return mgba_logger_available;
}

static void mgba_vprintf(MgbaLogLevel level, const char* fmt, va_list args)
{
    if (!mgba_logger_available || fmt == NULL)
        return;

    vsnprintf(MGBA_REG_DEBUG_STRING, MGBA_LOG_BUFFER_SIZE, fmt, args);

    *MGBA_REG_DEBUG_FLAGS = ((uint16_t)level & MGBA_LOG_LEVEL_MASK) | MGBA_LOG_SEND;
}

void mgba_printf(MgbaLogLevel level, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    mgba_vprintf(level, fmt, args);
    va_end(args);
}

void mgba_func_printf(MgbaLogLevel level, const char* func_name, const char* fmt, ...)
{
    if (!mgba_logger_available || func_name == NULL || fmt == NULL)
    {
        // The one place where we can't log the error.
        return;
    }

    char printed_str_buff[MGBA_LOG_BUFFER_SIZE];

    // Expand the format first so the full string is truncated in case it's too long
    va_list args;
    va_start(args, fmt);
    vsnprintf(printed_str_buff, sizeof(printed_str_buff), fmt, args);
    va_end(args);
    mgba_printf(level, "%s(): %s", func_name, printed_str_buff);
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

void mgba_func_printf(MgbaLogLevel level, const char* func_name, const char* fmt, ...)
{
}
#endif
