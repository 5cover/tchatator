/// @file
/// @author Raphaël
/// @brief Tchatator413 test - login by member1
/// @date 1/02/2025

#include "../tests.h"
#include <tchatator413/tchatator413.h>

#define NAME member1_login

static void on_action(action_t const *action, void *t) {
    base_on_action(t);
    if (!test_case_eq_int(t, action->type, action_type_login, )) return;
    test_case_eq_uuid(t, action->with.login.api_key, API_KEY_MEMBER1_UUID, );
    test_case_eq_str(t, action->with.login.password.val, "member1_mdp", );
}

static void on_response(response_t const *response, void *t) {
    test_t *test = base_on_response(t);
    test_case(t, !response->has_next_page, "");
    if (!test_case_eq_int(t, response->type, action_type_login, )) return;
    test_case(t, -1 != server_verify_token(test->server, response->body.login.token), "server verifies token %ld", response->body.login.token);
}

TEST_SIGNATURE(NAME) {
    test_t test = TEST_INIT(NAME);

    json_object *obj_input = load_json(IN_JSON(NAME, ));

    json_object *obj_output = tchatator413_interpret(obj_input, cfg, db, server, on_action, on_response, &test);
    test_case_n_actions(&test, 1);

    test_output_json_file(&test, obj_output, OUT_JSON(NAME, ));

    json_object_put(obj_output);
    json_object_put(obj_input);

    return test.t;
}
