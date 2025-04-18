/// @file
/// @author Raphaël
/// @brief Tchatator413 server configuration - Implementation
/// @date 29/01/2025

#include "tchatator413/cfg.h"
#include "tchatator413/json-helpers.h"
#include "tchatator413/uuid.h"
#include "util.h"
#include <bcrypt/bcrypt.h>
#include <errno.h>
#include <json-c/json.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <time.h>

#define STD_LOG_STREAM stderr

struct cfg {
    FILE *log_file;
    size_t max_msg_length;
    int page_inbox;
    int page_outbox;
    int rate_limit_m;
    int rate_limit_h;
    int block_for;
    int backlog;
    uint16_t port;
    char *log_file_name; ///< @remark Can be @c NULL if log_file is a standard stream.
    int verbosity;
    uuid4_t root_api_key;
    char root_password_hash[BCRYPT_HASHSIZE];
};

#define INTRO "config: "

char const *require_env(cfg_t *cfg, char const *name) {
    char *value = getenv(name);
    if (!value) {
        cfg_log(cfg, log_error, "envvar missing: %s\n", name);
        exit(EX_USAGE);
    }
    return value;
}

static inline void i_vlog(char const *file, int line, FILE *stream, log_lvl_t lvl, char const *fmt, va_list ap) {
    time_t t = time(NULL);
    char timestr[32];
    strftime(timestr, sizeof timestr, "%F %H:%M:%S", localtime(&t));
    fprintf(stream, "%s:%s:%d: ", timestr, file, line);
    switch (lvl) {
    case log_error: fputs("error: ", stream); break;
    case log_info: fputs("info: ", stream); break;
    case log_warning: fputs("warning: ", stream); break;
    }
    vfprintf(stream, fmt, ap);
}

static inline void i_log(char const *file, int line, FILE *stream, log_lvl_t lvl, char const *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    i_vlog(file, line, stream, lvl, fmt, ap);
    va_end(ap);
}

#define log(stream, lvl, fmt, ...) i_log(__FILE__, __LINE__, stream, lvl, fmt __VA_OPT__(, ) __VA_ARGS__)

static inline FILE *open_log_file(cfg_t *cfg) {
    if (cfg->log_file == STD_LOG_STREAM && cfg->log_file_name) {
        FILE *log_file;
        if ((log_file = fopen(cfg->log_file_name, "a"))) {
            // Disable buffering
            setvbuf(cfg->log_file = log_file, NULL, _IONBF, 0);
        } else {
            log(STD_LOG_STREAM, log_error, INTRO "could not open log file: %s\n", strerror(errno));
            log(STD_LOG_STREAM, log_info, INTRO "logging will continue on " STR(STD_LOG_STREAM));
        }
    }
    return cfg->log_file;
}

cfg_t *cfg_defaults(void) {
    cfg_t *p_cfg = malloc(sizeof *p_cfg);
    if (!p_cfg) errno_exit("malloc");

    p_cfg->log_file = STD_LOG_STREAM;
    p_cfg->log_file_name = NULL;
    p_cfg->verbosity = 0;

    p_cfg->backlog = 1;
    p_cfg->block_for = 86400;
    p_cfg->max_msg_length = 1000;
    p_cfg->page_inbox = 20;
    p_cfg->page_outbox = 20;
    p_cfg->port = 4113;
    p_cfg->rate_limit_h = 90;
    p_cfg->rate_limit_m = 12;
    return p_cfg;
}

void cfg_load_root_credentials(cfg_t *cfg, uuid4_t root_api_key, const char *root_password) {
    cfg->root_api_key = root_api_key;
    char salt[BCRYPT_HASHSIZE];
    if (0 != bcrypt_gensalt(12, salt)) errno_exit("bcrypt_gensalt");
    if (0 != bcrypt_hashpw(root_password, salt, cfg->root_password_hash)) errno_exit("bcrypt_hashpw");
}

void cfg_destroy(cfg_t *cfg) {
    if (!cfg) return;
    if (cfg->log_file && cfg->log_file != STD_LOG_STREAM) fclose(cfg->log_file);
    free(cfg->log_file_name);
    free(cfg);
}

void cfg_set_verbosity(cfg_t *cfg, int verbosity) {
    cfg->verbosity = verbosity;
}

void cfg_load_from_file(cfg_t *cfg, char const *filename) {
    json_object *jo, *jo_cfg = json_object_from_file(filename);

    if (!jo_cfg) {
        log(STD_LOG_STREAM, log_error, INTRO LOG_FMT_JSON_C("failed to parse config file at '%s'", filename));
        log(STD_LOG_STREAM, log_info, INTRO "using defaults\n");
    }

    if (json_object_object_get_ex(jo_cfg, "log_file", &jo)) {
        slice_t log_filename;
        if (!json_object_get_string_strict(jo, &log_filename)) {
            log(STD_LOG_STREAM, log_error, INTRO LOG_FMT_JSON_TYPE(json_type_string, json_object_get_type(jo), "log_file"));
        }
        if (log_filename.len != 1 && log_filename.val[0] != '-' && !(cfg->log_file_name = strndup(log_filename.val, log_filename.len))) {
            errno_exit("strndup");
        }
    }
    if (json_object_object_get_ex(jo_cfg, "backlog", &jo) && !json_object_get_int_strict(jo, &cfg->backlog)) {
        log(STD_LOG_STREAM, log_error, INTRO LOG_FMT_JSON_TYPE(json_type_int, json_object_get_type(jo), "backlog"));
    }
    if (json_object_object_get_ex(jo_cfg, "block_for", &jo) && !json_object_get_int_strict(jo, &cfg->block_for)) {
        log(STD_LOG_STREAM, log_error, INTRO LOG_FMT_JSON_TYPE(json_type_int, json_object_get_type(jo), "block_for"));
    }
    if (json_object_object_get_ex(jo_cfg, "max_msg_length", &jo)) {
        int64_t max_msg_length;
        if (!json_object_get_int64_strict(jo, &max_msg_length)) {
            log(STD_LOG_STREAM, log_error, INTRO LOG_FMT_JSON_TYPE(json_type_int, json_object_get_type(jo), "max_msg_length"));
        }
        if (max_msg_length < 0) {
            log(STD_LOG_STREAM, log_error, INTRO "max_msg_length: must be > 0\n");
        } else {
            cfg->max_msg_length = (size_t)max_msg_length;
        }
    }
    if (json_object_object_get_ex(jo_cfg, "page_inbox", &jo) && !json_object_get_int_strict(jo, &cfg->page_inbox)) {
        log(STD_LOG_STREAM, log_error, INTRO LOG_FMT_JSON_TYPE(json_type_int, json_object_get_type(jo), "page_inbox"));
    }
    if (json_object_object_get_ex(jo_cfg, "page_outbox", &jo) && !json_object_get_int_strict(jo, &cfg->page_outbox)) {
        log(STD_LOG_STREAM, log_error, INTRO LOG_FMT_JSON_TYPE(json_type_int, json_object_get_type(jo), "page_outbox"));
    }
    if (json_object_object_get_ex(jo_cfg, "port", &jo) && !json_object_get_uint16_strict(jo, &cfg->port)) {
        log(STD_LOG_STREAM, log_error, INTRO LOG_FMT_JSON_TYPE(json_type_int, json_object_get_type(jo), "port"));
    }
    if (json_object_object_get_ex(jo_cfg, "rate_limit_h", &jo) && !json_object_get_int_strict(jo, &cfg->rate_limit_h)) {
        log(STD_LOG_STREAM, log_error, INTRO LOG_FMT_JSON_TYPE(json_type_int, json_object_get_type(jo), "rate_limit_h"));
    }
    if (json_object_object_get_ex(jo_cfg, "rate_limit_m", &jo) && !json_object_get_int_strict(jo, &cfg->rate_limit_m)) {
        log(STD_LOG_STREAM, log_error, INTRO LOG_FMT_JSON_TYPE(json_type_int, json_object_get_type(jo), "rate_limit_m"));
    }

    json_object_put(jo_cfg);
}

void cfg_dump(cfg_t const *cfg) {
    puts("CONFIGURATION");
    printf("backlog         %d\n", cfg->backlog);
    printf("block_for       %d seconds\n", cfg->block_for);
    printf("log_file        %s\n", COALESCE(cfg->log_file_name, "-"));
    printf("max_msg_length  %zu characters\n", cfg->max_msg_length);
    printf("page_inbox      %d\n", cfg->page_inbox);
    printf("page_outbox     %d\n", cfg->page_outbox);
    printf("port            %hd\n", cfg->port);
    printf("rate_limit_h    %d\n", cfg->rate_limit_h);
    printf("rate_limit_m    %d\n\n", cfg->rate_limit_m);

    printf("log verbosity   %d\n", cfg->verbosity);
}

bool i_cfg_log(char const *file, int line, cfg_t *cfg, log_lvl_t lvl, char const *fmt, ...) {
    if (lvl == log_info && cfg->verbosity <= 0
        || lvl == log_warning && cfg->verbosity < 0) return false;
    va_list ap;
    va_start(ap, fmt);
    i_vlog(file, line, open_log_file(cfg), lvl, fmt, ap);
    va_end(ap);
    return true;
}

void cfg_log_putc(cfg_t *cfg, char c) {
    putc(c, open_log_file(cfg));
}

bool cfg_verify_root_constr(cfg_t const *cfg, constr_t constr) {
    if (!uuid4_eq(cfg->root_api_key, constr.api_key)) return false;
    switch (bcrypt_checkpw(constr.password, cfg->root_password_hash)) {
    case -1: errno_exit("bcrypt_checkpw");
    case 0: return true;
    default: return false;
    }
}

#define DEFINE_CONFIG_GETTER(type, attr) \
    type cfg_##attr(cfg_t const *cfg) {  \
        return cfg->attr;                \
    }

DEFINE_CONFIG_GETTER(size_t, max_msg_length)
DEFINE_CONFIG_GETTER(int, page_inbox)
DEFINE_CONFIG_GETTER(int, page_outbox)
DEFINE_CONFIG_GETTER(int, rate_limit_m)
DEFINE_CONFIG_GETTER(int, rate_limit_h)
DEFINE_CONFIG_GETTER(int, block_for)
DEFINE_CONFIG_GETTER(int, backlog)
DEFINE_CONFIG_GETTER(uint16_t, port)
DEFINE_CONFIG_GETTER(int, verbosity)
