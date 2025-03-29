/// @file
/// @author Raphaël
/// @brief Tchatator413 test - login of two users simultenaously
/// @date 1/02/2025

#include "../tests.h"
#include <tchatator413/tchatator413.h>

#define NAME member1_login_member2_login_member1_logout_member2_logout

static void on_action_login(action_t const *action, void *t) {
    test_t *test = base_on_action(t);
    if (!test_case_eq_int(t, action->type, action_type_login, )) return;
    switch (test->n_actions) {
    case 1:
        test_case_eq_uuid(t, action->with.login.api_key, API_KEY_MEMBER1_UUID, );
        test_case_eq_str(t, action->with.login.password.val, "member1_mdp", );
        break;
    case 2:
        test_case_eq_uuid(t, action->with.login.api_key, API_KEY_MEMBER2_UUID, );
        test_case_eq_str(t, action->with.login.password.val, "member2_mdp", );
        break;
    default:
        test_fail(t, "wrong test->n_actions: %d", test->n_actions);
    }
}

static token_t gs_tokens[2];

static void on_response_login(response_t const *response, void *t) {
    test_t *test = base_on_response(t);
    test_case(t, !response->has_next_page, "");
    if (!test_case_eq_int(t, response->type, action_type_login, )) return;
    gs_tokens[test->n_responses - 1] = response->body.login.token;
    test_case(t, -1 != server_verify_token(test->server, response->body.login.token), "server verifies token %ld", response->body.login.token);
}

static void on_action_logout(action_t const *action, void *t) {
    test_t *test = base_on_action(t);
    if (!test_case_eq_int(t, action->type, action_type_logout, )) return;
    test_case_eq_int64(t, action->with.logout.token, gs_tokens[test->n_actions - 3], );
}

static void on_response_logout(response_t const *response, void *t) {
    base_on_response(t);
    test_case_eq_int(t, response->type, action_type_logout, );
}

TEST_SIGNATURE(NAME) {
    test_t test = TEST_INIT(NAME);

#define STOP()                       \
    do {                             \
        json_object_put(obj_input);  \
        json_object_put(obj_output); \
        return test.t;               \
    } while (0)

    json_object *obj_input = json_object_from_file(IN_JSON(NAME, "1"));
    json_object *obj_output = tchatator413_interpret(obj_input, cfg, db, server, on_action_login, on_response_login, &test);
    test_case_n_actions(&test, 2);
    if (!test_output_json_file(&test, obj_output, OUT_JSON(NAME, "1"))) STOP();

    json_object_put(obj_input);
    obj_input = load_jsonf(IN_JSONF(NAME, "2"), gs_tokens[0], gs_tokens[1]);
    json_object_put(obj_output);
    obj_output = tchatator413_interpret(obj_input, cfg, db, server, on_action_logout, on_response_logout, &test);
    test_case_n_actions(&test, 4);
    test_output_json_file(&test, obj_output, OUT_JSON(NAME, "2"));

    STOP();
}
