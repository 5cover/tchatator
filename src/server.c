/// @file
/// @author Raphaël
/// @brief Tchatator413 server - Main program
/// @date 23/01/2025

#include <assert.h>
#include <getopt.h>
#include "json-c.h"
#include "memlst.h"
#include <stdio.h>
#include <stdlib.h>
#include "tchatator413/cfg.h"
#include "tchatator413/json-helpers.h"
#include "tchatator413/tchatator413.h"
#include "util.h"
#include <unistd.h>

/* to run it
set -a
. .env
set +a
./tchatator-server

testing:
nc 127.0.0.1 4113 <<< '[]'

*/

int main(int argc, char **argv) {
    int verbosity = 0;
    bool dump_config = false, interactive = false;

    memlst_t *mem = memlst_init();

    cfg_t *cfg = memlst_add(&mem, (dtor_fn)cfg_destroy, cfg_defaults());

    {
        api_key_t root_api_key;
        if (!uuid4_parse(&root_api_key, require_env(cfg, "ROOT_API_KEY"))) {
            cfg_log(cfg, log_error, "invalid ROOT_API_KEY\n");
            CLEAN_RETURN(mem, EX_USAGE);
        }
        cfg_load_root_credentials(cfg, root_api_key, require_env(cfg, "ROOT_PASSWORD"));
    }

    // Arguments
    {
        enum {
            OPT_HELP,
            OPT_VERSION,
            OPT_DUMP_CONFIG,
            OPT_QUIET = 'q',
            OPT_VERBOSE = 'v',
            OPT_INTERACTIVE = 'i',
            OPT_CONFIG = 'c',
        };
        struct option d_long_options[] = {
            {
                .name = "help",
                .val = OPT_HELP,
            },
            {
                .name = "version",
                .val = OPT_VERSION,
            },
            {
                .name = "dump-config",
                .val = OPT_DUMP_CONFIG,
            },
            {
                .name = "quiet",
                .val = OPT_QUIET,
            },
            {
                .name = "verbose",
                .val = OPT_VERBOSE,
            },
            {
                .name = "interactive",
                .val = OPT_INTERACTIVE,
            },
            {
                .name = "config",
                .val = OPT_CONFIG,
            },
            { 0 },
        };

        int opt;
        while (-1 != (opt = getopt_long(argc, argv, "qvic:", d_long_options, NULL))) {
            switch (opt) {
            case OPT_HELP:
                puts(HELP);
                CLEAN_RETURN(mem, EX_OK);
            case OPT_VERSION:
                puts(VERSION);
                CLEAN_RETURN(mem, EX_OK);
            case OPT_DUMP_CONFIG:
                dump_config = true;
                break;
            case OPT_QUIET: --verbosity; break;
            case OPT_VERBOSE: ++verbosity; break;
            case OPT_INTERACTIVE: interactive = true; break;
            case OPT_CONFIG:
                if (cfg) {
                    cfg_log(cfg, log_error, "config already specified by previous argument\n");
                    CLEAN_RETURN(mem, EX_USAGE);
                }
                cfg_load_from_file(cfg, optarg);
                break;
            case '?':
                puts(HELP);
                CLEAN_RETURN(mem, EX_USAGE);
            }
        }
    }

    cfg_set_verbosity(cfg, verbosity);

    if (dump_config) {
        cfg_dump(cfg);
        CLEAN_RETURN(mem, EX_OK);
    }

    db_t *db = memlst_add(&mem, (dtor_fn)db_destroy,
        db_connect(cfg,
            require_env(cfg, "DB_HOST"),
            require_env(cfg, "DB_PORT"),
            require_env(cfg, "DB_NAME"),
            require_env(cfg, "DB_USER"),
            require_env(cfg, "DB_PASSWORD")));
    if (!db) CLEAN_RETURN(mem, EX_NODB);

    CLEAN_RETURN(mem, interactive ? tchatator413_run_interactive(cfg, db, argc, argv) : tchatator413_run_socket(cfg, db));
}
