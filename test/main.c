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

#define PGDB_PORT "5432"
#define DB_USER "postgres"
#define DB_NAME "sae413_test"
#define DB_ROOT_PASSWORD "postgres"
#define DB_HOST "localhost"

int main(void) {
    struct test t;
    bool success = true;
    cfg_t *cfg = cfg_defaults();
    cfg_load_root_credentials(cfg, API_KEY_TEST_ROOT_UUID, PASSOWRD_TEST_ROOT);
    db_t *db = db_connect(cfg, 0, DB_HOST, PGDB_PORT, DB_NAME, DB_USER, DB_ROOT_PASSWORD);
    if (!db) return EX_NODB;

#define test(new_test)                \
    do {                              \
        t = new_test;                 \
        success &= test_end(&t, OUT); \
    } while (0)

    test(test_uuid4());

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
