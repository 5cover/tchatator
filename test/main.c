/// @file
/// @author Raphaël
/// @brief Tchatator413 test utilities - Implementation
/// @date 1/02/2025

#include "server/tests.h"
#include "tchatator413/cfg.h"
#include "tchatator413/const.h"
#include "tchatator413/db.h"
#include <stdlib.h>

// #define DO_OBSERVE
#define OUT stdout

int main(void) {
    struct test t;
    bool success = true;

#define test(new_test)                \
    do {                              \
        t = new_test;                 \
        success &= test_end(&t, OUT); \
    } while (0)

    test(test_uuid4());
    test(test_memlst());

    // probably a bad idea to proceed if uuid4 or memlst are bad
    if (!success) return EXIT_FAILURE;

    memlst_t *mem = memlst_init();

    cfg_t *cfg = memlst_add(&mem, (dtor_fn)cfg_destroy, cfg_defaults());
    cfg_set_verbosity(cfg, INT_MAX);

    {
        api_key_t root_api_key;
        if (!uuid4_parse(&root_api_key, require_env(cfg, "ROOT_API_KEY"))) {
            cfg_log(cfg, log_error, "invalid ROOT_API_KEY\n");
            CLEAN_RETURN(mem, EX_USAGE);
        }
        cfg_load_root_credentials(cfg, root_api_key, require_env(cfg, "ROOT_PASSWORD"));
    }

    db_t *db = memlst_add(&mem, (dtor_fn)db_destroy,
        db_connect(cfg,
            require_env(cfg, "DB_HOST"),
            require_env(cfg, "DB_PORT"),
            require_env(cfg, "DB_NAME"),
            require_env(cfg, "DB_USER"),
            require_env(cfg, "DB_PASSWORD")));
    if (!db) CLEAN_RETURN(mem, EX_NODB);

#define CALL_TEST(name) test(test_tchatator413_##name(&mem, cfg, db));
    X_TESTS(CALL_TEST)
#undef CALL_TEST

#ifdef DO_OBSERVE
    observe_put_role();
#endif

    CLEAN_RETURN(mem, success ? EXIT_SUCCESS : EXIT_FAILURE);
}
