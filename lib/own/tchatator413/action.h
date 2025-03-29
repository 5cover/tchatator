/// @file
/// @author Raphaël
/// @brief Tchatator413 request parsing and interpretation - Interface
/// @date 23/01/2025

#ifndef ACTION_H
#define ACTION_H

#include <json-c.h>
#include <stdbool.h>
#include <stdio.h>

#include "const.h"
#include "cfg.h"
#include "db.h"
#include "server.h"
#include "types.h"

/// @brief Status codes for the Tchatator413 protocol, modeled after HTTP status codes.
/// @remark The resemblance with HTTP status codes is only for familiarity.
typedef enum {
    status_bad_request = 400,           ///< @brief Bad request.
    status_unauthorized = 401,          ///< @brief Unauthorized.
    status_forbidden = 403,             ///< @brief Forbidden.
    status_not_found = 404,             ///< @brief Not found.
    status_payload_too_large = 413,     ///< @brief Payload too large.
    status_unprocessable_content = 422, ///< @brief Unprocessable content.
    status_too_many_requests = 429,     ///< @brief Too many requests.
    status_internal_server_error = 500  ///< @brief Internal server error.
} status_t;

/// @brief Enumerates the type of errors that occur while parsing or running an action.
typedef enum {
    action_error_type_type,        ///< @brief Parsing JSON type error.
    action_error_type_missing_key, ///< @brief Parsing JSON missing key.
    action_error_type_invalid,     ///< @brief Parsing JSON invalid value.
    action_error_type_rate_limit,  ///< @brief The rate limit has been reached.
    action_error_type_invariant,   ///< @brief An invariant was broken.
    action_error_type_other,       ///< @brief Another error occured.
} action_error_type_t;

/// @brief An error that occured while parsing or running an action.
typedef struct {
    /// @brief Type of the error.
    action_error_type_t type;
    /// @brief Payload of the action error, tagged by @ref action_error_t.type.
    union {
        struct {
            /// @brief A human-friendly representation of the location of the error in the JSON request structure.
            char const *location;
            /// @brief The JSON object that is of the wrong type.
            /// @note Invariant: the type of this object is different from @ref expected.
            json_object *obj_actual;
            /// @brief The type @ref obj_actual should have been of.
            json_type expected;
        } type;
        struct {
            /// @brief A human-friendly representation of the location of the error in the JSON request structure, including the missing key.
            char const *location;
        } missing_key;
        struct {
            /// @brief A human-friendly representation of the location of the error in the JSON request structure.
            char const *location;
            /// @brief The reason why the value is invalid.
            char const *reason;
            /// @brief The faulty JSON object.
            json_object *obj_bad;
        } invalid;
        struct {
            /// @brief The Unix time at which the next request will be accepted.
            time_t next_request_at;
        } rate_limit;
        struct {
            /// @brief The name of the invariant that was broken.
            char const *name;
        } invariant;
        struct {
            /// @brief A status code for the error.
            status_t status;
        } other;
    } info;
} action_error_t;

/// @brief Enumerates the types of actions, representing the various commands available.
typedef enum {
    /// @brief Special value for an action triggered an error.
    action_type_error,

#define X(name) action_type_##name,
    X_ACTIONS(X)
#undef X
} action_type_t;

/// @brief Expands to an identifier that is the type of the action of name @p name
#define ACTION_TYPE(name) CAT(action_type_, name)

/// @brief An action. Actions represent the commands the protocol implements.
typedef struct {
    /// @brief Type of the action.
    action_type_t type;
    /// @brief Payload of the action, tagged by @ref action_t.type.
    union {
        action_error_t error;
        struct {
            api_key_t api_key;
            slice_t password;
        } login;
        struct {
            token_t token;
        } logout, motd;
        struct {
            api_key_t api_key;
            serial_t user_id;
        } whois;
        struct {
            token_t token;
            serial_t dest_user_id;
            slice_t content;
        } send;
        struct {
            token_t token;
            page_number_t page;
        } inbox, outbox;
        struct {
            token_t token;
            serial_t msg_id;
            slice_t new_content;
        } edit;
        struct {
            token_t token;
            serial_t msg_id;
        } rm;
        struct {
            token_t token;
            serial_t user_id;
        } block, unblock, ban, unban;
    } with;
} action_t;

typedef struct {
    action_type_t type;
    bool has_next_page;
    union {
        action_error_t error;
        struct {
            token_t token;
        } login;
        struct {
            user_t user;
        } whois;
        struct {
            serial_t msg_id;
        } send;
        msg_list_t motd, inbox, outbox;
        /*struct {

        } edit;
        struct {

        } rm;
        struct {

        } block;
        struct {

        } unblock;
        struct {

        } ban;
        struct {

        } unban;*/
    } body;
} response_t;

/// @brief Destroy a response.
/// @param response The response to destroy.
void response_destroy(response_t *response);

/// @brief Builds a response for a rate limit error
/// @param next_request_at The time at which the next request will be allowed.
/// @return A new response.
response_t response_for_rate_limit(time_t next_request_at);

/// @brief Put an user role.
/// @param role The role flags
/// @param stream The stream to write to.
void put_role(role_flags_t role, FILE *stream);

/// @brief Parse an action from a JSON object.
/// @param db The configuration.
/// @param db The database connection.
/// @param obj The JSON object allegedly containing an action.
/// @return The parsed action.
action_t action_parse(cfg_t *cfg, db_t *db, json_object *obj);

/// @brief Evaluate an action.
/// @param action The action to evaluate.
/// @param cfg The configuration.
/// @param db The database connection.
/// @param server The server
/// @return The response to the action.
response_t action_evaluate(action_t const *action, cfg_t *cfg, db_t *db, server_t *server);

/// @brief Convert an action response to JSON.
/// @param response The action response.
/// @return A new JSON object.
json_object *response_to_json(response_t *response);

#ifndef NDEBUG
/// @brief Explain an action.
/// @param action The action to explain.
/// @param output The stream to write the exlanation to.
void action_explain(action_t const *action, FILE *output);
#endif // NDEBUG

#endif // ACTION_H
