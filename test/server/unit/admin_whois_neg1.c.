/// @file
/// @author Raphaël
/// @brief Tchatator413 test - whois of -1 by admin
/// @date 1/02/2025

#include "../tests.h"
#include <tchatator413/tchatator413.h>

#define NAME admin_whois_neg1

static void on_action(action_t const *action, void *t) {
    base_on_action(t);
    if (!test_case_eq_int(t, action->type, action_type_error, )) return;
}

static void on_response(response_t const *response, void *t) {
    base_on_response(t);
    test_case(t, !response->has_next_page, "");
    if (!test_case_eq_int(t, response->type, action_type_error, )) return;
    if (!test_case_eq_int(t, response->body.error.type, action_error_type_invalid, )) return;
    test_case_eq_str(t, response->body.error.info.invalid.location, "whois.with.user", );
    test_case_eq_str(t, response->body.error.info.invalid.reason, "invalid user key", );
    json_object *obj_bad = json_object_new_int(-1);
    test_case_eq_json_object(t, response->body.error.info.invalid.obj_bad, obj_bad, );
    json_object_put(obj_bad);
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
