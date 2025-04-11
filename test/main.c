/// @file
/// @author Raphaël
/// @brief Tchatator413 test utilities - Implementation
/// @date 1/02/2025

#include "server/tests.h"
#include "tchatator413/const.h"
#include <stdlib.h>
#include <tchatator413/cfg.h>
#include <tchatator413/db.h>

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

    cfg_t *cfg = cfg_defaults();

    {
        api_key_t root_api_key;
        if (!uuid4_parse(&root_api_key, require_env(cfg, "ROOT_API_KEY"))) {
            cfg_log(cfg, log_error, "invalid ROOT_API_KEY\n");
            return EX_USAGE;
        }
        cfg_load_root_credentials(cfg, root_api_key, require_env(cfg, "ROOT_PASSWORD"));
    }

    db_t *db = db_connect(cfg, INT_MAX,
        require_env(cfg, "DB_HOST"),
        require_env(cfg, "DB_PORT"),
        require_env(cfg, "DB_NAME"),
        require_env(cfg, "DB_USER"),
        require_env(cfg, "DB_PASSWORD"));

    if (!db) return EX_NODB;

#define CALL_TEST(name) test(test_tchatator413_##name(cfg, db));
    X_TESTS(CALL_TEST)
#undef CALL_TEST

#ifdef DO_OBSERVE
    observe_put_role();
#endif

    cfg_destroy(cfg);
    db_destroy(db);

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
