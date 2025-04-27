/// @file
/// @author Raphaël
/// @brief Tchatator413 test - empty request
/// @date 1/02/2025

#include "../tests.h"
#include "tchatator413/tchatator413.h"

#define NAME empty

#define IN ""

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
            json_object_from_file(IN));
    test_case(&tst.t, !jo_input, "input failed to parse");

    json_object *jo_output = memlst_add(tst.p_mem, dtor_json_object,
        tchatator413_interpret(jo_input, tst.cfg, tst.db, on_action, on_response, &tst));
    test_case_n_actions(&tst, 0);

    test_output_json_file(&tst, jo_output, OUT_JSON(NAME, ));

    return tst.t;
}
