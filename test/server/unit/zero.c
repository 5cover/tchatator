/// @file
/// @author Raphaël
/// @brief Tchatator413 test - empty request
/// @date 1/02/2025

#include "../tests.h"
#include <tchatator413/tchatator413.h>

#define NAME zero

#define IN "[]"
#define OUT "[]"

static void on_action(action_t const *action, void *t) {
    base_on_action(t);
    (void)action;
}

static void on_response(response_t const *response, void *t) {
    base_on_response(t);
    (void)response;
}

TEST_SIGNATURE(NAME) {
    test_t test = TEST_INIT(NAME);

    json_object *obj_input = json_tokener_parse(IN);

    json_object *obj_output = tchatator413_interpret(obj_input, cfg, db, server, on_action, on_response, &test);
    test_case_n_actions(&test, 0);

    json_object *obj_expected_output = json_tokener_parse(OUT);
    test_output_json(&test.t, obj_output, obj_expected_output);
    json_object_put(obj_expected_output);
    json_object_put(obj_output);
    json_object_put(obj_input);

    return test.t;
}
