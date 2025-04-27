/// @file
/// @author Raphaël
/// @brief Tchatator413 test - empty request
/// @date 1/02/2025

#include "../tests.h"
#include "tchatator413/tchatator413.h"

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
    test_t tst = TEST_INIT(NAME);

    json_object *jo_input = memlst_add(tst.p_mem, dtor_json_object,
        json_tokener_parse(IN));

    json_object *jo_output = memlst_add(tst.p_mem, dtor_json_object,
        tchatator413_interpret(jo_input, tst.cfg, tst.db, on_action, on_response, &tst));
    test_case_n_actions(&tst, 0);

    json_object *jo_expected_output = memlst_add(tst.p_mem, dtor_json_object,
        json_tokener_parse(OUT));
    TEST_OUTPUT_JSON(&tst.t, jo_output, jo_expected_output);

    return tst.t;
}
