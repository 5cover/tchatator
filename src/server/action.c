/// @file
/// @author Raphaël
/// @brief Tchatator413 request parsing and interpretation - Implementation
/// @date 23/01/2025

#include <assert.h>
#include <limits.h>
#include <tchatator413/action.h>
#include <tchatator413/db.h>

response_t response_for_rate_limit(time_t next_request_at) {
    return (response_t) {
        .type = action_type_error,
        .body.error = {
            .type = action_error_type_rate_limit,
            .info.rate_limit = {
                .next_request_at = next_request_at,
            } }
    };
}

response_t action_evaluate(action_t const *action, memlst_t **pmem, cfg_t *cfg, db_t *db) {
    response_t rep = { 0 };

#define fail(return_status)                               \
    do {                                                  \
        rep.type = action_type_error;                     \
        rep.body.error.type = action_error_type_other;    \
        rep.body.error.info.other.status = return_status; \
        return rep;                                       \
    } while (0)

#define fail_invariant(invariant_name)                       \
    do {                                                     \
        rep.type = action_type_error;                        \
        rep.body.error.type = action_error_type_invariant;   \
        rep.body.error.info.invariant.name = invariant_name; \
    } while (0)

#define check_role(allowed_roles) \
    if (!(user.role & (allowed_roles))) fail(status_forbidden)

    // Identify user
    user_identity_t user;

    switch (rep.type = action->type) {
    case action_type_error: {
        rep.body.error = action->with.error;
        return rep;
    }

#define DO whois
    case ACTION_TYPE(DO):
        switch (db_verify_user_constr(cfg, db, &user, action->with.DO.constr)) {
        case errstatus_handled: fail(status_internal_server_error);
        case errstatus_error: fail(status_unauthorized);
        default: check_role(role_all);
        }

        rep.body.DO.user.id = action->with.DO.user_id;
        switch (db_get_user(db, pmem, cfg, &rep.body.DO.user)) {
        case errstatus_handled: fail(status_internal_server_error);
        case errstatus_error: fail(status_not_found);
        default:;
        }
        break;
#undef DO
#define DO send
    case ACTION_TYPE(DO): {
        switch (db_verify_user_constr(cfg, db, &user, action->with.DO.constr)) {
        case errstatus_handled: fail(status_internal_server_error);
        case errstatus_error: fail(status_unauthorized);
        default: check_role(role_all);
        }

        int dest_role;
        switch (dest_role = db_get_user_role(db, cfg, action->with.DO.dest_user_id)) {
        case errstatus_handled: fail(status_internal_server_error);
        case errstatus_error: fail(status_not_found);
        }

        // if message length is greater than maximum
        if (action->with.DO.content.len > cfg_max_msg_length(cfg)) fail(status_payload_too_large);

        // if sender and dest are the same user
        if (user.id == action->with.DO.dest_user_id) fail_invariant("no_send_self");
        // if user is client and dest is not pro
        if (user.role & role_member && !(dest_role & role_pro)) fail_invariant("client_send_pro");
        // if user is pro and dest is not a client or dest hasn't contacted pro user first
        if (user.role & role_pro && (!(dest_role & role_member) || !db_count_msg(db, cfg, action->with.DO.dest_user_id, user.id)))
            fail_invariant("pro_responds_client");

        switch (rep.body.DO.msg_id = db_send_msg(db, cfg, user.id, action->with.DO.dest_user_id, action->with.DO.content.val)) {
        case errstatus_handled: fail(status_internal_server_error);
        case errstatus_error: fail(status_forbidden);
        }

        break;
    }
#undef DO
#define DO motd
    case ACTION_TYPE(DO):
        switch (db_verify_user_constr(cfg, db, &user, action->with.DO.constr)) {
        case errstatus_handled: fail(status_internal_server_error);
        case errstatus_error: fail(status_unauthorized);
        default: check_role(role_all);
        }

        break;
#undef DO
#define DO inbox
    case ACTION_TYPE(DO):
        switch (db_verify_user_constr(cfg, db, &user, action->with.DO.constr)) {
        case errstatus_handled: fail(status_internal_server_error);
        case errstatus_error: fail(status_unauthorized);
        default: check_role(role_all);
        }

        if (errstatus_ok != db_get_inbox(db, pmem, cfg, cfg_page_inbox(cfg), cfg_page_inbox(cfg) * (action->with.DO.page - 1), user.id, &rep.body.DO)) {
            fail(status_internal_server_error);
        }
        break;
#undef DO
#define DO outbox
    case ACTION_TYPE(DO):
        switch (db_verify_user_constr(cfg, db, &user, action->with.DO.constr)) {
        case errstatus_handled: fail(status_internal_server_error);
        case errstatus_error: fail(status_unauthorized);
        default: check_role(role_all);
        }

        break;
#undef DO
#define DO edit
    case ACTION_TYPE(DO):
        switch (db_verify_user_constr(cfg, db, &user, action->with.DO.constr)) {
        case errstatus_handled: fail(status_internal_server_error);
        case errstatus_error: fail(status_unauthorized);
        default: check_role(role_all);
        }

        break;
#undef DO
#define DO rm
    case ACTION_TYPE(DO):
        switch (db_verify_user_constr(cfg, db, &user, action->with.DO.constr)) {
        case errstatus_handled: fail(status_internal_server_error);
        case errstatus_error: fail(status_unauthorized);
        default: check_role(role_all);
        }

        switch (db_rm_msg(db, cfg, action->with.DO.msg_id)) {
        case errstatus_handled: fail(status_internal_server_error);
        case errstatus_error: fail(status_not_found);
        default: check_role(role_all);
        }

        break;
#undef DO
#define DO block
    case ACTION_TYPE(DO):
        switch (db_verify_user_constr(cfg, db, &user, action->with.DO.constr)) {
        case errstatus_handled: fail(status_internal_server_error);
        case errstatus_error: fail(status_unauthorized);
        default: check_role(role_admin | role_pro);
        }

        break;
#undef DO
#define DO unblock
    case ACTION_TYPE(DO):
        switch (db_verify_user_constr(cfg, db, &user, action->with.DO.constr)) {
        case errstatus_handled: fail(status_internal_server_error);
        case errstatus_error: fail(status_unauthorized);
        default: check_role(role_admin | role_pro);
        }

        break;
#undef DO
#define DO ban
    case ACTION_TYPE(DO):
        switch (db_verify_user_constr(cfg, db, &user, action->with.DO.constr)) {
        case errstatus_handled: fail(status_internal_server_error);
        case errstatus_error: fail(status_unauthorized);
        default: check_role(role_admin | role_pro);
        }

        break;
#undef DO
#define DO unban
    case ACTION_TYPE(DO):
        switch (db_verify_user_constr(cfg, db, &user, action->with.DO.constr)) {
        case errstatus_handled: fail(status_internal_server_error);
        case errstatus_error: fail(status_unauthorized);
        default: check_role(role_admin | role_pro);
        }

        break;
    }

    return rep;
}

#ifndef NDEBUG
void action_explain(action_t const *action, FILE *output) {
    switch (action->type) {
    case action_type_error:
        fprintf(output, "(none)\n");
        break;
    case action_type_whois:
        fprintf(output, "whois constr=");
        // todo: abstract constr printing
        uuid4_put(action->with.whois.constr.api_key, output);
        if (action->with.whois.constr.password) fprintf(output, "¤%s", action->with.whois.constr.password);
        fprintf(output, " user_id=%d\n", action->with.whois.user_id);
        break;
    case action_type_send:
        fprintf(output, "send\n");
        break;
    case action_type_motd:
        fprintf(output, "motd\n");
        break;
    case action_type_inbox:
        fprintf(output, "inbox\n");
        break;
    case action_type_outbox:
        fprintf(output, "outbox\n");
        break;
    case action_type_edit:
        fprintf(output, "edit\n");
        break;
    case action_type_rm:
        fprintf(output, "rm\n");
        break;
    case action_type_block:
        fprintf(output, "block\n");
        break;
    case action_type_unblock:
        fprintf(output, "unblock\n");
        break;
    case action_type_ban:
        fprintf(output, "ban\n");
        break;
    case action_type_unban:
        fprintf(output, "unban\n");
        break;
    }
}
#endif // NDEBUG

void put_role(role_t role, FILE *stream) {
    if (role & role_admin) fputs("admin", stream);
    if (role & role_member) fputs(role & role_admin ? " or member" : "member", stream);
    if (role & role_pro) fputs(role & (role_admin | role_member) ? " or professionnal" : "professionnal", stream);
}
