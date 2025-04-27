/// @file
/// @author Raphaël
/// @brief Tchatator413 test - whois of 2147483647 by admin
/// @date 1/02/2025

#include "../tests.h"
#include "tchatator413/tchatator413.h"

#define NAME admin_whois_imax

static void on_action(action_t const *action, void *t) {
    test_t *p_tst = base_on_action(t);
    if (!test_case(t, action->type == action_type_whois, "type == %d", action->type)) return;
    test_case(t, cfg_verify_root_constr(p_tst->cfg, action->with.whois.constr), "constr is root");
    TEST_CASE_EQ_INT(t, action->with.whois.user_id, 2147483647, );
}

static void on_response(response_t const *response, void *t) {
    base_on_response(t);
    test_case(t, !response->has_next_page, "");
    if (!TEST_CASE_EQ_INT(t, response->type, action_type_error, )) return;
    if (!TEST_CASE_EQ_INT(t, response->body.error.type, action_error_type_other, )) return;
    TEST_CASE_EQ_INT(t, response->body.error.info.other.status, status_not_found, );
}

TEST_SIGNATURE(NAME) {
    test_t tst = TEST_INIT(NAME);
    
    char uuid_repr[UUID4_REPR_LENGTH];
    json_object *jo_input = memlst_add(tst.p_mem, dtor_json_object,
        load_jsonf(IN_JSONF(NAME, ), uuid4_repr(tst.root_constr.api_key, uuid_repr), tst.root_constr.password));

    json_object *jo_output = memlst_add(tst.p_mem, dtor_json_object,
        tchatator413_interpret(jo_input, tst.cfg, tst.db, on_action, on_response, &tst));
    test_case_n_actions(&tst, 1);

    test_output_json_file(&tst, jo_output, OUT_JSON(NAME, ));

    return tst.t;
}
