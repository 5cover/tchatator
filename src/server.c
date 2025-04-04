/// @file
/// @author Raphaël
/// @brief Tchatator413 server - Main program
/// @date 23/01/2025

#include <assert.h>
#include <getopt.h>
#include <json-c.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchatator413/cfg.h>
#include <tchatator413/json-helpers.h>
#include <tchatator413/tchatator413.h>
#include <tchatator413/util.h>
#include <unistd.h>

/* to run it
set -a
. .env
set +a
./tchatator-server

testing:
nc 127.0.0.1 4113 <<< '[]'

*/

static inline char const *require_env(cfg_t *cfg, char const *name) {
    char *value = getenv(name);
    if (!value) {
        cfg_log(cfg, log_error, "envvar missing: %s\n", name);
        exit(EX_USAGE);
    }
    return value;
}

int main(int argc, char **argv) {
    int verbosity = 0;
    bool dump_config = false, interactive = false;

    cfg_t *cfg = NULL;

    // Arguments
    {
        enum {
            opt_help,
            opt_version,
            opt_dump_config,
            opt_quiet = 'q',
            opt_verbose = 'v',
            opt_interactive = 'i',
            opt_config = 'c',
        };
        struct option long_options[] = {
            {
                .name = "help",
                .val = opt_help,
            },
            {
                .name = "version",
                .val = opt_version,
            },
            {
                .name = "dump-config",
                .val = opt_dump_config,
            },
            {
                .name = "quiet",
                .val = opt_quiet,
            },
            {
                .name = "verbose",
                .val = opt_verbose,
            },
            {
                .name = "interactive",
                .val = opt_interactive,
            },
            {
                .name = "config",
                .val = opt_config,
            },
            { 0 },
        };

        int opt;
        while (-1 != (opt = getopt_long(argc, argv, "qvic:", long_options, NULL))) {
            switch (opt) {
            case opt_help:
                puts(HELP);
                return EXIT_SUCCESS;
            case opt_version:
                puts(VERSION);
                return EXIT_SUCCESS;
            case opt_dump_config:
                dump_config = true;
                break;
            case opt_quiet: --verbosity; break;
            case opt_verbose: ++verbosity; break;
            case opt_interactive: interactive = true; break;
            case opt_config:
                if (cfg) {
                    cfg_log(cfg, log_error, "config already specified by previous argument\n");
                    return EX_USAGE;
                }
                cfg = cfg_from_file(optarg);
                if (!cfg) return EX_CONFIG;
                break;
            case '?':
                puts(HELP);
                return EX_USAGE;
            }
        }
    }

    if (!cfg) cfg = cfg_defaults();

    int result;

    cfg_set_verbosity(cfg, verbosity);

    if (dump_config) {
        cfg_dump(cfg);
        result = EX_OK;
    } else {
        db_t *db = db_connect(cfg, verbosity,
            require_env(cfg, "DB_HOST"),
            require_env(cfg, "PGDB_PORT"),
            require_env(cfg, "DB_NAME"),
            require_env(cfg, "DB_USER"),
            require_env(cfg, "DB_ROOT_PASSWORD"));
        if (!db) return EX_NODB;
        
        api_key_t admin_api_key;
        if (!uuid4_parse(&admin_api_key, require_env(cfg, "ADMIN_API_KEY"))) {
            cfg_log(cfg, log_error, "invalid ADMIN_API_KEY\n");
            return EX_USAGE;
        }

        server_t *server = server_create(admin_api_key, require_env(cfg, "ADMIN_PASSWORD"));

        result = interactive
            ? tchatator413_run_interactive(cfg, db, server, argc, argv)
            : tchatator413_run_socket(cfg, db, server);

        server_destroy(server);
        db_destroy(db);
    }

    cfg_destroy(cfg);

    return result;
}
