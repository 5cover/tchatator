/// @file
/// @author Raphaël
/// @brief Testing - Interface
/// @date 1/02/2025

#ifndef TESTS_H
#define TESTS_H

#include <assert.h>
#include <json-c.h>
#include <stb_test.h>

#include <tchatator413/cfg.h>
#include <tchatator413/db.h>
#include <tchatator413/json-helpers.h>
#include <tchatator413/server.h>

// Other tests

struct test test_uuid4(void);

void observe_put_role(void);

// Tchattaotr413 Tests

// Tchatator413 test naming convention:
// test_tchatator413_<ROLE>_<ACTION>_<WITH...>
// ROLE is
//  - admin: administrator
//  - pro: professional
//  - member: membre
//  - invalid: invalid token / api key
// ACTION is an action name
// WITH are the action arguments, one or more, separated by '_'

// #pragma GCC diagnostic ignored "-Wmissing-prototypes"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcomment"

/// @brief X-macro that expands to the list of Tchattator413 tests.
#define X_TESTS(X)                                               \
    /* Integration tests (> 1 action) */                         \
    X(admin_login_logout)                                        \
    X(member1_login_login_logout_logout)                         \
    X(member1_login_logout_logout)                               \
    X(member1_login_logout)                                      \
    X(member1_login_member2_login_member1_logout_member2_logout) \
    X(member1_send_pro1_inbox_member1_rm)                        \
    X(pro1_login_logout)                                         \
    /* Unit tests */                                             \
    X(admin_whois_imax)                                          \
    X(admin_whois_neg1)                                          \
    X(admin_whois_pro1)                                          \
    X(empty)                                                     \
    X(invalid_login)                                             \
    X(invalid_logout)                                            \
    X(invalid_whois_pro1)                                        \
    X(malformed)                                                 \
    X(member1_login_wrong_password)                              \
    X(member1_login)                                             \
    X(member1_send)                                              \
    X(member1_whois_member1_by_email)                            \
    X(member1_whois_member1_by_name)                             \
    X(member1_whois_member1)                                     \
    X(member1_whois_pro1_by_email)                               \
    X(member1_whois_pro1_by_name)                                \
    X(member1_whois_pro1)                                        \
    X(pro1_inbox)                                                \
    X(pro1_send)                                                 \
    X(zero)                                                      \
    //
#pragma GCC diagnostic pop

/// @brief Expands to the signature of a Tchattator413 test function
/// @param name The unquoted name of the test.
#define TEST_SIGNATURE(name) struct test CAT(test_tchatator413_, name)(cfg_t * cfg, db_t * db, server_t * server)

#define DECLARE_TEST(name) TEST_SIGNATURE(name);
X_TESTS(DECLARE_TEST)
#undef DECLARE_TEST

#define TEST_INIT(name) {       \
    .t = test_start(STR(name)), \
    .cfg = cfg,                 \
    .db = db,                   \
    .server = server,           \
};

#define OUT_JSON(NAME, suffix) "test/json/" STR(NAME) "/out" suffix ".json"
#define OUT_JSONF(NAME, suffix) OUT_JSON(NAME, suffix) "f"
#define IN_JSON(NAME, suffix) "test/json/" STR(NAME) "/in" suffix ".json"
#define IN_JSONF(NAME, suffix) IN_JSON(NAME, suffix) "f"

// Implementation details

#include <stddef.h>

/// @brief Test admin API key representation.
#define API_KEY_TEST_ADMIN "63291606-ac93-41f7-b248-f0cd25adb61f"
/// @brief Test admin API key UUID.
#define API_KEY_TEST_ADMIN_UUID uuid4_of(0x63, 0x29, 0x16, 0x06, 0xac, 0x93, 0x41, 0xf7, 0xb2, 0x48, 0xf0, 0xcd, 0x25, 0xad, 0xb6, 0x1f)
/// @brief Test admin password.
#define API_KEY_TEST_ADMIN_PASSWORD "theendisnevertheendisnevertheendisnevertheend"

// exist in the test DB
/// @brief Member 1 API key representation.
#define API_KEY_MEMBER1 "123e4567-e89b-12d3-a456-426614174000"
/// @brief Member 1 API key UUID.
#define API_KEY_MEMBER1_UUID uuid4_of(0x12, 0x3e, 0x45, 0x67, 0xe8, 0x9b, 0x12, 0xd3, 0xa4, 0x56, 0x42, 0x66, 0x14, 0x17, 0x40, 0x00)
/// @brief Member 2 API key representation.
#define API_KEY_MEMBER2 "9ea59c5b-bb75-4cc9-8f80-77b4ce851a0b"
/// @brief Member 2 API key UUID.
#define API_KEY_MEMBER2_UUID uuid4_of(0x9e, 0xa5, 0x9c, 0x5b, 0xbb, 0x75, 0x4c, 0xc9, 0x8f, 0x80, 0x77, 0xb4, 0xce, 0x85, 0x1a, 0x0b)
/// @brief Pro 1 API key representation.
#define API_KEY_PRO1 "bb1b5a1f-a482-4858-8c6b-f4746481cffa"
/// @brief Pro 1 API key UUID.
#define API_KEY_PRO1_UUID uuid4_of(0xbb, 0x1b, 0x5a, 0x1f, 0xa4, 0x82, 0x48, 0x58, 0x8c, 0x6b, 0xf4, 0x74, 0x64, 0x81, 0xcf, 0xfa)
/// @brief Pro 2 API key representation.
#define API_KEY_PRO2 "52d43379-8f75-4fbd-8b06-d80a87b2c2b4"
/// @brief Pro 2 API key UUID.
#define API_KEY_PRO2_UUID uuid4_of(0x52, 0xd4, 0x33, 0x79, 0x8f, 0x75, 0x4f, 0xbd, 0x8b, 0x06, 0xd8, 0x0a, 0x87, 0xb2, 0xc2, 0xb4)

// do not exist in the test db
/// @brief "None 1" (non-existent user 1) API key representation.
#define API_KEY_NONE1 "00000000-0000-0000-0000-000000000000"
/// @brief "None 1" (non-existent user 1) API key UUID.
#define API_KEY_NONE1_UUID uuid4_of(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00)
/// @brief "None 2" (non-existent user 2) API key representation.
#define API_KEY_NONE2 "ffffffff-ffff-ffff-ffff-ffffffffffff"
/// @brief "None 2" (non-existent user 2) API key UUID.
#define API_KEY_NONE2_UUID uuid4_of(0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff)

/// @brief A malformed API key representation.
#define API_KEY_MALFORMED "123e4567e89b12d3a456426614174000"

/// @brief User ID of member 1.
#define USER_ID_MEMBER1 5
/// @brief User ID of pro 1.
#define USER_ID_PRO1 1

/// @brief A Tchattator413 test context.
typedef struct {
    /// @brief Backing test.
    struct test t;
    int n_actions, n_responses;
    cfg_t *cfg;
    db_t *db;
    server_t *server;
} test_t;
_Static_assert(offsetof(test_t, t) == 0, "backing test must be at start of struct for implicit base type punning");

/// @brief Test the amount of actions interpeted.
#define test_case_count(t, actual, expected, singular) test_case(t, actual == expected, "expected %d %s%s, got %d", expected, singular, expected == 1 ? "" : "s", actual)

json_object *load_json(char const *input_filename);
json_object *load_jsonf(char const *input_filename, ...);

#define test_output_json(t, obj_output, obj_expected_output) \
    test_case_wide(t, json_object_equal(obj_output, obj_expected_output), "output: %s == %s", min_json(obj_output), min_json(obj_expected_output))

/// @brief output from JSON file test case
bool test_output_json_file(test_t *test, json_object *obj_output, char const *expected_output_filename);

/// @brief Base on_action event handler.
test_t *base_on_action(void *test);
/// @brief Base on_response event handler.
test_t *base_on_response(void *test);

/// @brief Actions count test case
void test_case_n_actions(test_t *test, int expected);

/// @brief Test two JSON objects are equal for equality, with pattern matching using formatting.
///
/// Retrieves the values contained in the variadic arguments.
///
/// @p obj_expected may have contain formatting syntax to indicate the format of the expected values instead of hard strings.
bool json_object_eq_fmt(json_object *obj_actual, json_object *obj_expected);

/// @brief Test an integer for equality with an expected value.
#define test_case_eq_int(t, actual, expected, fmt) test_case((t), actual == expected, fmt " == %d", actual)
/// @brief Test a long for equality with an expected value.
#define test_case_eq_int64(t, actual, expected, fmt) test_case((t), actual == expected, fmt " == %ld", actual)
/// @brief Test a string for equality with an expected value.
#define test_case_eq_str(t, actual, expected, fmt) test_case((t), streq(actual, expected), fmt " == %s", actual)
/// @brief Test a JSON object for equality with an expected value.
#define test_case_eq_json_object(t, actual, expected, fmt) test_case((t), json_object_equal(actual, expected), fmt " == %s", min_json(actual))

extern char _g_test_case_eq_uuid_repr[UUID4_REPR_LENGTH];

/// @brief Test an UUID for equality with an expected value.
#define test_case_eq_uuid(t, actual, expected, fmt) test_case((t), uuid4_eq(actual, expected), fmt " == %" STR(UUID4_REPR_LENGTH) "s", uuid4_repr(expected, _g_test_case_eq_uuid_repr));

/// @brief A special return value for test transaction returns.
#define errstatus_tested (errstatus_t)(max_errstatus + 1)

#endif // TESTS_H
