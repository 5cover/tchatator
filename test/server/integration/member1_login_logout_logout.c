/// @file
/// @author Raphaël
/// @brief Tchatator413 test - member1 logs in an logs out twice. The second time should be an error.
/// @date 1/02/2025

#include "../tests.h"
#include <tchatator413/tchatator413.h>

#define NAME member1_login_logout_logout

static token_t gs_token;

static void on_action(action_t const *action, void *t) {
    test_t *test = base_on_action(t);
    switch (test->n_actions) {
    case 1:
        if (!test_case_eq_int(t, action->type, action_type_login, )) return;
        test_case_eq_uuid(t, action->with.login.api_key, API_KEY_MEMBER1_UUID, );
        test_case_eq_str(t, action->with.login.password.val, "member1_mdp", );
        break;
    case 2:
    case 3:
        if (!test_case_eq_int(t, action->type, action_type_logout, )) return;
        test_case_eq_int64(t, action->with.logout.token, gs_token, );
        break;
    default:
        test_fail(t, "wrong test->n_actions: %d", test->n_actions);
    }
}

static void on_response(response_t const *response, void *t) {
    test_t *test = base_on_response(t);
    test_case(t, !response->has_next_page, "");
    switch (test->n_responses) {
    case 1:
        if (!test_case_eq_int(t, response->type, action_type_login, )) return;
        gs_token = response->body.login.token;
        test_case(t, -1 != server_verify_token(test->server, response->body.login.token), "server verifies token %ld", response->body.login.token);
        break;
    case 2:
        test_case_eq_int(t, response->type, action_type_logout, );
        break;
    case 3:
        if (!test_case_eq_int(t, response->type, action_type_error, )) return;
        if (!test_case_eq_int(t, response->body.error.type, action_error_type_other, )) return;
        test_case_eq_int(t, response->body.error.info.other.status, status_unauthorized, );
        break;
    default:
        test_fail(t, "wrong test->n_reponse: %d", test->n_responses);
    }
}

TEST_SIGNATURE(NAME) {
    test_t test = TEST_INIT(NAME);

    json_object *obj_input = json_object_from_file(IN_JSON(NAME, "1"));
    json_object *obj_output = tchatator413_interpret(obj_input, cfg, db, server, on_action, on_response, &test);
    test_case_n_actions(&test, 1);
    bool ok = test_output_json_file(&test, obj_output, OUT_JSON(NAME, "1"));
    json_object_put(obj_input);
    json_object_put(obj_output);
    if (!ok) return test.t;

    obj_input = load_jsonf(IN_JSONF(NAME, "2"), gs_token);

    obj_output = tchatator413_interpret(obj_input, cfg, db, server, on_action, on_response, &test);
    test_case_n_actions(&test, 2);
    test_output_json_file(&test, obj_output, OUT_JSON(NAME, "2"));
    json_object_put(obj_output);

    obj_output = tchatator413_interpret(obj_input, cfg, db, server, on_action, on_response, &test);
    test_case_n_actions(&test, 3);
    test_output_json_file(&test, obj_output, OUT_JSON(NAME, "3"));
    json_object_put(obj_output);

    json_object_put(obj_input);

    return test.t;
}
