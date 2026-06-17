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
 * @note You have to fight with other logs in mgba and INFO can get messy.
 *
 * @note FATAL does kill the game. Use with care.
 */
#ifndef MGBA_LOGGER_H
#define MGBA_LOGGER_H

#include <stdbool.h>

typedef enum
{
    MGBA_LOG_FATAL,
    MGBA_LOG_ERROR,
    MGBA_LOG_WARN,
    MGBA_LOG_INFO,
    MGBA_LOG_DEBUG,
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
 * @param level
 * @param fmt Format string
 * @param ... variadic arguments
 *
 * @note for all logs, it's cutoff at the hard mgba limit of 0x100
 */
void mgba_printf(MgbaLogLevel level, const char* fmt, ...);

/**
 * @brief Print to mgba log with a format string and function name
 *
 * @param level
 * @param func_name Function name - prepended to the log string "<func_name>(): <log_string>"
 * @param fmt Format string
 * @param ... variadic arguments
 *
 * @note for all logs, it's cutoff at the hard mgba limit of 0x100
 */
void mgba_func_printf(MgbaLogLevel level, const char* func_name, const char* fmt, ...);

// clang-format off
#ifdef MGBA_LOGGING

#define MGBA_FATAL(...) mgba_printf(MGBA_LOG_FATAL, __VA_ARGS__)
#define MGBA_ERROR(...) mgba_printf(MGBA_LOG_ERROR, __VA_ARGS__)
#define MGBA_WARN(...)  mgba_printf(MGBA_LOG_WARN,  __VA_ARGS__)
#define MGBA_INFO(...)  mgba_printf(MGBA_LOG_INFO,  __VA_ARGS__)
#define MGBA_DEBUG(...) mgba_printf(MGBA_LOG_DEBUG, __VA_ARGS__)

#define MGBA_FUNC_LOG(level, ...) mgba_func_printf(level, __func__, __VA_ARGS__)

#define MGBA_FUNC_FATAL(...) MGBA_FUNC_LOG(MGBA_LOG_FATAL, __VA_ARGS__)
#define MGBA_FUNC_ERROR(...) MGBA_FUNC_LOG(MGBA_LOG_ERROR, __VA_ARGS__)
#define MGBA_FUNC_WARN(...)  MGBA_FUNC_LOG(MGBA_LOG_WARN,  __VA_ARGS__)
#define MGBA_FUNC_INFO(...)  MGBA_FUNC_LOG(MGBA_LOG_INFO,  __VA_ARGS__)
#define MGBA_FUNC_DEBUG(...) MGBA_FUNC_LOG(MGBA_LOG_DEBUG, __VA_ARGS__)

#else
          
#define MGBA_FATAL(...) ((void)0)
#define MGBA_ERROR(...) ((void)0)
#define MGBA_WARN(...) ((void)0)
#define MGBA_INFO(...) ((void)0)
#define MGBA_DEBUG(...) ((void)0)

#define MGBA_FUNC_LOG(level, ...) ((void)0)

#define MGBA_FUNC_FATAL(...) ((void)0)
#define MGBA_FUNC_ERROR(...) ((void)0)
#define MGBA_FUNC_WARN(...) ((void)0)
#define MGBA_FUNC_INFO(...) ((void)0)
#define MGBA_FUNC_DEBUG(...) ((void)0)
#endif
// clang-format on

#endif
