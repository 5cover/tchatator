/// @file
/// @author Raphaël
/// @brief Tchatator413 test
///
/// Tests the sending, retrieval and removal of a message
/// - member1 send
/// - pro1 inbox
/// - member1 rm
///
/// @date 1/02/2025

#include "../tests.h"
#include <tchatator413/action.h>
#include <tchatator413/tchatator413.h>

#define NAME member1_send_pro1_inbox_member1_rm

#define MSG_CONTENT "Bonjour du language C :)"

static serial_t gs_msg_id;
static time_t gs_msg_sent_at;

static void on_action(action_t const *action, void *t) {
    test_t const *test = base_on_action(t);
    switch (test->n_actions) {
    case 1: // send
        if (!TEST_CASE_EQ_INT(t, action->type, action_type_send, )) return;
        TEST_CASE_EQ_UUID(t, action->with.send.constr.api_key, API_KEY_MEMBER1_UUID, );
        TEST_CASE_EQ_STR(t, action->with.send.constr.password, "member1_mdp", );
        TEST_CASE_EQ_STR(t, action->with.send.content.val, MSG_CONTENT, );
        TEST_CASE_EQ_INT(t, action->with.send.dest_user_id, USER_ID_PRO1, );
        break;
    case 2: // inbox
        if (!TEST_CASE_EQ_INT(t, action->type, action_type_inbox, )) return;
        TEST_CASE_EQ_UUID(t, action->with.send.constr.api_key, API_KEY_PRO1_UUID, );
        TEST_CASE_EQ_STR(t, action->with.send.constr.password, "pro1_mdp", );
        TEST_CASE_EQ_INT(t, action->with.inbox.page, 1, );
        break;
    case 3: // rm
        if (!TEST_CASE_EQ_INT(t, action->type, action_type_rm, )) return;
        TEST_CASE_EQ_UUID(t, action->with.send.constr.api_key, API_KEY_MEMBER1_UUID, );
        TEST_CASE_EQ_STR(t, action->with.send.constr.password, "member1_mdp", );
        TEST_CASE_EQ_INT(t, action->with.rm.msg_id, gs_msg_id, );
        break;
    default: test_fail(t, "wrong test->n_actions: %d", test->n_actions);
    }
}

static void on_response(response_t const *response, void *t) {
    test_t *test = base_on_response(t);
    test_case(t, !response->has_next_page, "");
    switch (test->n_responses) {
    case 1: { // send
        if (!TEST_CASE_EQ_INT(t, response->type, action_type_send, )) return;

        msg_t msg = { .id = response->body.send.msg_id };
        if (!test_case(t, errstatus_ok == db_get_msg(test->db, test->pmem, test->cfg, &msg), "sent msg id %d exists", msg.id)) return;
        gs_msg_id = msg.id;
        gs_msg_sent_at = msg.sent_at;
        TEST_CASE_EQ_INT(t, msg.read_age, 0, );
        TEST_CASE_EQ_INT(t, msg.edited_age, 0, );
        TEST_CASE_EQ_INT(t, msg.deleted_age, 0, );
        TEST_CASE_EQ_INT(t, msg.user_id_sender, USER_ID_MEMBER1, );
        TEST_CASE_EQ_INT(t, msg.user_id_recipient, USER_ID_PRO1, );
        TEST_CASE_EQ_STR(t, msg.content, MSG_CONTENT, );
        break;
    }
    case 2: { // inbox
        if (!TEST_CASE_EQ_INT(t, response->type, action_type_inbox, )) return;
        if (!TEST_CASE_EQ_INT64(t, response->body.inbox.n_msgs, 1, )) return;
        msg_t const *msg = &response->body.inbox.msgs[0];
        TEST_CASE_EQ_INT(t, msg->id, gs_msg_id, );
        TEST_CASE_EQ_INT64(t, msg->sent_at, gs_msg_sent_at, );
        TEST_CASE_EQ_INT(t, msg->read_age, 0, );
        TEST_CASE_EQ_INT(t, msg->edited_age, 0, );
        TEST_CASE_EQ_INT(t, msg->deleted_age, 0, );
        TEST_CASE_EQ_INT(t, msg->user_id_sender, USER_ID_MEMBER1, );
        TEST_CASE_EQ_INT(t, msg->user_id_recipient, USER_ID_PRO1, );
        TEST_CASE_EQ_STR(t, msg->content, MSG_CONTENT, );
        break;
    }
    case 3: // rm
        TEST_CASE_EQ_INT(t, response->type, action_type_rm, );
        break;
    default: test_fail(t, "wrong test->n_responses: %d", test->n_actions);
    }
}

TEST_SIGNATURE(NAME) {
    test_t test = TEST_INIT(NAME);

    uuid4_t api_key_m1, api_key_p1;
    bool ok = uuid4_parse(&api_key_m1, API_KEY_MEMBER1);
    assert(ok);
    ok = uuid4_parse(&api_key_p1, API_KEY_PRO1);
    assert(ok);

    // Member sends message
    {
        json_object *obj_input = memlst_add(test.pmem, dtor_json_object,
            load_jsonf(IN_JSONF(NAME, "_send"), API_KEY_MEMBER1 "¤member1_mdp"));
        json_object *obj_output = memlst_add(test.pmem, dtor_json_object,
            tchatator413_interpret(obj_input, cfg, db, on_action, on_response, &test));

        test_case_n_actions(&test, 1);
        json_object *obj_expected_output = memlst_add(test.pmem, dtor_json_object,
            load_jsonf(OUT_JSONF(NAME, "_send"), gs_msg_id));
        if (!test_output_json(&test.t, obj_output, obj_expected_output)) return test.t;
    }

    // Pro queries inbox
    /*{
        json_object *obj_input = load_jsonf(IN_JSONF(NAME, "_inbox"), API_KEY_PRO1 "¤pro1_mdp");
        json_object *obj_output = tchatator413_interpret(obj_input, cfg, db, on_action, on_response, &test);

        test_case_n_actions(&test, 2);
        json_object *obj_expected_output = load_jsonf(OUT_JSONF(NAME, "_inbox"), gs_msg_id, gs_msg_sent_at);
        test_output_json(&test.t, obj_output, obj_expected_output);
        json_object_put(obj_expected_output);

        json_object_put(obj_input);
        json_object_put(obj_output);
    }

    // Member deletes message
    {
        json_object *obj_input = load_jsonf(IN_JSONF(NAME, "_rm"), API_KEY_MEMBER1 "¤member1_mdp", gs_msg_id);
        json_object *obj_output = tchatator413_interpret(obj_input, cfg, db, on_action, on_response, &test);
        test_case_n_actions(&test, 3);
        test_output_json_file(&test, obj_output, OUT_JSON(NAME, "_rm"));

        json_object_put(obj_input);
        json_object_put(obj_output);
    }*/

    return test.t;
}
