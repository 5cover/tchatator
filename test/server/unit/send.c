/// @file
/// @author Raphaël
/// @brief Tchatator413 test - member1 sends a message
/// @date 11/02/2025

#include "../tests.h"
#include "tchatator413/action.h"
#include "tchatator413/db.h"
#include "tchatator413/tchatator413.h"

#define MSG_CONTENT "I. Am. A. God. (i am a god)"

typedef struct {
    test_t test;
    token_t token;
    serial_t user_id_sender, user_id_recipient, msg_id;
    bool msg_id_set;
} context_t;

static inline void on_action_send(action_t const *action, void *ctx) {
    context_t *c = ctx;
    base_on_action(&c->test);
    if (!test_case_eq_int(&c->test.t, action->type, action_type_send, )) return;
    test_case_eq_str(&c->test.t, action->with.send.content.val, MSG_CONTENT, );
    test_case_eq_int(&c->test.t, action->with.send.dest_user_id, c->user_id_recipient, );
    test_case_eq_int64(&c->test.t, action->with.send.token, c->token, );
}

static inline void on_response_send(response_t const *response, void *ctx) {
    context_t *c = ctx;
    base_on_response(&c->test);
    c->msg_id_set = false;
    if (!test_case_eq_int(&c->test.t, response->type, action_type_send, )) return;
    c->msg_id = response->body.send.msg_id;
    c->msg_id_set = true;
}

static inline errstatus_t transaction(db_t *db, cfg_t *cfg, void *ctx) {
    context_t *c = ctx;

    json_object *obj_input, *obj_output, *obj_output_expected;

    // Send a message
    obj_input = load_jsonf(IN_JSONF(send, "_send"), c->token, c->user_id_recipient);
    time_t msg_sentat = time(NULL);
    obj_output = tchatator413_interpret(obj_input, cfg, db, c->test.server, on_action_send, on_response_send, ctx);
    json_object_put(obj_input);
    test_case_n_actions(&c->test, 1);

    if (!c->msg_id_set) return errstatus_tested;

    obj_output_expected = load_jsonf(OUT_JSONF(send, "_send"), c->msg_id);
    test_case_eq_json_object(&c->test.t, obj_output, obj_output_expected, );
    json_object_put(obj_output_expected);
    json_object_put(obj_output);

    // Try to retrieve it
    msg_t msg = { .id = c->msg_id };
    void *msg_memory_owner;
    if (!test_case_eq_int(&c->test.t, db_get_msg(db, cfg, &msg, &msg_memory_owner), errstatus_ok, )) return errstatus_handled;

    // Assert its properties
    test_case_eq_str(&c->test.t, msg.content, MSG_CONTENT, );
    test_case_eq_int64(&c->test.t, msg.sent_at, msg_sentat, );
    test_case_eq_int(&c->test.t, msg.deleted_age, 0, );
    test_case_eq_int(&c->test.t, msg.edited_age, 0, );
    test_case_eq_int(&c->test.t, msg.read_age, 0, );
    test_case_eq_int(&c->test.t, msg.id, c->msg_id, );
    test_case_eq_int(&c->test.t, msg.user_id_recipient, c->user_id_recipient, );
    test_case_eq_int(&c->test.t, msg.user_id_sender, c->user_id_sender, );

    db_collect(msg_memory_owner);

    return errstatus_tested; // Trigger a rollback by returning a non-ok, special result.
}

#define NAME member1_send
TEST_SIGNATURE(NAME) {
    test_t test = TEST_INIT(NAME);

    response_t response;

    // Login as member1
    response = action_evaluate(
        &(action_t) {
            .type = action_type_login,
            .with.login = {
                .api_key = API_KEY_MEMBER1_UUID,
                .password = SLICE_CONST("member1_mdp"),
            } },
        cfg, db, server);
    assert(response.type == action_type_login);
    token_t token = response.body.login.token;
    response_destroy(&response);

    test_case_eq_int(&test.t,
        db_transaction(db, cfg, transaction,
            &(context_t) {
                .test = test,
                .token = token,
                .user_id_sender = USER_ID_MEMBER1,
                .user_id_recipient = USER_ID_PRO1,
            }),
        errstatus_tested, );

    // Although it would make for a more thourough clean up. logging out doesn't seem necessary at this point.

    return test.t;
}

#undef NAME
#define NAME pro1_send

TEST_SIGNATURE(NAME) {
    test_t test = TEST_INIT(NAME);

    response_t response;

    response = action_evaluate(
        &(action_t) {
            .type = action_type_login,
            .with.login = {
                .api_key = API_KEY_PRO1_UUID,
                .password = SLICE_CONST("pro1_mdp"),
            } },
        cfg, db, server);
    assert(response.type == action_type_login);
    token_t token = response.body.login.token;
    response_destroy(&response);

    test_case_eq_int(&test.t,
        db_transaction(db, cfg, transaction,
            &(context_t) {
                .test = test,
                .token = token,
                .user_id_sender = USER_ID_PRO1,
                .user_id_recipient = USER_ID_MEMBER1,
            }),
        errstatus_tested, );

    return test.t;
}
