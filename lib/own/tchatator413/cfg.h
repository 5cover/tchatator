/// @file
/// @author Raphaël
/// @brief Tchatator413 server configuration - Interface
/// @date 29/01/2025

#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/// @brief An opaque handle to a configuration object.
typedef struct cfg cfg_t;

/// @brief Load the default configuration.
/// @return A new configuration object.
///
/// The default verbosity is 0.
cfg_t *cfg_defaults(void);

/// @brief Destroy a configuration.
/// @param cfg The configuration to destroy. No-op if @c NULL.
void cfg_destroy(cfg_t *cfg);

/// @brief Load configuration from a file.
/// @param filename The filename to read the config from.
/// @return A new configuration object.
cfg_t *cfg_from_file(char const *filename);

/// @brief Dump a configuration to standard output.
/// @param cfg The configuration to dump.
void cfg_dump(cfg_t const *cfg);

/// @brief Set the logging verbosity.
/// @param cfg The configuration.
/// @param verbosity The logging verbosity.
void cfg_set_verbosity(cfg_t *cfg, int verbosity);

/// @brief Level of a log entry.
typedef enum {
    log_error, ///< Error. Always logged.
    log_info, ///< Informational. Logged when verbosity is > @c 0.
    log_warning, ///< Warning. Logged when verbosity is >= @c 0.
} log_lvl_t;

/// @brief Log a formatted string.
/// @param file The file.
/// @param line The line.
/// @param cfg The configuration.
/// @param lvl The logging level. With the configured verbosity, this determines whether an entry will be logged or not.
/// @param fmt The format string.
/// @param ... Arguments to the format string.
/// @return @c true if an entry has been logged.
/// @return @c false if no entry has been logged.
bool _cfg_log(char const *file, int line,
    cfg_t *cfg, log_lvl_t lvl, char const *fmt, ...);
#define cfg_log(cfg, lvl, fmt, ...) _cfg_log(__FILE__, __LINE__, cfg, lvl, fmt __VA_OPT__(,) __VA_ARGS__)

/// @brief Log a single character.
/// @param cfg The configuration.
/// @param c A character.
void cfg_log_putc(cfg_t *cfg, char c);

/// @brief Get the configuration max_msg_length.
/// @param cfg Configuration
/// @return the configuration max_msg_length.
size_t cfg_max_msg_length(cfg_t const *cfg);
/// @brief Get the configuration page_inbox.
/// @param cfg Configuration
/// @return the configuration page_inbox.
int cfg_page_inbox(cfg_t const *cfg);
/// @brief Get the configuration page_outbox.
/// @param cfg Configuration
/// @return the configuration page_outbox.
int cfg_page_outbox(cfg_t const *cfg);
/// @brief Get the configuration rate_limit_m.
/// @param cfg Configuration
/// @return the configuration rate_limit_m.
int cfg_rate_limit_m(cfg_t const *cfg);
/// @brief Get the configuration rate_limit_h.
/// @param cfg Configuration
/// @return the configuration rate_limit_h.
int cfg_rate_limit_h(cfg_t const *cfg);
/// @brief Get the configuration block_for.
/// @param cfg Configuration
/// @return the configuration block_for.
int cfg_block_for(cfg_t const *cfg);
/// @brief Get the configuration backlog.
/// @param cfg Configuration
/// @return the configuration backlog.
int cfg_backlog(cfg_t const *cfg);
/// @brief Get the configuration port.
/// @param cfg Configuration
/// @return the configuration port.
uint16_t cfg_port(cfg_t const *cfg);

#endif // CONFIG_H
