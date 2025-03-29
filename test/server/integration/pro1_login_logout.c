/// @file
/// @author Raphaël
/// @brief Tchatator413 test - login and logout by pro1
/// @date 1/02/2025

#include "../tests.h"
#include <tchatator413/tchatator413.h>

#define NAME pro1_login_logout

static token_t gs_token;

static void on_action_1(action_t const *action, void *t) {
    base_on_action(t);
    if (!test_case_eq_int(t, action->type, action_type_login, )) return;
    test_case_eq_uuid(t, action->with.login.api_key, API_KEY_PRO1_UUID, );
    test_case_eq_str(t, action->with.login.password.val, "pro1_mdp", );
}

static void on_response_1(response_t const *response, void *t) {
    test_t *test = base_on_response(t);
    test_case(t, !response->has_next_page, "");
    if (!test_case_eq_int(t, response->type, action_type_login, )) return;
    gs_token = response->body.login.token;
    test_case(t, -1 != server_verify_token(test->server, response->body.login.token), "server verifies token %ld", response->body.login.token);
}

static void on_action_2(action_t const *action, void *t) {
    base_on_action(t);
    if (!test_case_eq_int(t, action->type, action_type_logout, )) return;
    test_case_eq_int64(t, action->with.logout.token, gs_token, );
}

static void on_response_2(response_t const *response, void *t) {
    base_on_response(t);
    test_case_eq_int(t, response->type, action_type_logout, );
}

TEST_SIGNATURE(NAME) {
    test_t test = TEST_INIT(NAME);

    json_object *obj_input = load_json(IN_JSON(NAME, "1"));
    json_object *obj_output = tchatator413_interpret(obj_input, cfg, db, server, on_action_1, on_response_1, &test);
    test_case_n_actions(&test, 1);
    bool ok = test_output_json_file(&test, obj_output, OUT_JSON(NAME, "1"));
    json_object_put(obj_input);
    json_object_put(obj_output);
    if (!ok) return test.t;

    obj_input = load_jsonf(IN_JSONF(NAME, "2"), gs_token);
    obj_output = tchatator413_interpret(obj_input, cfg, db, server, on_action_2, on_response_2, &test);
    test_case_n_actions(&test, 2);
    test_output_json_file(&test, obj_output, OUT_JSON(NAME, "2"));
    json_object_put(obj_output);
    json_object_put(obj_input);

    return test.t;
}
