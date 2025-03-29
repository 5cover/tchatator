/// @file
/// @author Raphaël
/// @brief Tchatator413 protocol - Implementation (JSON-related)
/// @date 23/01/2025

#include <json-c.h>
#include <tchatator413/action.h>
#include <tchatator413/const.h>
#include <tchatator413/errstatus.h>
#include <tchatator413/json-helpers.h>
#include <tchatator413/util.h>

/// @return @ref serial_t The user ID.
/// @return @ref errstatus_handled An error occured and was handled.
/// @return @ref errstatus_error Invalid user key.
static inline serial_t get_user_id(cfg_t *cfg, db_t *db, json_object *obj_user) {
    switch (json_object_get_type(obj_user)) {
    case json_type_int: {
        serial_t maybe_user_id = json_object_get_int(obj_user);
        return maybe_user_id > 0 ? maybe_user_id : errstatus_error;
    }
    case json_type_string: {
        if (json_object_get_string_len(obj_user) > MAX(EMAIL_LENGTH, PSEUDO_LENGTH)) break;
        const char *email_or_pseudo = json_object_get_string(obj_user);
        return strchr(email_or_pseudo, '@')
            ? db_get_user_id_by_email(db, cfg, email_or_pseudo)
            : db_get_user_id_by_name(db, cfg, email_or_pseudo);
    }
    default:;
    }
    return errstatus_error;
}

action_t action_parse(cfg_t *cfg, db_t *db, json_object *obj) {
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

#define fail_type(_location, _obj_actual, _expected)          \
    do {                                                      \
        action.type = action_type_error;                      \
        action.with.error.type = action_error_type_type;      \
        action.with.error.info.type.location = _location;     \
        action.with.error.info.type.obj_actual = _obj_actual; \
        action.with.error.info.type.expected = _expected;     \
        return action;                                        \
    } while (0)

#define fail_invalid(_location, _obj_bad, _reason)           \
    do {                                                     \
        action.type = action_type_error;                     \
        action.with.error.type = action_error_type_invalid;  \
        action.with.error.info.invalid.location = _location; \
        action.with.error.info.invalid.obj_bad = _obj_bad;   \
        action.with.error.info.invalid.reason = _reason;     \
        return action;                                       \
    } while (0)

#define getarg(obj, key, out_value, json_type, getter)         \
    do {                                                       \
        if (!json_object_object_get_ex(obj_with, key, &obj)) { \
            fail_missing_key(arg_loc(key));                    \
        }                                                      \
        if (!getter(obj, out_value)) {                         \
            fail_type(arg_loc(key), obj, json_type);           \
        }                                                      \
    } while (0)

#define getarg_string(obj, key, out_value) getarg(obj, key, out_value, json_type_string, json_object_get_string_strict)
#define getarg_int(obj, key, out_value) getarg(obj, key, out_value, json_type_int, json_object_get_int_strict)
#define getarg_int64(obj, key, out_value) getarg(obj, key, out_value, json_type_int, json_object_get_int64_strict)
#define getarg_user(obj, key, out_value)                                           \
    do {                                                                           \
        if (!json_object_object_get_ex(obj_with, key, &obj)) {                     \
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
        if (json_object_object_get_ex(obj_with, key, &obj)) {           \
            getarg_int(obj, key, out_value);                            \
            if (*out_value < 1) {                                       \
                fail_invalid(arg_loc(key), obj, "invalid page number"); \
            }                                                           \
        } else {                                                        \
            *out_value = 1;                                             \
        }                                                               \
    } while (0)
#define getarg_api_key(obj, get, out_value)                              \
    do {                                                                 \
        slice_t api_key_repr;                                            \
        getarg_string(obj, "api_key", &api_key_repr);                    \
        if (!uuid4_parse_slice(&action.with.DO.api_key, api_key_repr)) { \
            fail_invalid(arg_loc("api_key"), obj, "invalid API key");    \
        }                                                                \
    } while (0)

#define arg_loc(key) (STR(DO) ".with." key)

    json_object *obj_do;
    if (!json_object_object_get_ex(obj, "do", &obj_do)) fail_missing_key("action.do");

    slice_t action_name;
    if (!json_object_get_string_strict(obj_do, &action_name)) fail_type("action", obj_do, json_type_string);

    json_object *obj_with;
    if (!json_object_object_get_ex(obj, "with", &obj_with)) fail_missing_key("action.with");

// this is save because the null terminator of the literal string STR(name) will stop strcmp
#define action_is(name) streq(STR(name), action_name.val)

#define DO login
    if (action_is(DO)) {
        action.type = ACTION_TYPE(DO);

        // api_key
        json_object *obj_api_key;
        getarg_api_key(obj_api_key, "api_key", &action.with.DO.api_key);

        // password
        json_object *obj_password;
        getarg_string(obj_password, "password", &action.with.DO.password);
#undef DO
#define DO logout
    } else if (action_is(DO)) {
        action.type = ACTION_TYPE(DO);

        // token
        json_object *obj_token;
        getarg_int64(obj_token, "token", &action.with.DO.token);
#undef DO
#define DO whois
    } else if (action_is(DO)) {
        action.type = ACTION_TYPE(DO);

        // api_key
        json_object *obj_api_key;
        getarg_api_key(obj_api_key, "api_key", &action.with.DO.api_key);

        // user
        json_object *obj_user;
        getarg_user(obj_user, "user", &action.with.DO.user_id);
#undef DO
#define DO send
    } else if (action_is(DO)) {
        action.type = ACTION_TYPE(DO);

        // token
        json_object *obj_token;
        getarg_int64(obj_token, "token", &action.with.DO.token);

        // content
        json_object *obj_content;
        getarg_string(obj_content, "content", &action.with.DO.content);

        // dest
        json_object *obj_dest;
        getarg_user(obj_dest, "dest", &action.with.DO.dest_user_id);
#undef DO
#define DO motd
    } else if (action_is(DO)) {
        action.type = ACTION_TYPE(DO);

        // token
        json_object *obj_token;
        getarg_int64(obj_token, "token", &action.with.DO.token);
#undef DO
#define DO inbox
    } else if (action_is(DO)) {
        action.type = ACTION_TYPE(DO);

        // token
        json_object *obj_token;
        getarg_int64(obj_token, "token", &action.with.DO.token);

        // page
        json_object *obj_page;
        getarg_page(obj_page, "page", &action.with.DO.page);
#undef DO
#define DO outbox
    } else if (action_is(DO)) {
        action.type = ACTION_TYPE(DO);

        // token
        json_object *obj_token;
        getarg_int64(obj_token, "token", &action.with.DO.token);

        // page
        json_object *obj_page;
        getarg_page(obj_page, "page", &action.with.DO.page);
#undef DO
#define DO edit
    } else if (action_is(DO)) {
        action.type = ACTION_TYPE(DO);

        // token
        json_object *obj_token;
        getarg_int64(obj_token, "token", &action.with.DO.token);

        // msg_id
        json_object *obj_msg_id;
        getarg_int(obj_msg_id, "msg_id", &action.with.DO.msg_id);

        // new_content
        json_object *obj_new_content;
        getarg_string(obj_new_content, "new_content", &action.with.DO.new_content);
#undef DO
#define DO rm
    } else if (action_is(DO)) {
        action.type = ACTION_TYPE(DO);

        // token
        json_object *obj_token;
        getarg_int64(obj_token, "token", &action.with.DO.token);

        // msg_id
        json_object *obj_msg_id;
        getarg_int(obj_msg_id, "msg_id", &action.with.DO.msg_id);
#undef DO
#define DO block
    } else if (action_is(DO)) {
        action.type = ACTION_TYPE(DO);

        // token
        json_object *obj_token;
        getarg_int64(obj_token, "token", &action.with.DO.token);

        // user
        json_object *obj_user;
        getarg_user(obj_user, "user", &action.with.DO.user_id);
#undef DO
#define DO unblock
    } else if (action_is(DO)) {
        action.type = ACTION_TYPE(DO);

        // token
        json_object *obj_token;
        getarg_int64(obj_token, "token", &action.with.DO.token);

        // user
        json_object *obj_user;
        getarg_user(obj_user, "user", &action.with.DO.user_id);
#undef DO
#define DO ban
    } else if (action_is(DO)) {
        action.type = ACTION_TYPE(DO);

        // token
        json_object *obj_token;
        getarg_int64(obj_token, "token", &action.with.DO.token);

        // user
        json_object *obj_user;
        getarg_user(obj_user, "user", &action.with.DO.user_id);
#undef DO
#define DO unban
    } else if (action_is(DO)) {
        action.type = ACTION_TYPE(DO);
        // token
        json_object *obj_token;
        getarg_int64(obj_token, "token", &action.with.DO.token);

        // user
        json_object *obj_user;
        getarg_user(obj_user, "user", &action.with.DO.user_id);
    } else {
        cfg_log(cfg, log_error, "unknown action: %s\n", action_name.val);
        fail();
    }

    return action;
}

#define add_key(o, k, v) json_object_object_add_ex(o, k, v, JSON_C_OBJECT_ADD_KEY_IS_NEW | JSON_C_OBJECT_KEY_IS_CONSTANT)

static json_object *msg_to_json_object(msg_t *msg) {
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
    json_object *obj_body = NULL, *obj_error = NULL;

    switch (response->type) {
    case action_type_error: {
        status_t status;
        obj_error = json_object_new_object();
        switch (response->body.error.type) {
        case action_error_type_type: {
            status = status_bad_request;
            json_object *obj_actual = response->body.error.info.type.obj_actual;
            json_type actual_type = json_object_get_type(obj_actual);
            char *msg = actual_type == json_type_null
                ? strfmt("%s: expected %s, got %s",
                      response->body.error.info.type.location,
                      json_type_to_name(response->body.error.info.type.expected),
                      json_type_to_name(actual_type))
                : strfmt("%s: expected %s, got %s: %s",
                      response->body.error.info.type.location,
                      json_type_to_name(response->body.error.info.type.expected),
                      json_type_to_name(actual_type),
                      min_json(obj_actual));
            if (msg) add_key(obj_error, "message", json_object_new_string(msg));
            free(msg);
            break;
        }
        case action_error_type_missing_key: {
            status = status_bad_request;
            char *msg = strfmt("%s: key missing", response->body.error.info.missing_key.location);
            if (msg) add_key(obj_error, "message", json_object_new_string(msg));
            free(msg);
            break;
        }
        case action_error_type_invalid: {
            status = status_bad_request;
            char *msg = strfmt("%s: %s: %s", response->body.error.info.invalid.location,
                response->body.error.info.invalid.reason,
                json_object_to_json_string(response->body.error.info.invalid.obj_bad));
            if (msg) add_key(obj_error, "message", json_object_new_string(msg));
            free(msg);
            break;
        }
        case action_error_type_other: {
            status = response->body.error.info.other.status;
            break;
        }
        case action_error_type_rate_limit: {
            status = status_too_many_requests;
            add_key(obj_error, "next_request_at", json_object_new_int64(response->body.error.info.rate_limit.next_request_at));
            break;
        }
        default: unreachable();
        }
        add_key(obj_error, "status", json_object_new_int(status));
        break;
    }
    case action_type_login:
        obj_body = json_object_new_object();
        add_key(obj_body, "token", json_object_new_int64(response->body.login.token));
        break;
    case action_type_logout:

        break;
    case action_type_whois:
        obj_body = json_object_new_object();
        add_key(obj_body, "user_id", json_object_new_int(response->body.whois.user.id));
        add_key(obj_body, "email", json_object_new_string(response->body.whois.user.email));
        add_key(obj_body, "last_name", json_object_new_string(response->body.whois.user.last_name));
        add_key(obj_body, "first_name", json_object_new_string(response->body.whois.user.first_name));
        add_key(obj_body, "display_name", json_object_new_string(response->body.whois.user.display_name));
        add_key(obj_body, "kind", json_object_new_int(response->body.whois.user.kind));
        break;
    case action_type_send:
        obj_body = json_object_new_object();
        add_key(obj_body, "msg_id", json_object_new_int(response->body.send.msg_id));
        break;
    case action_type_motd:

        break;
    case action_type_inbox: {
        obj_body = json_object_new_array();

        for (size_t i = 0; i < response->body.inbox.n_msgs; ++i) {
            json_object_array_add(obj_body, msg_to_json_object(response->body.inbox.msgs));
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
    if (obj_body) add_key(obj, "body", obj_body);
    if (obj_error) add_key(obj, "error", obj_error);

#undef add_key

    return obj;
}
