/**
 * @file mgba_logger.h
 *
 * @brief Interface to interact with the mgba logger.
 *
 * Use with mgba in the command line.
 *
 * You can pass which logs you'd like to print with a flag passed to mgba
 * with `-l` or `--log-level`.
 *
 * | 7 | 6 | 5 |   4   |   3  |   2  |   1   |   0   |
 * |---|---|---|-------|------|------|-------|-------|
 * | / | / | / | DEBUG | INFO | WARN | ERROR | FATAL |
 *
 * ```sh
 * mgba -l 3 game.rom # ERROR and FATAL
 * mgba -l 14 game.rom # INFO, WARN, and ERROR
 * ```
 *
 * **Note**: You have to fight with other logs in mgba and DEBUG can get
 * messy.
 *
 * **Note**: FATAL does kill the game. Use with care.
 */
#ifndef MGBA_LOGGER_H
#define MGBA_LOGGER_H

#include <stdbool.h>

typedef enum
{
    MGBA_LOG_FATAL = 0,
    MGBA_LOG_ERROR = 1,
    MGBA_LOG_WARN = 2,
    MGBA_LOG_INFO = 3,
    MGBA_LOG_DEBUG = 4,
} MgbaLogLevel;

/**
 * @brief Initialize mgba logger
 *
 * Checks that the logger is available by checking the expected registers
 * magic number
 */
bool mgba_logger_init(void);
/**
 * @brief Print to mgba log with a format string
 *
 * Note, for all logs, it's cutoff at the hard mgba limit of 0x100
 *
 * @param level
 * @param fmt Format string
 * @param ... variadic arguments
 */
void mgba_printf(MgbaLogLevel level, const char* fmt, ...);

// clang-format off
#ifdef MGBA_LOGGING
#define MGBA_FATAL(...) mgba_printf(MGBA_LOG_FATAL, __VA_ARGS__)
#define MGBA_ERROR(...) mgba_printf(MGBA_LOG_ERROR, __VA_ARGS__)
#define MGBA_WARN(...)  mgba_printf(MGBA_LOG_WARN,  __VA_ARGS__)
#define MGBA_INFO(...)  mgba_printf(MGBA_LOG_INFO,  __VA_ARGS__)
#define MGBA_DEBUG(...) mgba_printf(MGBA_LOG_DEBUG, __VA_ARGS__)
#else
#define MGBA_FATAL(...) 
#define MGBA_ERROR(...)
#define MGBA_WARN(...)
#define MGBA_INFO(...)
#define MGBA_DEBUG(...)
#endif
// clang-format on

#endif
