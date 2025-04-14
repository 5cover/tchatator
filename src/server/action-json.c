/// @file
/// @author Raphaël
/// @brief Tchatator413 protocol - Implementation (JSON-related)
/// @date 23/01/2025

#include "json-c.h"
#include "tchatator413/action.h"
#include "tchatator413/const.h"
#include "tchatator413/errstatus.h"
#include "tchatator413/json-helpers.h"
#include "util.h"

/// @return @ref serial_t The user ID.
/// @return @ref errstatus_handled An error occured and was handled.
/// @return @ref errstatus_error Invalid user key.
static inline serial_t
get_user_id(cfg_t *cfg, db_t *db, json_object *jo_user) {
    switch (json_object_get_type(jo_user)) {
    case json_type_int: {
        serial_t maybe_user_id = json_object_get_int(jo_user);
        return maybe_user_id > 0 ? maybe_user_id : errstatus_error;
    }
    case json_type_string: {
        if (json_object_get_string_len(jo_user) > MAX(EMAIL_LENGTH, PSEUDO_LENGTH)) break;
        const char *email_or_pseudo = json_object_get_string(jo_user);
        return strchr(email_or_pseudo, '@')
            ? db_get_user_id_by_email(db, cfg, email_or_pseudo)
            : db_get_user_id_by_name(db, cfg, email_or_pseudo);
    }
    default:;
    }
    return errstatus_error;
}

action_t action_parse(memlst_t **pmem, cfg_t *cfg, db_t *db, json_object const *obj) {
    // json_object internal memory is not considered stable enough to reuse outside of this function, so we must duplicate extracted pointers (such as strings).

    action_t action = { 0 };

#define fail()                                                              \
    do {                                                                    \
        action.type = action_type_error;                                    \
        action.with.error.type = action_error_type_other;                   \
        action.with.error.info.other.status = status_internal_server_error; \
        return action;                                                      \
    } while (0)

#define fail_missing_key(_location)                              \
    do {                                                         \
        action.type = action_type_error;                         \
        action.with.error.type = action_error_type_missing_key;  \
        action.with.error.info.missing_key.location = _location; \
        return action;                                           \
    } while (0)

#define fail_type(_location, _jo_actual, _expected)          \
    do {                                                      \
        action.type = action_type_error;                      \
        action.with.error.type = action_error_type_type;      \
        action.with.error.info.type.location = _location;     \
        action.with.error.info.type.jo_actual = _jo_actual; \
        action.with.error.info.type.expected = _expected;     \
        return action;                                        \
    } while (0)

#define fail_invalid(_location, _jo_bad, _reason)           \
    do {                                                     \
        action.type = action_type_error;                     \
        action.with.error.type = action_error_type_invalid;  \
        action.with.error.info.invalid.location = _location; \
        action.with.error.info.invalid.jo_bad = _jo_bad;   \
        action.with.error.info.invalid.reason = _reason;     \
        return action;                                       \
    } while (0)

#define getarg(obj, key, out_value, json_type, getter)         \
    do {                                                       \
        if (!json_object_object_get_ex(jo_with, key, &obj)) { \
            fail_missing_key(arg_loc(key));                    \
        }                                                      \
        if (!getter(obj, out_value)) {                         \
            fail_type(arg_loc(key), obj, json_type);           \
        }                                                      \
    } while (0)

#define getarg_int(obj, key, out_value) getarg(obj, key, out_value, json_type_int, json_object_get_int_strict)
#define getarg_int64(obj, key, out_value) getarg(obj, key, out_value, json_type_int, json_object_get_int64_strict)
#define getarg_string(obj, key, out_value)                         \
    do {                                                           \
        if (!json_object_object_get_ex(jo_with, key, &obj)) {     \
            fail_missing_key(arg_loc(key));                        \
        }                                                          \
        if (!json_object_get_string_strict(obj, out_value)) {      \
            fail_type(arg_loc(key), obj, json_type_string);        \
        }                                                          \
        if (!((out_value)->val = memlst_add(pmem, free,            \
                  strndup((out_value)->val, (out_value)->len)))) { \
            errno_exit("strdup");                                  \
        }                                                          \
    } while (0)
#define DELIMITER "¤"
#define getarg_constr(obj, key, out_value)                                                \
    do {                                                                                  \
        if (!json_object_object_get_ex(jo_with, key, &obj)) {                            \
            fail_missing_key(arg_loc(key));                                               \
        }                                                                                 \
        slice_t constr;                                                                   \
        if (!json_object_get_string_strict(obj, &constr)) {                               \
            fail_type(arg_loc(key), obj, json_type_string);                               \
        }                                                                                 \
        if (!uuid4_parse_slice(&(out_value)->api_key, constr)) {                          \
            fail_invalid(arg_loc(key), obj, "invalid API key");                           \
        }                                                                                 \
        if (constr.len >= UUID4_REPR_LENGTH + sizeof DELIMITER - 1                        \
            && strneq(constr.val + UUID4_REPR_LENGTH, DELIMITER, sizeof DELIMITER - 1)) { \
            if (!((out_value)->password = memlst_add(pmem, free,                          \
                      strdup(constr.val + UUID4_REPR_LENGTH + sizeof DELIMITER - 1)))) {  \
                errno_exit("strdup");                                                     \
            }                                                                             \
        } else {                                                                          \
            (out_value)->password = NULL;                                                 \
        }                                                                                 \
    } while (0)

#define getarg_user(obj, key, out_value)                                           \
    do {                                                                           \
        if (!json_object_object_get_ex(jo_with, key, &obj)) {                     \
            fail_missing_key(arg_loc(key));                                        \
        }                                                                          \
        switch (*out_value = get_user_id(cfg, db, obj)) {                          \
        case errstatus_error: fail_invalid(arg_loc(key), obj, "invalid user key"); \
        case errstatus_handled: fail();                                            \
        case errstatus_ok:;                                                        \
        }                                                                          \
    } while (0)
#define getarg_page(obj, key, out_value)                                \
    do { /* temporary fix to make optional arguments */                 \
        if (json_object_object_get_ex(jo_with, key, &obj)) {           \
            getarg_int(obj, key, out_value);                            \
            if (*out_value < 1) {                                       \
                fail_invalid(arg_loc(key), obj, "invalid page number"); \
            }                                                           \
        } else {                                                        \
            *out_value = 1;                                             \
        }                                                               \
    } while (0)
#define getarg_api_key(obj, get)                                         \
    do {                                                                 \
        slice_t api_key_repr;                                            \
        getarg_string(obj, "api_key", &api_key_repr);                    \
        if (!uuid4_parse_slice(&action.with.DO.api_key, api_key_repr)) { \
            fail_invalid(arg_loc("api_key"), obj, "invalid API key");    \
        }                                                                \
    } while (0)

#define arg_loc(key) (STR(DO) ".with." key)

    json_object *jo_do;
    if (!json_object_object_get_ex(obj, "do", &jo_do)) fail_missing_key("action.do");

    slice_t action_name;
    if (!json_object_get_string_strict(jo_do, &action_name)) fail_type("action", jo_do, json_type_string);

    json_object *jo_with;
    if (!json_object_object_get_ex(obj, "with", &jo_with)) fail_missing_key("action.with");

// this is save because the null terminator of the literal string STR(name) will stop strcmp
#define action_is(name) streq(STR(name), action_name.val)

#define DO whois
    if (action_is(DO)) {
        action.type = ACTION_TYPE(DO);
        json_object *jo_constr;
        getarg_constr(jo_constr, "constr", &action.with.DO.constr);
        json_object *jo_user;
        getarg_user(jo_user, "user", &action.with.DO.user_id);
#undef DO
#define DO send
    } else if (action_is(DO)) {
        action.type = ACTION_TYPE(DO);
        json_object *jo_constr;
        getarg_constr(jo_constr, "constr", &action.with.DO.constr);
        json_object *jo_content;
        getarg_string(jo_content, "content", &action.with.DO.content);
        json_object *jo_dest;
        getarg_user(jo_dest, "dest", &action.with.DO.dest_user_id);
#undef DO
#define DO motd
    } else if (action_is(DO)) {
        action.type = ACTION_TYPE(DO);
        json_object *jo_constr;
        getarg_constr(jo_constr, "constr", &action.with.DO.constr);
#undef DO
#define DO inbox
    } else if (action_is(DO)) {
        action.type = ACTION_TYPE(DO);
        json_object *jo_constr;
        getarg_constr(jo_constr, "constr", &action.with.DO.constr);
        json_object *jo_page;
        getarg_page(jo_page, "page", &action.with.DO.page);
#undef DO
#define DO outbox
    } else if (action_is(DO)) {
        action.type = ACTION_TYPE(DO);
        json_object *jo_constr;
        getarg_constr(jo_constr, "constr", &action.with.DO.constr);
        json_object *jo_page;
        getarg_page(jo_page, "page", &action.with.DO.page);
#undef DO
#define DO edit
    } else if (action_is(DO)) {
        action.type = ACTION_TYPE(DO);
        json_object *jo_constr;
        getarg_constr(jo_constr, "constr", &action.with.DO.constr);
        json_object *jo_msg_id;
        getarg_int(jo_msg_id, "msg_id", &action.with.DO.msg_id);
        json_object *jo_new_content;
        getarg_string(jo_new_content, "new_content", &action.with.DO.new_content);
#undef DO
#define DO rm
    } else if (action_is(DO)) {
        action.type = ACTION_TYPE(DO);
        json_object *jo_constr;
        getarg_constr(jo_constr, "constr", &action.with.DO.constr);
        json_object *jo_msg_id;
        getarg_int(jo_msg_id, "msg_id", &action.with.DO.msg_id);
#undef DO
#define DO block
    } else if (action_is(DO)) {
        action.type = ACTION_TYPE(DO);
        json_object *jo_constr;
        getarg_constr(jo_constr, "constr", &action.with.DO.constr);
        json_object *jo_user;
        getarg_user(jo_user, "user", &action.with.DO.user_id);
#undef DO
#define DO unblock
    } else if (action_is(DO)) {
        action.type = ACTION_TYPE(DO);
        json_object *jo_constr;
        getarg_constr(jo_constr, "constr", &action.with.DO.constr);
        json_object *jo_user;
        getarg_user(jo_user, "user", &action.with.DO.user_id);
#undef DO
#define DO ban
    } else if (action_is(DO)) {
        action.type = ACTION_TYPE(DO);
        json_object *jo_constr;
        getarg_constr(jo_constr, "constr", &action.with.DO.constr);
        json_object *jo_user;
        getarg_user(jo_user, "user", &action.with.DO.user_id);
#undef DO
#define DO unban
    } else if (action_is(DO)) {
        action.type = ACTION_TYPE(DO);
        json_object *jo_constr;
        getarg_constr(jo_constr, "constr", &action.with.DO.constr);
        json_object *jo_user;
        getarg_user(jo_user, "user", &action.with.DO.user_id);
    } else {
        cfg_log(cfg, log_error, "unknown action: %s\n", action_name.val);
        fail();
    }

    return action;
}

#define add_key(o, k, v) json_object_object_add_ex(o, k, v, JSON_C_OBJECT_ADD_KEY_IS_NEW | JSON_C_OBJECT_KEY_IS_CONSTANT)

static json_object *msg_to_json_object(msg_t const *msg) {
    json_object *obj = json_object_new_object();
    add_key(obj, "msg_id", json_object_new_int(msg->id));
    add_key(obj, "sent_at", json_object_new_int64(msg->sent_at));
    add_key(obj, "content", json_object_new_string(msg->content));
    add_key(obj, "sender", json_object_new_int(msg->user_id_sender));
    add_key(obj, "recipient", json_object_new_int(msg->user_id_recipient));
    if (msg->deleted_age) add_key(obj, "deleted_age", json_object_new_int(msg->deleted_age));
    if (msg->read_age) add_key(obj, "read_age", json_object_new_int(msg->read_age));
    if (msg->edited_age) add_key(obj, "edited_age", json_object_new_int(msg->edited_age));
    return obj;
}

json_object *response_to_json(response_t *response) {
    json_object *jo_body = NULL, *jo_error = NULL;

    switch (response->type) {
    case action_type_error: {
        status_t status;
        jo_error = json_object_new_object();
        switch (response->body.error.type) {
        case action_error_type_type: {
            status = status_bad_request;
            json_object *jo_actual = response->body.error.info.type.jo_actual;
            json_type actual_type = json_object_get_type(jo_actual);
            char *msg = actual_type == json_type_null
                ? strfmt("%s: expected %s, got %s",
                      response->body.error.info.type.location,
                      json_type_to_name(response->body.error.info.type.expected),
                      json_type_to_name(actual_type))
                : strfmt("%s: expected %s, got %s: %s",
                      response->body.error.info.type.location,
                      json_type_to_name(response->body.error.info.type.expected),
                      json_type_to_name(actual_type),
                      min_json(jo_actual));
            if (msg) add_key(jo_error, "message", json_object_new_string(msg));
            free(msg);
            break;
        }
        case action_error_type_missing_key: {
            status = status_bad_request;
            char *msg = strfmt("%s: key missing", response->body.error.info.missing_key.location);
            if (msg) add_key(jo_error, "message", json_object_new_string(msg));
            free(msg);
            break;
        }
        case action_error_type_invalid: {
            status = status_bad_request;
            char *msg = strfmt("%s: %s: %s", response->body.error.info.invalid.location,
                response->body.error.info.invalid.reason,
                json_object_to_json_string(response->body.error.info.invalid.jo_bad));
            if (msg) add_key(jo_error, "message", json_object_new_string(msg));
            free(msg);
            break;
        }
        case action_error_type_other: {
            status = response->body.error.info.other.status;
            break;
        }
        case action_error_type_rate_limit: {
            status = status_too_many_requests;
            add_key(jo_error, "next_request_at", json_object_new_int64(response->body.error.info.rate_limit.next_request_at));
            break;
        }
        default: unreachable();
        }
        add_key(jo_error, "status", json_object_new_int(status));
        break;
    }
    case action_type_whois: {
        jo_body = json_object_new_object();
        user_t *user = &response->body.whois.user;
        add_key(jo_body, "user_id", json_object_new_int(user->id));
        json_object *jo_role = json_object_new_object();
        const char *role_key;
        switch (user->role) {
        case role_admin:
            role_key = "admin";
            break;
        case role_member:
            role_key = "member";
            add_key(jo_role, "user_name", json_object_new_string(user->member.user_name));
            break;
        case role_pro:
            role_key = "pro";
            add_key(jo_role, "business_name", json_object_new_string(user->pro.business_name));
            break;
        default:
            unreachable();
        }
        add_key(jo_body, role_key, jo_body);
        break;
    }
    case action_type_send:
        jo_body = json_object_new_object();
        add_key(jo_body, "msg_id", json_object_new_int(response->body.send.msg_id));
        break;
    case action_type_motd:

        break;
    case action_type_inbox: {
        jo_body = json_object_new_array();

        for (size_t i = 0; i < response->body.inbox.n_msgs; ++i) {
            json_object_array_add(jo_body, msg_to_json_object(&response->body.inbox.msgs[i]));
        }
        break;
    }
    case action_type_outbox:

        break;
    case action_type_edit:

        break;
    case action_type_rm:

        break;
    case action_type_block:

        break;
    case action_type_unblock:

        break;
    case action_type_ban:

        break;
    case action_type_unban:

        break;
    default: unreachable();
    }

    json_object *obj = json_object_new_object();

    if (response->has_next_page) add_key(obj, "has_next_page", json_object_new_boolean(true));
    if (jo_body) add_key(obj, "body", jo_body);
    if (jo_error) add_key(obj, "error", jo_error);

#undef add_key

    return obj;
}
