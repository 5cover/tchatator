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

static token_t gs_token_member1;
static token_t gs_token_pro1;
static serial_t gs_msg_id;
static time_t gs_msg_sent_at;

static void on_action(action_t const *action, void *t) {
    test_t const *test = base_on_action(t);
    switch (test->n_actions) {
    case 1: // send
        if (!test_case_eq_int(t, action->type, action_type_send, )) return;
        test_case_eq_int64(t, action->with.send.token, gs_token_member1, );
        test_case_eq_str(t, action->with.send.content.val, MSG_CONTENT, );
        test_case_eq_int(t, action->with.send.dest_user_id, USER_ID_PRO1, );
        break;
    case 2: // inbox
        if (!test_case_eq_int(t, action->type, action_type_inbox, )) return;
        test_case_eq_int64(t, action->with.inbox.token, gs_token_pro1, );
        test_case_eq_int(t, action->with.inbox.page, 1, );
        break;
    case 3: // rm
        if (!test_case_eq_int(t, action->type, action_type_rm, )) return;
        test_case_eq_int64(t, action->with.rm.token, gs_token_member1, );
        test_case_eq_int(t, action->with.rm.msg_id, gs_msg_id, );
        break;
    default: test_fail(t, "wrong test->n_actions: %d", test->n_actions);
    }
}

static void on_response(response_t const *response, void *t) {
    test_t const *test = base_on_response(t);
    test_case(t, !response->has_next_page, "");
    switch (test->n_responses) {
    case 1: { // send
        if (!test_case_eq_int(t, response->type, action_type_send, )) return;
        void *memory_owner_db;
        msg_t msg = { .id = response->body.send.msg_id };
        if (!test_case(t, errstatus_ok == db_get_msg(test->db, test->cfg, &msg, &memory_owner_db), "sent msg id %d exists", msg.id)) return;
        gs_msg_id = msg.id;
        gs_msg_sent_at = msg.sent_at;
        test_case_eq_int(t, msg.read_age, 0, );
        test_case_eq_int(t, msg.edited_age, 0, );
        test_case_eq_int(t, msg.deleted_age, 0, );
        test_case_eq_int(t, msg.user_id_sender, USER_ID_MEMBER1, );
        test_case_eq_int(t, msg.user_id_recipient, USER_ID_PRO1, );
        test_case_eq_str(t, msg.content, MSG_CONTENT, );
        db_collect(memory_owner_db);
        break;
    }
    case 2: { // inbox
        if (!test_case_eq_int(t, response->type, action_type_inbox, )) return;
        if (!test_case_eq_int64(t, response->body.inbox.n_msgs, 1, )) return;
        msg_t msg = response->body.inbox.msgs[0];
        test_case_eq_int(t, msg.id, gs_msg_id, );
        test_case_eq_int64(t, msg.sent_at, gs_msg_sent_at, );
        test_case_eq_int(t, msg.read_age, 0, );
        test_case_eq_int(t, msg.edited_age, 0, );
        test_case_eq_int(t, msg.deleted_age, 0, );
        test_case_eq_int(t, msg.user_id_sender, USER_ID_MEMBER1, );
        test_case_eq_int(t, msg.user_id_recipient, USER_ID_PRO1, );
        test_case_eq_str(t, msg.content, MSG_CONTENT, );
        break;
    }
    case 3: // rm
        test_case_eq_int(t, response->type, action_type_rm, );
        break;
    default: test_fail(t, "wrong test->n_responses: %d", test->n_actions);
    }
}

TEST_SIGNATURE(NAME) {
    test_t test = TEST_INIT(NAME);

    uuid4_t api_key_m1, api_key_p1;
    assert(uuid4_parse(&api_key_m1, API_KEY_MEMBER1));
    assert(uuid4_parse(&api_key_p1, API_KEY_PRO1));

    // Member logs in
    {
        response_t response = action_evaluate(
            &(action_t) {
                .type = action_type_login,
                .with.login = {
                    .api_key = api_key_m1,
                    .password = SLICE_CONST("member1_mdp"),
                },
            },
            cfg, db, server);
        if (!test_case_eq_int(&test.t, response.type, action_type_login, "member logs in")) return test.t;
        gs_token_member1 = response.body.login.token;
        response_destroy(&response);
    }

    // Member sends message
    {
        json_object *obj_input = load_jsonf(IN_JSONF(NAME, "_send"), gs_token_member1);
        json_object *obj_output = tchatator413_interpret(obj_input, cfg, db, server, on_action, on_response, &test);
        test_case_n_actions(&test, 1);
        json_object *obj_expected_output = load_jsonf(OUT_JSONF(NAME, "_send"), gs_msg_id);
        if (!test_output_json(&test.t, obj_output, obj_expected_output)) return test.t;
        json_object_put(obj_expected_output);
        json_object_put(obj_input);
        json_object_put(obj_output);
    }

    // Pro logs in
    {
        response_t response = action_evaluate(
            &(action_t) {
                .type = action_type_login,
                .with.login = {
                    .api_key = api_key_p1,
                    .password = SLICE_CONST("pro1_mdp"),
                },
            },
            cfg, db, server);
        if (!test_case_eq_int(&test.t, response.type, action_type_login, "pro logs in")) return test.t;
        gs_token_pro1 = response.body.login.token;
        response_destroy(&response);
    }

    // Pro queries inbox
    {
        json_object *obj_input = load_jsonf(IN_JSONF(NAME, "_inbox"), gs_token_pro1);
        json_object *obj_output = tchatator413_interpret(obj_input, cfg, db, server, on_action, on_response, &test);
        test_case_n_actions(&test, 2);
        json_object *obj_expected_output = load_jsonf(OUT_JSONF(NAME, "_inbox"), gs_msg_id, gs_msg_sent_at);
        test_output_json(&test.t, obj_output, obj_expected_output);
        json_object_put(obj_expected_output);
        json_object_put(obj_input);
        json_object_put(obj_output);
    }

    // Pro logs out
    {
        response_t response = action_evaluate(
            &(action_t) {
                .type = action_type_logout,
                .with.logout.token = gs_token_pro1 },
            cfg, db, server);
        test_case_eq_int(&test.t, response.type, action_type_logout, "pro logs out");
        response_destroy(&response);
    }

    // Member deletes message
    {
        json_object *obj_input = load_jsonf(IN_JSONF(NAME, "_rm"), gs_token_member1, gs_msg_id);
        json_object *obj_output = tchatator413_interpret(obj_input, cfg, db, server, on_action, on_response, &test);
        test_case_n_actions(&test, 3);
        test_output_json_file(&test, obj_output, OUT_JSON(NAME, "_rm"));

        // Member logs out
        response_t response = action_evaluate(
            &(action_t) {
                .type = action_type_logout,
                .with.logout.token = gs_token_member1 },
            cfg, db, server);
        if (!test_case_eq_int(&test.t, response.type, action_type_logout, "member logs out")) return test.t;
        response_destroy(&response);

        json_object_put(obj_input);
        json_object_put(obj_output);
    }

    return test.t;
}
